// Тесты придумала и написала ИИ

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>

#include "queue/priority_queue.hpp"

// здесь ваш код

namespace {

dispatcher::queue::PriorityQueue MakeDefaultQueue() {
    std::unordered_map<dispatcher::TaskPriority, dispatcher::queue::QueueOptions> options{
        {dispatcher::TaskPriority::High, {true, 100}},
        {dispatcher::TaskPriority::Normal, {false, std::nullopt}},
    };
    return dispatcher::queue::PriorityQueue(options);
}

}  // namespace

TEST(PriorityQueueTest, PopReturnsPushedTask) {
    auto queue = MakeDefaultQueue();
    int value = 0;
    queue.push(dispatcher::TaskPriority::Normal, [&value] { value = 42; });
    auto task = queue.pop();
    ASSERT_TRUE(task.has_value());
    (*task)();
    EXPECT_EQ(value, 42);
}

TEST(PriorityQueueTest, HighPriorityPoppedFirst) {
    auto queue = MakeDefaultQueue();
    std::vector<int> order;
    queue.push(dispatcher::TaskPriority::Normal, [&order] { order.push_back(1); });
    queue.push(dispatcher::TaskPriority::High, [&order] { order.push_back(2); });
    queue.push(dispatcher::TaskPriority::Normal, [&order] { order.push_back(3); });
    queue.push(dispatcher::TaskPriority::High, [&order] { order.push_back(4); });
    queue.shutdown();
    for (int i = 0; i < 4; ++i) {
        auto task = queue.pop();
        ASSERT_TRUE(task.has_value());
        (*task)();
    }
    EXPECT_EQ(order, (std::vector<int>{2, 4, 1, 3}));
}

TEST(PriorityQueueTest, PopBlocksUntilTaskArrives) {
    auto queue = MakeDefaultQueue();
    std::atomic<bool> popped{false};
    std::thread waiter([&] {
        auto task = queue.pop();
        popped.store(task.has_value(), std::memory_order_release);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(popped.load(std::memory_order_acquire));
    queue.push(dispatcher::TaskPriority::Normal, [] {});
    waiter.join();
    EXPECT_TRUE(popped.load(std::memory_order_acquire));
}

TEST(PriorityQueueTest, ShutdownWakesBlockedPop) {
    auto queue = MakeDefaultQueue();
    std::atomic<bool> finished{false};
    std::thread waiter([&] {
        auto task = queue.pop();
        finished.store(!task.has_value(), std::memory_order_release);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(finished.load(std::memory_order_acquire));
    queue.shutdown();
    waiter.join();
    EXPECT_TRUE(finished.load(std::memory_order_acquire));
}

TEST(PriorityQueueTest, PopAfterShutdownDrainsRemainingTasks) {
    auto queue = MakeDefaultQueue();
    std::atomic<int> executed{0};
    queue.push(dispatcher::TaskPriority::High, [&executed] { executed.fetch_add(1, std::memory_order_relaxed); });
    queue.push(dispatcher::TaskPriority::Normal, [&executed] { executed.fetch_add(1, std::memory_order_relaxed); });
    queue.shutdown();
    while (auto task = queue.pop()) {
        (*task)();
    }
    EXPECT_EQ(executed.load(std::memory_order_acquire), 2);
    EXPECT_FALSE(queue.pop().has_value());
}

TEST(PriorityQueueTest, PushUnknownPriorityThrows) {
    auto queue = MakeDefaultQueue();
    EXPECT_THROW(queue.push(static_cast<dispatcher::TaskPriority>(42), [] {}), std::invalid_argument);
}

TEST(PriorityQueueTest, BoundedConfigWithoutCapacityThrows) {
    std::unordered_map<dispatcher::TaskPriority, dispatcher::queue::QueueOptions> options{
        {dispatcher::TaskPriority::High, {true, std::nullopt}},
    };
    auto construct = [&]() { dispatcher::queue::PriorityQueue queue(options); };
    EXPECT_THROW(construct(), std::invalid_argument);
}

TEST(PriorityQueueTest, EmptyConfigThrows) {
    std::unordered_map<dispatcher::TaskPriority, dispatcher::queue::QueueOptions> options{};
    auto construct = [&]() { dispatcher::queue::PriorityQueue queue(options); };
    EXPECT_THROW(construct(), std::invalid_argument);
}

TEST(PriorityQueueTest, ConcurrentPushAndPop) {
    auto queue = MakeDefaultQueue();
    constexpr int task_count = 1000;
    std::atomic<int> executed{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < task_count / 2; ++i) {
                queue.push(dispatcher::TaskPriority::Normal,
                           [&executed] { executed.fetch_add(1, std::memory_order_relaxed); });
            }
        });
    }
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < task_count / 2; ++i) {
                auto task = queue.pop();
                if (task) {
                    (*task)();
                }
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    queue.shutdown();
    while (auto task = queue.pop()) {
        (*task)();
    }
    EXPECT_EQ(executed.load(std::memory_order_acquire), task_count);
}