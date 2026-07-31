#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {

// ==========================================
// 1. CLI Arguments & System Environment
// ==========================================

static int g_argc = 0;
static char** g_argv = nullptr;

void __vit_init_args(int argc, char** argv) {
    g_argc = argc;
    g_argv = argv;
}

double __vit_get_arg_count() {
    return (double)g_argc;
}

const char* __vit_get_arg(double index) {
    int idx = (int)index;
    if (idx >= 0 && idx < g_argc && g_argv && g_argv[idx]) {
        return g_argv[idx];
    }
    return "";
}

const char* __vit_get_env(const char* name) {
    if (!name) return nullptr;
    const char* val = std::getenv(name);
    return val;
}

// ==========================================
// 2. HashMap Runtime Backing (std::unordered_map)
// ==========================================

struct VitHashMap {
    std::unordered_map<std::string, std::string> map;
};

void* __vit_hashmap_create() {
    return new VitHashMap();
}

void __vit_hashmap_set(void* handle, const char* key, const char* val) {
    if (!handle || !key || !val) return;
    auto* hm = static_cast<VitHashMap*>(handle);
    hm->map[std::string(key)] = std::string(val);
}

const char* __vit_hashmap_get(void* handle, const char* key) {
    if (!handle || !key) return "";
    auto* hm = static_cast<VitHashMap*>(handle);
    auto it = hm->map.find(std::string(key));
    if (it != hm->map.end()) {
        // Return duplicate string or persistent pointer in std::string
        return it->second.c_str();
    }
    return "";
}

double __vit_hashmap_has(void* handle, const char* key) {
    if (!handle || !key) return 0.0;
    auto* hm = static_cast<VitHashMap*>(handle);
    return (hm->map.find(std::string(key)) != hm->map.end()) ? 1.0 : 0.0;
}

void __vit_hashmap_remove(void* handle, const char* key) {
    if (!handle || !key) return;
    auto* hm = static_cast<VitHashMap*>(handle);
    hm->map.erase(std::string(key));
}

double __vit_hashmap_size(void* handle) {
    if (!handle) return 0.0;
    auto* hm = static_cast<VitHashMap*>(handle);
    return (double)hm->map.size();
}

void __vit_hashmap_free(void* handle) {
    if (!handle) return;
    auto* hm = static_cast<VitHashMap*>(handle);
    delete hm;
}

// ==========================================
// 3. JSON Helper Utilities
// ==========================================

const char* __vit_json_escape_string(const char* str) {
    if (!str) return "\"\"";
    std::string s(str);
    std::string res = "\"";
    for (char c : s) {
        if (c == '"') res += "\\\"";
        else if (c == '\\') res += "\\\\";
        else if (c == '\b') res += "\\b";
        else if (c == '\f') res += "\\f";
        else if (c == '\n') res += "\\n";
        else if (c == '\r') res += "\\r";
        else if (c == '\t') res += "\\t";
        else res += c;
    }
    res += "\"";
    char* out = (char*)malloc(res.size() + 1);
    std::strcpy(out, res.c_str());
    return out;
}

}
