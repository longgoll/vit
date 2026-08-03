#ifdef _WIN32

#include "concurrency_rt.h"

// Forward C runtime and Win32 declarations to avoid system header dependency issues
typedef void* HANDLE;
typedef unsigned long DWORD;
typedef int BOOL;

typedef struct _RTL_CRITICAL_SECTION_DEBUG* PRTL_CRITICAL_SECTION_DEBUG;
typedef struct _CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    long LockCount;
    long RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    unsigned long* SpinCount;
} CRITICAL_SECTION, *PCRITICAL_SECTION;

typedef struct _CONDITION_VARIABLE {
    void* Ptr;
} CONDITION_VARIABLE, *PCONDITION_VARIABLE;

#define INFINITE 0xFFFFFFFF

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t size);
void free(void* ptr);

__declspec(dllimport) HANDLE __stdcall CreateThread(void* lpThreadAttributes, size_t dwStackSize, DWORD (__stdcall *lpStartAddress)(void*), void* lpParameter, DWORD dwCreationFlags, DWORD* lpThreadId);
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE hObject);
__declspec(dllimport) DWORD __stdcall WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);

__declspec(dllimport) void __stdcall InitializeCriticalSection(CRITICAL_SECTION* lpCriticalSection);
__declspec(dllimport) void __stdcall DeleteCriticalSection(CRITICAL_SECTION* lpCriticalSection);
__declspec(dllimport) void __stdcall EnterCriticalSection(CRITICAL_SECTION* lpCriticalSection);
__declspec(dllimport) void __stdcall LeaveCriticalSection(CRITICAL_SECTION* lpCriticalSection);

__declspec(dllimport) void __stdcall InitializeConditionVariable(CONDITION_VARIABLE* ConditionVariable);
__declspec(dllimport) BOOL __stdcall SleepConditionVariableCS(CONDITION_VARIABLE* ConditionVariable, CRITICAL_SECTION* CriticalSection, DWORD dwMilliseconds);
__declspec(dllimport) void __stdcall WakeConditionVariable(CONDITION_VARIABLE* ConditionVariable);
__declspec(dllimport) void __stdcall WakeAllConditionVariable(CONDITION_VARIABLE* ConditionVariable);

#ifdef __cplusplus
}
#endif

typedef struct {
    double* data;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cv;
} WinChannel;

typedef struct {
    int state; // 0 = PENDING, 1 = RESOLVED
    double value;
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cv;
} WinPromise;

typedef struct {
    void* (*func)(void*);
    void* arg;
} ThreadLauncherArg;

static DWORD __stdcall win32_thread_proc(void* param) {
    ThreadLauncherArg* larg = (ThreadLauncherArg*)param;
    larg->func(larg->arg);
    free(larg);
    return 0;
}

void* vit_thread_spawn(void* (*func)(void*), void* arg) {
    ThreadLauncherArg* larg = (ThreadLauncherArg*)malloc(sizeof(ThreadLauncherArg));
    larg->func = func;
    larg->arg = arg;
    HANDLE hThread = CreateThread(NULL, 0, win32_thread_proc, larg, 0, NULL);
    return (void*)hThread;
}

void vit_thread_join(void* thread_ptr) {
    if (!thread_ptr) return;
    HANDLE hThread = (HANDLE)thread_ptr;
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
}

void vit_thread_detach(void* thread_ptr) {
    if (!thread_ptr) return;
    HANDLE hThread = (HANDLE)thread_ptr;
    CloseHandle(hThread);
}

void* vit_channel_create(void) {
    WinChannel* ch = (WinChannel*)malloc(sizeof(WinChannel));
    ch->capacity = 1024;
    ch->data = (double*)malloc(sizeof(double) * ch->capacity);
    ch->head = 0;
    ch->tail = 0;
    ch->count = 0;
    InitializeCriticalSection(&ch->cs);
    InitializeConditionVariable(&ch->cv);
    return (void*)ch;
}

void vit_channel_send(void* channel_ptr, double val) {
    if (!channel_ptr) return;
    WinChannel* ch = (WinChannel*)channel_ptr;
    EnterCriticalSection(&ch->cs);
    if (ch->count < ch->capacity) {
        ch->data[ch->tail] = val;
        ch->tail = (ch->tail + 1) % ch->capacity;
        ch->count++;
    }
    LeaveCriticalSection(&ch->cs);
    WakeConditionVariable(&ch->cv);
}

double vit_channel_receive(void* channel_ptr) {
    if (!channel_ptr) return 0.0;
    WinChannel* ch = (WinChannel*)channel_ptr;
    EnterCriticalSection(&ch->cs);
    while (ch->count == 0) {
        SleepConditionVariableCS(&ch->cv, &ch->cs, INFINITE);
    }
    double val = ch->data[ch->head];
    ch->head = (ch->head + 1) % ch->capacity;
    ch->count--;
    LeaveCriticalSection(&ch->cs);
    return val;
}

void vit_channel_free(void* channel_ptr) {
    if (!channel_ptr) return;
    WinChannel* ch = (WinChannel*)channel_ptr;
    DeleteCriticalSection(&ch->cs);
    free(ch->data);
    free(ch);
}

void* vit_promise_create(void) {
    WinPromise* p = (WinPromise*)malloc(sizeof(WinPromise));
    p->state = 0;
    p->value = 0.0;
    InitializeCriticalSection(&p->cs);
    InitializeConditionVariable(&p->cv);
    return (void*)p;
}

void vit_promise_resolve(void* promise_ptr, double val) {
    if (!promise_ptr) return;
    WinPromise* p = (WinPromise*)promise_ptr;
    EnterCriticalSection(&p->cs);
    p->value = val;
    p->state = 1;
    LeaveCriticalSection(&p->cs);
    WakeAllConditionVariable(&p->cv);
}

double vit_promise_await(void* promise_ptr) {
    if (!promise_ptr) return 0.0;
    WinPromise* p = (WinPromise*)promise_ptr;
    EnterCriticalSection(&p->cs);
    while (p->state == 0) {
        SleepConditionVariableCS(&p->cv, &p->cs, INFINITE);
    }
    double val = p->value;
    LeaveCriticalSection(&p->cs);
    return val;
}

void vit_promise_free(void* promise_ptr) {
    if (!promise_ptr) return;
    WinPromise* p = (WinPromise*)promise_ptr;
    DeleteCriticalSection(&p->cs);
    free(p);
}

void vit_task_spawn(void (*func)(void*), void* arg) {
    ThreadLauncherArg* larg = (ThreadLauncherArg*)malloc(sizeof(ThreadLauncherArg));
    larg->func = (void* (*)(void*))func;
    larg->arg = arg;
    HANDLE hThread = CreateThread(NULL, 0, win32_thread_proc, larg, 0, NULL);
    if (hThread) CloseHandle(hThread);
}

#endif /* _WIN32 */
