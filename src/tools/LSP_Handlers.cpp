#include "tools/LSP.h"
#include "tools/LSP_Internal.h"
#include "tools/Formatter.h"
#include "lexer/Lexer.h"

#include <sstream>
#include <algorithm>
#include <vector>
#include <string>

namespace vit {

// ─── Position utilities ──────────────────────────────────────────────────────


static std::string getWordAtPosition(const std::string& content, int line, int character) {
    std::stringstream ss(content);
    std::string l;
    int curLine = 0;
    while (std::getline(ss, l)) {
        if (curLine == line) {
            if (!l.empty() && l.back() == '\r') l.pop_back();
            if (character < 0 || character >= (int)l.length()) {
                if (character > 0 && character <= (int)l.length()) {
                    character = (int)l.length() - 1;
                } else {
                    return "";
                }
            }
            int start = character;
            while (start > 0 && (isalnum((unsigned char)l[start - 1]) || l[start - 1] == '_')) {
                start--;
            }
            int end = character;
            while (end < (int)l.length() && (isalnum((unsigned char)l[end]) || l[end] == '_')) {
                end++;
            }
            if (start < end) {
                return l.substr(start, end - start);
            }
            return "";
        }
        curLine++;
    }
    return "";
}

static std::string getLinePrefix(const std::string& content, int line, int character) {
    std::stringstream ss(content);
    std::string l;
    int curLine = 0;
    while (std::getline(ss, l)) {
        if (curLine == line) {
            if (!l.empty() && l.back() == '\r') l.pop_back();
            if (character > 0 && character <= (int)l.length()) {
                return l.substr(0, character);
            }
            return l;
        }
        curLine++;
    }
    return "";
}

// ─── Request handlers ────────────────────────────────────────────────────────

void LSPServer::handleHover(const std::string& id, const std::string& uri, int line, int character) {
    std::string content = m_documents[uri];
    std::string word = getWordAtPosition(content, line, character);
    auto symbols = extractDocumentSymbols(content);

    const LSPSymbolInfo* matched = nullptr;
    if (!word.empty()) {
        for (const auto& sym : symbols) {
            if (sym.name == word) {
                matched = &sym;
                break;
            }
        }
    }

    std::string hoverResult;
    if (matched) {
        std::stringstream ss;
        ss << "{\"contents\":{\"kind\":\"markdown\",\"value\":\"```vit\\n"
           << escapeJson(matched->detail) << "\\n```\\n\\n"
           << escapeJson(matched->documentation) << "\"}}";
        hoverResult = ss.str();
    } else if (!word.empty()) {
        hoverResult = "{\"contents\":{\"kind\":\"markdown\",\"value\":\"**vit symbol**: `" + escapeJson(word) + "`\"}}";
    } else {
        hoverResult = "{\"contents\":{\"kind\":\"markdown\",\"value\":\"**Vit Code Region**\\nLine " + std::to_string(line + 1) + ", Col " + std::to_string(character + 1) + "\"}}";
    }

    sendResponse(id, hoverResult);
}

void LSPServer::handleCompletion(const std::string& id, const std::string& uri, int line, int character) {
    std::string content = m_documents[uri];
    auto symbols = extractDocumentSymbols(content);
    std::string prefix = getLinePrefix(content, line, character);

    std::stringstream ss;
    ss << "{\"isIncomplete\":false,\"items\":[";
    bool first = true;

    auto addItem = [&](const std::string& label, int kind, const std::string& detail, const std::string& insertText = "") {
        if (!first) ss << ",";
        first = false;
        ss << "{\"label\":\"" << escapeJson(label)
           << "\",\"kind\":" << kind
           << ",\"detail\":\"" << escapeJson(detail) << "\"";
        if (!insertText.empty()) {
            ss << ",\"insertText\":\"" << escapeJson(insertText) << "\",\"insertTextFormat\":2";
        }
        ss << "}";
    };

    if (prefix.rfind("math.", prefix.length() - 5) != std::string::npos || prefix.ends_with("math.")) {
        addItem("sqrt",   3, "fn sqrt(x: number): number",                           "sqrt(${1:x})");
        addItem("cos",    3, "fn cos(x: number): number",                             "cos(${1:x})");
        addItem("sin",    3, "fn sin(x: number): number",                             "sin(${1:x})");
        addItem("tan",    3, "fn tan(x: number): number",                             "tan(${1:x})");
        addItem("abs",    3, "fn abs(x: number): number",                             "abs(${1:x})");
        addItem("pow",    3, "fn pow(x: number, y: number): number",                  "pow(${1:x}, ${2:y})");
        addItem("floor",  3, "fn floor(x: number): number",                           "floor(${1:x})");
        addItem("ceil",   3, "fn ceil(x: number): number",                            "ceil(${1:x})");
        addItem("round",  3, "fn round(x: number): number",                           "round(${1:x})");
        addItem("log",    3, "fn log(x: number): number",                             "log(${1:x})");
        addItem("exp",    3, "fn exp(x: number): number",                             "exp(${1:x})");
        addItem("min",    3, "fn min(a: number, b: number): number",                  "min(${1:a}, ${2:b})");
        addItem("max",    3, "fn max(a: number, b: number): number",                  "max(${1:a}, ${2:b})");
        addItem("clamp",  3, "fn clamp(val: number, min: number, max: number): number","clamp(${1:val}, ${2:min}, ${3:max})");
        addItem("random", 3, "fn random(): number",                                   "random()");
        ss << "]}";
        sendResponse(id, ss.str());
        return;
    }

    if (prefix.rfind("fs.", prefix.length() - 3) != std::string::npos || prefix.ends_with("fs.")) {
        addItem("readFile",   3, "fn readFile(path: string): string",                    "readFile(${1:path})");
        addItem("writeFile",  3, "fn writeFile(path: string, content: string): boolean", "writeFile(${1:path}, ${2:content})");
        addItem("appendFile", 3, "fn appendFile(path: string, content: string): boolean","appendFile(${1:path}, ${2:content})");
        addItem("exists",     3, "fn exists(path: string): boolean",                     "exists(${1:path})");
        addItem("removeFile", 3, "fn removeFile(path: string): boolean",                 "removeFile(${1:path})");
        addItem("fileSize",   3, "fn fileSize(path: string): number",                    "fileSize(${1:path})");
        ss << "]}";
        sendResponse(id, ss.str());
        return;
    }

    if (prefix.rfind("str.", prefix.length() - 4) != std::string::npos ||
        prefix.rfind("string.", prefix.length() - 7) != std::string::npos ||
        prefix.ends_with("str.") || prefix.ends_with("string.")) {
        addItem("length",     3, "fn length(str: string): number",                                         "length(${1:str})");
        addItem("charAt",     3, "fn charAt(str: string, index: number): string",                           "charAt(${1:str}, ${2:index})");
        addItem("indexOf",    3, "fn indexOf(str: string, sub: string): number",                            "indexOf(${1:str}, ${2:sub})");
        addItem("substring",  3, "fn substring(str: string, start: number, len: number): string",           "substring(${1:str}, ${2:start}, ${3:len})");
        addItem("startsWith", 3, "fn startsWith(str: string, prefix: string): boolean",                     "startsWith(${1:str}, ${2:prefix})");
        addItem("endsWith",   3, "fn endsWith(str: string, suffix: string): boolean",                       "endsWith(${1:str}, ${2:suffix})");
        addItem("trim",       3, "fn trim(str: string): string",                                            "trim(${1:str})");
        addItem("replace",    3, "fn replace(str: string, target: string, repl: string): string",           "replace(${1:str}, ${2:target}, ${3:repl})");
        ss << "]}";
        sendResponse(id, ss.str());
        return;
    }

    static const std::vector<std::string> keywords = {
        "let", "const", "fn", "async", "await", "if", "else", "while", "for",
        "return", "break", "continue", "struct", "enum", "match", "try", "catch",
        "import", "export", "from"
    };
    for (const auto& kw : keywords) {
        addItem(kw, 14, "Keyword " + kw);
    }

    addItem("print",     3, "fn print(val: any): void",                          "print(${1:val})");
    addItem("println",   3, "fn println(val: any): void",                        "println(${1:val})");
    addItem("len",       3, "fn len(arr: Array): number",                        "len(${1:arr})");
    addItem("readFile",  3, "fn readFile(path: string): string",                 "readFile(${1:path})");
    addItem("writeFile", 3, "fn writeFile(path: string, content: string): boolean","writeFile(${1:path}, ${2:content})");
    addItem("exists",    3, "fn exists(path: string): boolean",                  "exists(${1:path})");
    addItem("trim",      3, "fn trim(str: string): string",                      "trim(${1:str})");
    addItem("replace",   3, "fn replace(str: string, target: string, repl: string): string","replace(${1:str}, ${2:target}, ${3:repl})");
    addItem("sqrt",      3, "fn sqrt(x: number): number",                        "sqrt(${1:x})");
    addItem("min",       3, "fn min(a: number, b: number): number",              "min(${1:a}, ${2:b})");
    addItem("max",       3, "fn max(a: number, b: number): number",              "max(${1:a}, ${2:b})");

    for (const auto& sym : symbols) {
        addItem(sym.name, sym.kind, sym.detail);
        for (const auto& f : sym.fields) {
            addItem(f.first, 5, "field " + f.first + ": " + f.second);
        }
    }

    ss << "]}";
    sendResponse(id, ss.str());
}

void LSPServer::handleDefinition(const std::string& id, const std::string& uri, int line, int character) {
    std::string content = m_documents[uri];
    std::string word = getWordAtPosition(content, line, character);
    auto symbols = extractDocumentSymbols(content);

    size_t targetLine = 0;
    size_t targetCol = 0;
    size_t len = 5;

    if (!word.empty()) {
        for (const auto& sym : symbols) {
            if (sym.name == word) {
                targetLine = sym.line;
                targetCol = sym.col;
                len = sym.name.length();
                break;
            }
        }
    }

    std::stringstream defResult;
    defResult << "{\"uri\":\"" << escapeJson(uri)
              << "\",\"range\":{\"start\":{\"line\":" << targetLine << ",\"character\":" << targetCol
              << "},\"end\":{\"line\":" << targetLine << ",\"character\":" << (targetCol + len) << "}}}";
    sendResponse(id, defResult.str());
}

void LSPServer::handleFormatting(const std::string& id, const std::string& uri) {
    if (m_documents.find(uri) == m_documents.end()) {
        sendResponse(id, "[]");
        return;
    }

    std::string text = m_documents[uri];
    std::string formatted = Formatter::formatCode(text);

    int lineCount = 0;
    size_t lastLineLen = 0;
    for (char c : text) {
        if (c == '\n') {
            lineCount++;
            lastLineLen = 0;
        } else {
            lastLineLen++;
        }
    }

    std::stringstream res;
    res << "[{\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":"
        << lineCount << ",\"character\":" << lastLineLen
        << "}},\"newText\":\"" << escapeJson(formatted) << "\"}]";

    sendResponse(id, res.str());
}

void LSPServer::handleSignatureHelp(const std::string& id, const std::string& uri, int line, int character) {
    (void)line; (void)character;
    std::string content = m_documents[uri];
    auto symbols = extractDocumentSymbols(content);

    std::stringstream ss;
    ss << "{\"signatures\":[";
    bool first = true;
    for (const auto& sym : symbols) {
        if (sym.kind == 3) { // Function
            if (!first) ss << ",";
            first = false;
            ss << "{\"label\":\"" << escapeJson(sym.detail)
               << "\",\"documentation\":\"" << escapeJson(sym.documentation)
               << "\",\"parameters\":[{\"label\":\"val: any\"}]}";
        }
    }
    if (first) {
        ss << "{\"label\":\"fn print(val: String): void\",\"documentation\":\"Print value to output\",\"parameters\":[{\"label\":\"val: String\"}]}";
    }
    ss << "],\"activeSignature\":0,\"activeParameter\":0}";
    sendResponse(id, ss.str());
}

void LSPServer::handleInlayHint(const std::string& id, const std::string& uri) {
    std::string content = m_documents[uri];
    auto symbols = extractDocumentSymbols(content);

    std::stringstream ss;
    ss << "[";
    bool first = true;
    for (const auto& sym : symbols) {
        if (sym.kind == 6) { // Variable
            if (!first) ss << ",";
            first = false;
            ss << "{\"position\":{\"line\":" << sym.line << ",\"character\":" << (sym.col + sym.name.length())
               << "},\"label\":\": " << (sym.typeName.empty() ? "any" : sym.typeName)
               << "\",\"kind\":1,\"paddingLeft\":true}";
        }
    }
    ss << "]";
    sendResponse(id, ss.str());
}

void LSPServer::handleRename(const std::string& id, const std::string& uri, int line, int character, const std::string& newName) {
    std::string content = m_documents[uri];
    auto symbols = extractDocumentSymbols(content);

    std::string targetName = "";
    for (const auto& sym : symbols) {
        if ((int)sym.line == line) {
            targetName = sym.name;
            break;
        }
    }

    if (targetName.empty()) {
        sendResponse(id, "null");
        return;
    }

    std::stringstream ss;
    ss << "{\"changes\":{\"" << escapeJson(uri) << "\":[";
    bool first = true;

    try {
        Lexer lexer(content);
        std::vector<Token> tokens = lexer.tokenizeAll();
        for (const auto& tok : tokens) {
            if (tok.lexeme == targetName) {
                if (!first) ss << ",";
                first = false;
                size_t l = tok.line > 0 ? tok.line - 1 : 0;
                size_t c = tok.column > 0 ? tok.column - 1 : 0;
                ss << "{\"range\":{\"start\":{\"line\":" << l << ",\"character\":" << c
                   << "},\"end\":{\"line\":" << l << ",\"character\":" << (c + tok.lexeme.length())
                   << "}},\"newText\":\"" << escapeJson(newName) << "\"}";
            }
        }
    } catch (...) {}

    ss << "]}}";
    sendResponse(id, ss.str());
}

void LSPServer::handleCodeAction(const std::string& id, const std::string& uri) {
    (void)uri;
    std::string result = "["
        "{\"title\":\"⚡ Format Vit Document\",\"kind\":\"quickfix\",\"command\":{\"title\":\"Format\",\"command\":\"editor.action.formatDocument\"}},"
        "{\"title\":\"▶ Run Vit File\",\"kind\":\"quickfix\",\"command\":{\"title\":\"Run\",\"command\":\"vit.runFile\"}}"
    "]";
    sendResponse(id, result);
}

void LSPServer::handleCodeLens(const std::string& id, const std::string& uri) {
    std::string content = m_documents[uri];
    std::stringstream ss;
    ss << "[";

    size_t mainLine = 0;
    bool foundMain = false;
    try {
        Lexer lexer(content);
        std::vector<Token> tokens = lexer.tokenizeAll();
        for (size_t i = 0; i < tokens.size(); ++i) {
            if ((tokens[i].type == TokenType::KwFunction || tokens[i].lexeme == "function" || tokens[i].lexeme == "fn") &&
                i + 1 < tokens.size() && tokens[i + 1].lexeme == "main") {
                mainLine = tokens[i].line > 0 ? tokens[i].line - 1 : 0;
                foundMain = true;
                break;
            }
        }
    } catch (...) {}

    if (foundMain) {
        ss << "{\"range\":{\"start\":{\"line\":" << mainLine << ",\"character\":0},\"end\":{\"line\":" << mainLine << ",\"character\":10}},"
           << "\"command\":{\"title\":\"▶ Run Vit File\",\"command\":\"vit.runFile\"}}";
    }

    ss << "]";
    sendResponse(id, ss.str());
}

} // namespace vit
