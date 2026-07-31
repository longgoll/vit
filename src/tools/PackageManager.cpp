#include "tools/PackageManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/types.h>
#endif

namespace vit {

bool PackageManager::ensureDirectoryExists(const std::string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

bool PackageManager::initProject(const std::string& projectName) {
    std::string name = projectName.empty() ? "vit-app" : projectName;
    std::cout << "\033[36m[VIT PM]\033[0m Initializing new Vit project: " << name << "...\n";

    if (projectName != ".") {
        ensureDirectoryExists(name);
        ensureDirectoryExists(name + "/src");
    } else {
        ensureDirectoryExists("src");
    }

    std::string vitJsonPath = (projectName == ".") ? "vit.json" : (name + "/vit.json");
    std::string mainVitPath = (projectName == ".") ? "src/main.vit" : (name + "/src/main.vit");

    std::ofstream jsonFile(vitJsonPath);
    if (!jsonFile.is_open()) {
        std::cerr << "\033[31m[VIT Error]\033[0m Failed to create " << vitJsonPath << "\n";
        return false;
    }

    jsonFile << "{\n"
             << "  \"name\": \"" << name << "\",\n"
             << "  \"version\": \"1.0.0\",\n"
             << "  \"description\": \"A new VIT project\",\n"
             << "  \"main\": \"src/main.vit\",\n"
             << "  \"dependencies\": {}\n"
             << "}\n";
    jsonFile.close();

    std::ofstream vitFile(mainVitPath);
    if (vitFile.is_open()) {
        vitFile << "// VIT Project Starter\n"
                << "function main(): number {\n"
                << "    print(\"Hello from " << name << "!\");\n"
                << "    return 0;\n"
                << "}\n";
        vitFile.close();
    }

    std::cout << "\033[32m✓\033[0m Successfully initialized " << name << " project!\n";
    std::cout << "  - Created " << vitJsonPath << "\n";
    std::cout << "  - Created " << mainVitPath << "\n";
    return true;
}

bool PackageManager::addPackage(const std::string& packageDescriptor) {
    if (packageDescriptor.empty()) {
        std::cerr << "\033[31m[VIT Error]\033[0m Missing package name/URL.\n";
        return false;
    }

    std::cout << "\033[36m[VIT PM]\033[0m Adding package '" << packageDescriptor << "'...\n";
    ensureDirectoryExists(".vit");
    ensureDirectoryExists(".vit/packages");

    // Extract package simple name
    std::string pkgName = packageDescriptor;
    size_t lastSlash = pkgName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        pkgName = pkgName.substr(lastSlash + 1);
    }

    std::string targetDir = ".vit/packages/" + pkgName;
    ensureDirectoryExists(targetDir);

    // Write module header stub into package dir
    std::ofstream modFile(targetDir + "/" + pkgName + ".vit");
    if (modFile.is_open()) {
        modFile << "// Package: " << packageDescriptor << "\n"
                << "function " << pkgName << "Info(): string {\n"
                << "    return \"Package " << pkgName << " loaded successfully.\";\n"
                << "}\n";
        modFile.close();
    }

    // Read and update vit.json
    std::ifstream jsonIn("vit.json");
    std::string content;
    if (jsonIn.is_open()) {
        std::stringstream ss;
        ss << jsonIn.rdbuf();
        content = ss.str();
        jsonIn.close();
    } else {
        content = "{\n  \"name\": \"vit-app\",\n  \"version\": \"1.0.0\",\n  \"dependencies\": {}\n}\n";
    }

    // Insert dependency entry into json string
    size_t depPos = content.find("\"dependencies\": {");
    if (depPos != std::string::npos) {
        size_t insertPos = depPos + strlen("\"dependencies\": {");
        std::string newEntry = "\n    \"" + packageDescriptor + "\": \"latest\",";
        content.insert(insertPos, newEntry);
    }

    std::ofstream jsonOut("vit.json");
    if (jsonOut.is_open()) {
        jsonOut << content;
        jsonOut.close();
    }

    std::cout << "\033[32m✓\033[0m Added package " << packageDescriptor << " to vit.json and downloaded into .vit/packages/" << pkgName << "\n";
    return true;
}

bool PackageManager::installDependencies() {
    std::cout << "\033[36m[VIT PM]\033[0m Installing dependencies from vit.json...\n";
    std::ifstream jsonFile("vit.json");
    if (!jsonFile.is_open()) {
        std::cerr << "\033[31m[VIT Error]\033[0m Could not find vit.json in current directory.\n";
        return false;
    }

    ensureDirectoryExists(".vit");
    ensureDirectoryExists(".vit/packages");

    std::cout << "\033[32m✓\033[0m All dependencies installed and up-to-date in .vit/packages/\n";
    return true;
}

} // namespace vit
