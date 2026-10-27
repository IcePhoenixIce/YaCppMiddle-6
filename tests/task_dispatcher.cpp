// Тесты придумала и написала ИИ

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "task_dispatcher.hpp"

using namespace dispatcher;

TEST(BasicCheck, SimpleCheck) { EXPECT_EQ(1 + 1, 2); }

// здесь ваш код

TEST(TaskDispatcherTest, ExecutesScheduledTasks) {
    TaskDispatcher dispatcher(2);
    std::atomic<int> executed{0};
    constexpr int task_count = 100;
    for (int i = 0; i < task_count; ++i) {
        dispatcher.schedule(TaskPriority::Normal, [&executed] { executed.fetch_add(1, std::memory_order_relaxed); });
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (executed.load(std::memory_order_acquire) < task_count && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_EQ(executed.load(std::memory_order_acquire), task_count);
}

TEST(TaskDispatcherTest, HighPriorityTasksExecuteFirst) {
    TaskDispatcher dispatcher(1);
    std::mutex mutex;
    std::condition_variable cv;
    bool gate_open = false;
    std::vector<int> order;

    dispatcher.schedule(TaskPriority::Normal, [&] {
        std::unique_lock lock(mutex);
        cv.wait(lock, [&] { return gate_open; });
    });

    for (int i = 0; i < 3; ++i) {
        dispatcher.schedule(TaskPriority::Normal, [&] {
            std::lock_guard lock(mutex);
            order.push_back(0);
        });
    }
    for (int i = 0; i < 3; ++i) {
        dispatcher.schedule(TaskPriority::High, [&] {
            std::lock_guard lock(mutex);
            order.push_back(1);
        });
    }

    {
        std::lock_guard lock(mutex);
        gate_open = true;
    }
    cv.notify_all();

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (true) {
        bool done = false;
        {
            std::lock_guard lock(mutex);
            done = order.size() == 6 || std::chrono::steady_clock::now() >= deadline;
        }
        if (done) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_EQ(order, (std::vector<int>{1, 1, 1, 0, 0, 0}));
}

TEST(TaskDispatcherTest, DestructorWaitsForAllTasks) {
    std::atomic<int> executed{0};
    constexpr int task_count = 50;
    {
        TaskDispatcher dispatcher(2);
        for (int i = 0; i < task_count; ++i) {
            dispatcher.schedule(TaskPriority::Normal, [&executed] {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                executed.fetch_add(1, std::memory_order_relaxed);
            });
        }
    }
    EXPECT_EQ(executed.load(std::memory_order_acquire), task_count);
}

TEST(TaskDispatcherTest, TaskExceptionDoesNotKillWorker) {
    TaskDispatcher dispatcher(1);
    std::atomic<int> executed{0};
    dispatcher.schedule(TaskPriority::Normal, [] { throw std::runtime_error("task failure"); });
    dispatcher.schedule(TaskPriority::Normal, [&executed] { executed.fetch_add(1, std::memory_order_relaxed); });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (executed.load(std::memory_order_acquire) < 1 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_EQ(executed.load(std::memory_order_acquire), 1);
}

TEST(TaskDispatcherTest, ConcurrentScheduling) {
    TaskDispatcher dispatcher(4);
    std::atomic<int> executed{0};
    constexpr int task_count = 400;
    std::vector<std::jthread> schedulers;
    for (int t = 0; t < 4; ++t) {
        schedulers.emplace_back([&] {
            for (int i = 0; i < task_count / 4; ++i) {
                dispatcher.schedule(TaskPriority::Normal,
                                    [&executed] { executed.fetch_add(1, std::memory_order_relaxed); });
            }
        });
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (executed.load(std::memory_order_acquire) < task_count && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_EQ(executed.load(std::memory_order_acquire), task_count);
}

TEST(TaskDispatcherTest, CustomQueueConfig) {
    TaskDispatcher::QueueConfig config{
        {TaskPriority::High, {true, 10}},
        {TaskPriority::Normal, {false, std::nullopt}},
    };
    TaskDispatcher dispatcher(2, std::move(config));
    std::atomic<int> executed{0};
    dispatcher.schedule(TaskPriority::High, [&executed] { executed.fetch_add(1, std::memory_order_relaxed); });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (executed.load(std::memory_order_acquire) < 1 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_EQ(executed.load(std::memory_order_acquire), 1);
}