#ifndef VIT_TOOLS_REPL_H
#define VIT_TOOLS_REPL_H

#include <string>
#include <vector>

namespace vit {

class REPLEngine {
public:
    REPLEngine();
    void run();

private:
    bool m_showAST;
    std::vector<std::string> m_history;
    
    void printHeader();
    void evalLine(const std::string& line);
};

} // namespace vit

#endif // VIT_TOOLS_REPL_H
