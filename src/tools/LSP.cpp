#include "tools/LSP.h"
#include "tools/LSP_Internal.h"
#include "tools/Formatter.h"
#include "diagnostics/DiagnosticPrinter.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantics/SemanticAnalyzer.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <string>

namespace vit {

// ─── Internal helpers ────────────────────────────────────────────────────────

static std::string extractDocComment(const std::string& content, size_t declarationLineIndex) { // internal helper
    std::vector<std::string> lines;
    std::stringstream ss(content);
    std::string l;
    while (std::getline(ss, l)) {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        lines.push_back(l);
    }

    if (declarationLineIndex >= lines.size()) return "";

    std::vector<std::string> docLines;
    int curr = (int)declarationLineIndex - 1;
    while (curr >= 0) {
        std::string lineStr = lines[curr];
        size_t firstNonSpace = lineStr.find_first_not_of(" \t");
        if (firstNonSpace != std::string::npos) {
            std::string trimmed = lineStr.substr(firstNonSpace);
            if (trimmed.rfind("//", 0) == 0) {
                std::string commentText = trimmed.substr(2);
                if (!commentText.empty() && commentText[0] == ' ') commentText = commentText.substr(1);
                docLines.push_back(commentText);
                curr--;
                continue;
            }
        }
        break;
    }

    if (docLines.empty()) return "";

    std::reverse(docLines.begin(), docLines.end());
    std::string result = "";
    for (size_t k = 0; k < docLines.size(); ++k) {
        if (k > 0) result += "\n";
        result += docLines[k];
    }
    return result;
}

std::vector<LSPSymbolInfo> extractDocumentSymbols(const std::string& content) {
    std::vector<LSPSymbolInfo> symbols;
    try {
        Lexer lexer(content);
        std::vector<Token> tokens = lexer.tokenizeAll();

        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& tok = tokens[i];
            if (tok.type == TokenType::KwFunction || tok.type == TokenType::KwAsync) {
                size_t idx = i;
                if (tok.type == TokenType::KwAsync && i + 1 < tokens.size() && tokens[i + 1].type == TokenType::KwFunction) {
                    idx = i + 1;
                }
                if (idx + 1 < tokens.size() && tokens[idx + 1].type == TokenType::Identifier) {
                    const auto& nameTok = tokens[idx + 1];
                    LSPSymbolInfo sym;
                    sym.name = nameTok.lexeme;
                    sym.kind = 3; // Function
                    sym.line = nameTok.line > 0 ? nameTok.line - 1 : 0;
                    sym.col = nameTok.column > 0 ? nameTok.column - 1 : 0;

                    std::string params = "(";
                    size_t p = idx + 2;
                    while (p < tokens.size() && tokens[p].type != TokenType::RParen && tokens[p].type != TokenType::LBrace && tokens[p].type != TokenType::Semicolon) {
                        params += tokens[p].lexeme;
                        if (tokens[p].type == TokenType::Comma) params += " ";
                        p++;
                    }
                    if (p < tokens.size() && tokens[p].type == TokenType::RParen) params += ")";

                    std::string retType = "";
                    if (p + 2 < tokens.size() && tokens[p + 1].type == TokenType::Colon) {
                        retType = ": " + tokens[p + 2].lexeme;
                    }

                    sym.detail = (tok.type == TokenType::KwAsync ? "async fn " : "fn ") + sym.name + params + retType;
                    std::string doc = extractDocComment(content, sym.line);
                    sym.documentation = doc.empty() ? ("Function defined at line " + std::to_string(nameTok.line)) : doc;
                    symbols.push_back(sym);
                }
            } else if (tok.type == TokenType::KwStruct) {
                if (i + 1 < tokens.size() && tokens[i + 1].type == TokenType::Identifier) {
                    const auto& nameTok = tokens[i + 1];
                    LSPSymbolInfo sym;
                    sym.name = nameTok.lexeme;
                    sym.kind = 22; // Struct
                    sym.line = nameTok.line > 0 ? nameTok.line - 1 : 0;
                    sym.col = nameTok.column > 0 ? nameTok.column - 1 : 0;
                    sym.detail = "struct " + sym.name;
                    std::string doc = extractDocComment(content, sym.line);
                    sym.documentation = doc.empty() ? ("Struct defined at line " + std::to_string(nameTok.line)) : doc;

                    size_t j = i + 2;
                    while (j < tokens.size() && tokens[j].type != TokenType::LBrace) j++;
                    if (j < tokens.size() && tokens[j].type == TokenType::LBrace) {
                        j++;
                        while (j + 2 < tokens.size() && tokens[j].type != TokenType::RBrace) {
                            if (tokens[j].type == TokenType::Identifier && tokens[j + 1].type == TokenType::Colon) {
                                sym.fields.push_back({tokens[j].lexeme, tokens[j + 2].lexeme});
                            }
                            j++;
                        }
                    }
                    symbols.push_back(sym);
                }
            } else if (tok.type == TokenType::KwEnum) {
                if (i + 1 < tokens.size() && tokens[i + 1].type == TokenType::Identifier) {
                    const auto& nameTok = tokens[i + 1];
                    LSPSymbolInfo sym;
                    sym.name = nameTok.lexeme;
                    sym.kind = 13; // Enum
                    sym.line = nameTok.line > 0 ? nameTok.line - 1 : 0;
                    sym.col = nameTok.column > 0 ? nameTok.column - 1 : 0;
                    sym.detail = "enum " + sym.name;
                    std::string doc = extractDocComment(content, sym.line);
                    sym.documentation = doc.empty() ? ("Enum defined at line " + std::to_string(nameTok.line)) : doc;
                    symbols.push_back(sym);
                }
            } else if (tok.type == TokenType::KwLet || tok.type == TokenType::KwConst) {
                if (i + 1 < tokens.size() && tokens[i + 1].type == TokenType::Identifier) {
                    const auto& nameTok = tokens[i + 1];
                    LSPSymbolInfo sym;
                    sym.name = nameTok.lexeme;
                    sym.kind = 6; // Variable
                    sym.line = nameTok.line > 0 ? nameTok.line - 1 : 0;
                    sym.col = nameTok.column > 0 ? nameTok.column - 1 : 0;
                    sym.detail = (tok.type == TokenType::KwConst ? "const " : "let ") + sym.name;
                    if (i + 3 < tokens.size() && tokens[i + 2].type == TokenType::Colon && tokens[i + 3].type == TokenType::Identifier) {
                        sym.typeName = tokens[i + 3].lexeme;
                        sym.detail += ": " + sym.typeName;
                    }
                    std::string doc = extractDocComment(content, sym.line);
                    sym.documentation = doc.empty() ? ("Variable defined at line " + std::to_string(nameTok.line)) : doc;
                    symbols.push_back(sym);
                }
            }
        }
    } catch (...) {}
    return symbols;
}

