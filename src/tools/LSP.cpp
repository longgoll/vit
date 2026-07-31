#include "tools/LSP.h"
#include "diagnostics/DiagnosticPrinter.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace vit {

LSPServer::LSPServer() : m_running(true) {}

void LSPServer::run() {
    std::string line;
    while (m_running && std::cin.good()) {
        int contentLength = 0;
        while (std::getline(std::cin, line)) {
            // Trim trailing \r if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                break; // End of HTTP headers
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
            "\"definitionProvider\":true"
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
                   << "\"message\":\"" << err << "\""
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
           << "\"message\":\"Parse Error: " << e.what() << "\""
           << "}]";
        diagList = ss.str();
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "[{"
           << "\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":0,\"character\":5}},"
           << "\"severity\":1,"
           << "\"message\":\"" << e.what() << "\""
           << "}]";
        diagList = ss.str();
    }

    std::stringstream params;
    params << "{\"uri\":\"" << uri << "\",\"diagnostics\":" << diagList << "}";
    sendNotification("textDocument/publishDiagnostics", params.str());
}

void LSPServer::handleHover(const std::string& id, const std::string& uri, int line, int character) {
    (void)line; (void)character;
    std::string doc = m_documents[uri];
    std::string hoverResult = "{\"contents\":{\"kind\":\"markdown\",\"value\":\"**VIT Symbol**\\n```vit\\n// Symbol Hover Info\\n```\"}}";
    sendResponse(id, hoverResult);
}

void LSPServer::handleCompletion(const std::string& id, const std::string& uri, int line, int character) {
    (void)uri; (void)line; (void)character;
    std::string completionResult = "{\"isIncomplete\":false,\"items\":["
        "{\"label\":\"let\",\"kind\":14,\"detail\":\"Keyword let\"},"
        "{\"label\":\"fn\",\"kind\":14,\"detail\":\"Keyword function\"},"
        "{\"label\":\"async\",\"kind\":14,\"detail\":\"Keyword async\"},"
        "{\"label\":\"await\",\"kind\":14,\"detail\":\"Keyword await\"},"
        "{\"label\":\"print\",\"kind\":3,\"detail\":\"fn print(val: String): void\"},"
        "{\"label\":\"println\",\"kind\":3,\"detail\":\"fn println(val: String): void\"},"
        "{\"label\":\"struct\",\"kind\":14,\"detail\":\"Keyword struct\"},"
        "{\"label\":\"class\",\"kind\":14,\"detail\":\"Keyword class\"},"
        "{\"label\":\"import\",\"kind\":14,\"detail\":\"Keyword import\"}"
    "]}";
    sendResponse(id, completionResult);
}

void LSPServer::handleDefinition(const std::string& id, const std::string& uri, int line, int character) {
    (void)line; (void)character;
    std::stringstream defResult;
    defResult << "{\"uri\":\"" << uri << "\",\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":0,\"character\":10}}}";
    sendResponse(id, defResult.str());
}

} // namespace vit
