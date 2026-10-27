#include "thread_pool/thread_pool.hpp"

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <utility>

namespace dispatcher::thread_pool {

// здесь ваш код

ThreadPool::ThreadPool(std::shared_ptr<queue::PriorityQueue> queue, size_t thread_count)
    : queue_(std::move(queue)) {
    if (!queue_) {
        throw std::invalid_argument("ThreadPool requires a valid queue");
    }
    if (thread_count == 0) {
        throw std::invalid_argument("ThreadPool requires at least one thread");
    }
    workers_.reserve(thread_count);
    for (size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back([this] { WorkerLoop(); });
    }
}

void ThreadPool::WorkerLoop() {
    while (auto task = queue_->pop()) {
        try {
            (*task)();
        } catch (const std::exception &e) {
            std::fprintf(stderr, "Task failed: %s\n", e.what());
        } catch (...) {
            std::fprintf(stderr, "Task failed with unknown exception\n");
        }
    }
}

ThreadPool::~ThreadPool() {
    queue_->shutdown();
    for (auto &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

} // namespace dispatcher::thread_pool