std::string escapeJson(const std::string& str) {
    std::stringstream ss;
    for (char c : str) {
        if (c == '"') ss << "\\\"";
        else if (c == '\\') ss << "\\\\";
        else if (c == '\n') ss << "\\n";
        else if (c == '\r') ss << "\\r";
        else if (c == '\t') ss << "\\t";
        else ss << c;
    }
    return ss.str();
}

// ─── LSPServer core ───────────────────────────────────────────────────────────

LSPServer::LSPServer() : m_running(true) {}

void LSPServer::run() {
    std::string line;
    while (m_running && std::cin.good()) {
        int contentLength = 0;
        while (std::getline(std::cin, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                break;
            }
            if (line.rfind("Content-Length: ", 0) == 0) {
                contentLength = std::stoi(line.substr(16));
            }
        }

        if (contentLength <= 0) {
            continue;
        }

        std::vector<char> buffer(contentLength);
        std::cin.read(buffer.data(), contentLength);
        std::string rawJson(buffer.data(), contentLength);

        handleMessage(rawJson);
    }
}

std::string LSPServer::extractJsonField(const std::string& json, const std::string& fieldName) {
    std::string key = "\"" + fieldName + "\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos) return "";

    size_t colon = json.find(":", pos + key.length());
    if (colon == std::string::npos) return "";

    size_t start = json.find_first_not_of(" \t\r\n", colon + 1);
    if (start == std::string::npos) return "";

    if (json[start] == '"') {
        size_t end = json.find('"', start + 1);
        if (end != std::string::npos) {
            return json.substr(start + 1, end - start - 1);
        }
    } else {
        size_t end = json.find_first_of(",}\r\n", start);
        if (end != std::string::npos) {
            return json.substr(start, end - start);
        }
    }
    return "";
}

