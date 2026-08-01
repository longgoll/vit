#ifndef VIT_TOOLS_LSP_H
#define VIT_TOOLS_LSP_H

#include "ast/ASTNode.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantics/SemanticAnalyzer.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace vit {

class LSPServer {
public:
    LSPServer();
    void run();

private:
    bool m_running;
    std::unordered_map<std::string, std::string> m_documents;

    void handleMessage(const std::string& rawJson);
    void sendResponse(const std::string& id, const std::string& resultJson);
    void sendNotification(const std::string& method, const std::string& paramsJson);
    
    void handleInitialize(const std::string& id);
    void handleDidOpen(const std::string& uri, const std::string& text);
    void handleDidChange(const std::string& uri, const std::string& text);
    void handleHover(const std::string& id, const std::string& uri, int line, int character);
    void handleCompletion(const std::string& id, const std::string& uri, int line, int character);
    void handleDefinition(const std::string& id, const std::string& uri, int line, int character);
    void handleFormatting(const std::string& id, const std::string& uri);
    void handleSignatureHelp(const std::string& id, const std::string& uri, int line, int character);
    void handleInlayHint(const std::string& id, const std::string& uri);
    void handleRename(const std::string& id, const std::string& uri, int line, int character, const std::string& newName);
    void handleCodeAction(const std::string& id, const std::string& uri);
    void handleCodeLens(const std::string& id, const std::string& uri);
    void publishDiagnostics(const std::string& uri, const std::string& content);

    std::string extractJsonField(const std::string& json, const std::string& fieldName);
};

} // namespace vit

#endif // VIT_TOOLS_LSP_H
