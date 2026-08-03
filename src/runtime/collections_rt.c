// collections_rt.c - Standalone C runtime for Vit Compiler (Zero header dependencies)

typedef unsigned long long size_t;

#define NULL ((void*)0)

void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);
int strcmp(const char* s1, const char* s2);
size_t strlen(const char* s);
char* getenv(const char* name);

static char* vit_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* new_str = (char*)malloc(len + 1);
    if (!new_str) return NULL;
    for (size_t i = 0; i <= len; i++) {
        new_str[i] = s[i];
    }
    return new_str;
}

// ==========================================
// 1. CLI Arguments & System Environment
// ==========================================

static int g_argc = 0;
static char** g_argv = NULL;

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
    if (!name) return NULL;
    return getenv(name);
}

// ==========================================
// 2. Pure C Hash Map Implementation
// ==========================================

typedef struct {
    char* key;
    char* value;
} HashEntry;

typedef struct {
    HashEntry* entries;
    int capacity;
    int count;
} VitHashMap;

static unsigned int hash_string(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

void* __vit_hashmap_create() {
    VitHashMap* map = (VitHashMap*)malloc(sizeof(VitHashMap));
    map->capacity = 16;
    map->count = 0;
    map->entries = (HashEntry*)calloc(map->capacity, sizeof(HashEntry));
    return map;
}

static void hashmap_resize(VitHashMap* map, int new_capacity) {
    HashEntry* old_entries = map->entries;
    int old_capacity = map->capacity;

    map->capacity = new_capacity;
    map->entries = (HashEntry*)calloc(new_capacity, sizeof(HashEntry));
    map->count = 0;

    for (int i = 0; i < old_capacity; i++) {
        if (old_entries[i].key != NULL) {
            unsigned int index = hash_string(old_entries[i].key) % new_capacity;
            while (map->entries[index].key != NULL) {
                index = (index + 1) % new_capacity;
            }
            map->entries[index].key = old_entries[i].key;
            map->entries[index].value = old_entries[i].value;
            map->count++;
        }
    }
    free(old_entries);
}

void __vit_hashmap_set(void* handle, const char* key, const char* val) {
    if (!handle || !key || !val) return;
    VitHashMap* map = (VitHashMap*)handle;
    if (map->count * 2 >= map->capacity) {
        hashmap_resize(map, map->capacity * 2);
    }
    unsigned int index = hash_string(key) % map->capacity;
    while (map->entries[index].key != NULL) {
        if (strcmp(map->entries[index].key, key) == 0) {
            free(map->entries[index].value);
            map->entries[index].value = vit_strdup(val);
            return;
        }
        index = (index + 1) % map->capacity;
    }
    map->entries[index].key = vit_strdup(key);
    map->entries[index].value = vit_strdup(val);
    map->count++;
}

const char* __vit_hashmap_get(void* handle, const char* key) {
    if (!handle || !key) return "";
    VitHashMap* map = (VitHashMap*)handle;
    if (map->capacity == 0) return "";
    unsigned int index = hash_string(key) % map->capacity;
    while (map->entries[index].key != NULL) {
        if (strcmp(map->entries[index].key, key) == 0) {
            return map->entries[index].value;
        }
        index = (index + 1) % map->capacity;
    }
    return "";
}

double __vit_hashmap_has(void* handle, const char* key) {
    if (!handle || !key) return 0.0;
    VitHashMap* map = (VitHashMap*)handle;
    if (map->capacity == 0) return 0.0;
    unsigned int index = hash_string(key) % map->capacity;
    while (map->entries[index].key != NULL) {
        if (strcmp(map->entries[index].key, key) == 0) {
            return 1.0;
        }
        index = (index + 1) % map->capacity;
    }
    return 0.0;
}

void __vit_hashmap_remove(void* handle, const char* key) {
    if (!handle || !key) return;
    VitHashMap* map = (VitHashMap*)handle;
    if (map->capacity == 0) return;
    unsigned int index = hash_string(key) % map->capacity;
    while (map->entries[index].key != NULL) {
        if (strcmp(map->entries[index].key, key) == 0) {
            free(map->entries[index].key);
            free(map->entries[index].value);
            map->entries[index].key = NULL;
            map->entries[index].value = NULL;
            map->count--;

            // Backward shift entries in open-addressing probe chain
            int i = index;
            int j = (i + 1) % map->capacity;
            while (map->entries[j].key != NULL) {
                unsigned int k = hash_string(map->entries[j].key) % map->capacity;
                if ((i <= j) ? (k <= i || k > j) : (k <= i && k > j)) {
                    map->entries[i] = map->entries[j];
                    map->entries[j].key = NULL;
                    map->entries[j].value = NULL;
                    i = j;
                }
                j = (j + 1) % map->capacity;
            }
            return;
        }
        index = (index + 1) % map->capacity;
    }
}

double __vit_hashmap_size(void* handle) {
    if (!handle) return 0.0;
    VitHashMap* map = (VitHashMap*)handle;
    return (double)map->count;
}

void __vit_hashmap_free(void* handle) {
    if (!handle) return;
    VitHashMap* map = (VitHashMap*)handle;
    for (int i = 0; i < map->capacity; i++) {
        if (map->entries[i].key) free(map->entries[i].key);
        if (map->entries[i].value) free(map->entries[i].value);
    }
    free(map->entries);
    free(map);
}

// ==========================================
// 3. JSON Helper Utilities
// ==========================================

static char* g_json_ring_buf[8] = {NULL};
static int g_json_ring_idx = 0;

const char* __vit_json_escape_string(const char* str) {
    if (!str) return "\"\"";
    size_t len = strlen(str);
    size_t needed = len * 2 + 3;

    g_json_ring_idx = (g_json_ring_idx + 1) % 8;
    g_json_ring_buf[g_json_ring_idx] = (char*)realloc(g_json_ring_buf[g_json_ring_idx], needed);
    char* buf = g_json_ring_buf[g_json_ring_idx];
    if (!buf) return "\"\"";

    size_t pos = 0;
    buf[pos++] = '"';
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == '"') { buf[pos++] = '\\'; buf[pos++] = '"'; }
        else if (c == '\\') { buf[pos++] = '\\'; buf[pos++] = '\\'; }
        else if (c == '\n') { buf[pos++] = '\\'; buf[pos++] = 'n'; }
        else if (c == '\r') { buf[pos++] = '\\'; buf[pos++] = 'r'; }
        else if (c == '\t') { buf[pos++] = '\\'; buf[pos++] = 't'; }
        else { buf[pos++] = c; }
    }
    buf[pos++] = '"';
    buf[pos] = '\0';
    return buf;
}

const char* __vit_char_at(const char* str, double index) {
    if (!str) return vit_strdup("");
    int idx = (int)index;
    size_t len = strlen(str);
    if (idx < 0 || (size_t)idx >= len) return vit_strdup("");
    char* res = (char*)malloc(2);
    if (!res) return vit_strdup("");
    res[0] = str[idx];
    res[1] = '\0';
    return res;
}

double __vit_strlen(const char* str) {
    if (!str) return 0.0;
    return (double)strlen(str);
}