void LSPServer::sendResponse(const std::string& id, const std::string& resultJson) {
    std::stringstream body;
    body << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":" << resultJson << "}";
    std::string str = body.str();

    std::cout << "Content-Length: " << str.length() << "\r\n\r\n" << str << std::flush;
}

void LSPServer::sendNotification(const std::string& method, const std::string& paramsJson) {
    std::stringstream body;
    body << "{\"jsonrpc\":\"2.0\",\"method\":\"" << method << "\",\"params\":" << paramsJson << "}";
    std::string str = body.str();

    std::cout << "Content-Length: " << str.length() << "\r\n\r\n" << str << std::flush;
}

void LSPServer::handleMessage(const std::string& rawJson) {
    std::string id = extractJsonField(rawJson, "id");
    std::string method = extractJsonField(rawJson, "method");

    if (method == "initialize") {
        handleInitialize(id.empty() ? "1" : id);
    } else if (method == "textDocument/didOpen") {
        std::string uri = extractJsonField(rawJson, "uri");
        std::string text = extractJsonField(rawJson, "text");
        handleDidOpen(uri, text);
    } else if (method == "textDocument/didChange") {
        std::string uri = extractJsonField(rawJson, "uri");
        std::string text = extractJsonField(rawJson, "text");
        handleDidChange(uri, text);
    } else if (method == "textDocument/hover") {
        std::string uri = extractJsonField(rawJson, "uri");
        std::string lineStr = extractJsonField(rawJson, "line");
        std::string charStr = extractJsonField(rawJson, "character");
        int line = lineStr.empty() ? 0 : std::stoi(lineStr);
        int character = charStr.empty() ? 0 : std::stoi(charStr);
        handleHover(id.empty() ? "1" : id, uri, line, character);
    } else if (method == "textDocument/completion") {
        std::string uri = extractJsonField(rawJson, "uri");
        std::string lineStr = extractJsonField(rawJson, "line");
        std::string charStr = extractJsonField(rawJson, "character");
        int line = lineStr.empty() ? 0 : std::stoi(lineStr);
        int character = charStr.empty() ? 0 : std::stoi(charStr);
        handleCompletion(id.empty() ? "1" : id, uri, line, character);
    } else if (method == "textDocument/definition") {
        std::string uri = extractJsonField(rawJson, "uri");
        std::string lineStr = extractJsonField(rawJson, "line");
        std::string charStr = extractJsonField(rawJson, "character");
        int line = lineStr.empty() ? 0 : std::stoi(lineStr);
        int character = charStr.empty() ? 0 : std::stoi(charStr);
        handleDefinition(id.empty() ? "1" : id, uri, line, character);
    } else if (method == "textDocument/formatting") {
        std::string uri = extractJsonField(rawJson, "uri");
        handleFormatting(id.empty() ? "1" : id, uri);
    } else if (method == "textDocument/signatureHelp") {
        std::string uri = extractJsonField(rawJson, "uri");
        std::string lineStr = extractJsonField(rawJson, "line");
        std::string charStr = extractJsonField(rawJson, "character");
        int line = lineStr.empty() ? 0 : std::stoi(lineStr);
        int character = charStr.empty() ? 0 : std::stoi(charStr);
        handleSignatureHelp(id.empty() ? "1" : id, uri, line, character);
    } else if (method == "textDocument/inlayHint") {
        std::string uri = extractJsonField(rawJson, "uri");
        handleInlayHint(id.empty() ? "1" : id, uri);
    } else if (method == "textDocument/rename") {
        std::string uri = extractJsonField(rawJson, "uri");
        std::string lineStr = extractJsonField(rawJson, "line");
        std::string charStr = extractJsonField(rawJson, "character");
        std::string newName = extractJsonField(rawJson, "newName");
        int line = lineStr.empty() ? 0 : std::stoi(lineStr);
        int character = charStr.empty() ? 0 : std::stoi(charStr);
        handleRename(id.empty() ? "1" : id, uri, line, character, newName);
    } else if (method == "textDocument/codeAction") {
        std::string uri = extractJsonField(rawJson, "uri");
        handleCodeAction(id.empty() ? "1" : id, uri);
    } else if (method == "textDocument/codeLens") {
        std::string uri = extractJsonField(rawJson, "uri");
        handleCodeLens(id.empty() ? "1" : id, uri);
    } else if (method == "shutdown") {
        sendResponse(id.empty() ? "1" : id, "null");
    } else if (method == "exit") {
        m_running = false;
    }
}

