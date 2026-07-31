#include "concurrency_rt.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <iostream>
#include <atomic>

struct VitChannel {
    std::queue<double> items;
    std::mutex mtx;
    std::condition_variable cv;
};

struct VitPromise {
    enum State { PENDING, RESOLVED, REJECTED } state = PENDING;
    double value = 0.0;
    std::mutex mtx;
    std::condition_variable cv;
};

extern "C" {

void* vit_thread_spawn(void* (*func)(void*), void* arg) {
    try {
        std::thread* t = new std::thread([func, arg]() {
            func(arg);
        });
        return static_cast<void*>(t);
    } catch (const std::exception& e) {
        std::cerr << "[VIT Runtime Error] Failed to spawn thread: " << e.what() << std::endl;
        return nullptr;
    }
}

void vit_thread_join(void* thread_ptr) {
    if (!thread_ptr) return;
    std::thread* t = static_cast<std::thread*>(thread_ptr);
    if (t->joinable()) {
        t->join();
    }
    delete t;
}

void vit_thread_detach(void* thread_ptr) {
    if (!thread_ptr) return;
    std::thread* t = static_cast<std::thread*>(thread_ptr);
    if (t->joinable()) {
        t->detach();
    }
    delete t;
}

void* vit_channel_create(void) {
    return static_cast<void*>(new VitChannel());
}

void vit_channel_send(void* channel_ptr, double val) {
    if (!channel_ptr) return;
    VitChannel* ch = static_cast<VitChannel*>(channel_ptr);
    {
        std::lock_guard<std::mutex> lock(ch->mtx);
        ch->items.push(val);
    }
    ch->cv.notify_one();
}

double vit_channel_receive(void* channel_ptr) {
    if (!channel_ptr) return 0.0;
    VitChannel* ch = static_cast<VitChannel*>(channel_ptr);
    std::unique_lock<std::mutex> lock(ch->mtx);
    ch->cv.wait(lock, [ch]() { return !ch->items.empty(); });
    double val = ch->items.front();
    ch->items.pop();
    return val;
}

void vit_channel_free(void* channel_ptr) {
    if (!channel_ptr) return;
    VitChannel* ch = static_cast<VitChannel*>(channel_ptr);
    delete ch;
}

void* vit_promise_create(void) {
    return static_cast<void*>(new VitPromise());
}

void vit_promise_resolve(void* promise_ptr, double val) {
    if (!promise_ptr) return;
    VitPromise* p = static_cast<VitPromise*>(promise_ptr);
    {
        std::lock_guard<std::mutex> lock(p->mtx);
        p->value = val;
        p->state = VitPromise::RESOLVED;
    }
    p->cv.notify_all();
}

double vit_promise_await(void* promise_ptr) {
    if (!promise_ptr) return 0.0;
    VitPromise* p = static_cast<VitPromise*>(promise_ptr);
    std::unique_lock<std::mutex> lock(p->mtx);
    p->cv.wait(lock, [p]() { return p->state != VitPromise::PENDING; });
    return p->value;
}

void vit_promise_free(void* promise_ptr) {
    if (!promise_ptr) return;
    VitPromise* p = static_cast<VitPromise*>(promise_ptr);
    delete p;
}

void vit_task_spawn(void (*func)(void*), void* arg) {
    std::thread t([func, arg]() {
        func(arg);
    });
    t.detach();
}

} // extern "C"