void LSPServer::handleInitialize(const std::string& id) {
    std::string capabilities = "{"
        "\"capabilities\":{"
            "\"textDocumentSync\":1,"
            "\"hoverProvider\":true,"
            "\"completionProvider\":{\"triggerCharacters\":[\".\",\":\"]},"
            "\"definitionProvider\":true,"
            "\"documentFormattingProvider\":true,"
            "\"signatureHelpProvider\":{\"triggerCharacters\":[\"(\",\",\"]},"
            "\"inlayHintProvider\":true,"
            "\"renameProvider\":true,"
            "\"codeActionProvider\":true,"
            "\"codeLensProvider\":{\"resolveProvider\":false}"
        "}"
    "}";
    sendResponse(id, capabilities);
}

void LSPServer::handleDidOpen(const std::string& uri, const std::string& text) {
    m_documents[uri] = text;
    publishDiagnostics(uri, text);
}

void LSPServer::handleDidChange(const std::string& uri, const std::string& text) {
    m_documents[uri] = text;
    publishDiagnostics(uri, text);
}

void LSPServer::publishDiagnostics(const std::string& uri, const std::string& content) {
    std::string diagList = "[]";
    try {
        Lexer lexer(content);
        Parser parser(std::move(lexer));
        auto programAST = parser.parseProgram();

        SemanticAnalyzer analyzer;
        if (!analyzer.analyze(programAST.get())) {
            std::stringstream ss;
            ss << "[";
            bool first = true;
            for (const auto& err : analyzer.getErrors()) {
                if (!first) ss << ",";
                first = false;
                ss << "{"
                   << "\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":0,\"character\":10}},"
                   << "\"severity\":1,"
                   << "\"message\":\"" << escapeJson(err) << "\""
                   << "}";
            }
            ss << "]";
            diagList = ss.str();
        }
    } catch (const ParseError& e) {
        std::stringstream ss;
        int line = std::max(0, (int)e.line - 1);
        int col = std::max(0, (int)e.column - 1);
        ss << "[{"
           << "\"range\":{\"start\":{\"line\":" << line << ",\"character\":" << col << "},\"end\":{\"line\":" << line << ",\"character\":" << (col + 5) << "}},"
           << "\"severity\":1,"
           << "\"message\":\"Parse Error: " << escapeJson(e.what()) << "\""
           << "}]";
        diagList = ss.str();
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "[{"
           << "\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":0,\"character\":5}},"
           << "\"severity\":1,"
           << "\"message\":\"" << escapeJson(e.what()) << "\""
           << "}]";
        diagList = ss.str();
    }

    std::stringstream params;
    params << "{\"uri\":\"" << uri << "\",\"diagnostics\":" << diagList << "}";
    sendNotification("textDocument/publishDiagnostics", params.str());
}

// ─── LSP request handlers (hover, completion, etc.) → LSP_Handlers.cpp ──────

} // namespace vit
