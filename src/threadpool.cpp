#include "threadpool.h"

ThreadPool::ThreadPool(size_t num_threads) : running_(true) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        task_queue_.push(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::stop() {
    running_ = false;
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

void ThreadPool::worker() {
    while (running_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] {
                return !task_queue_.empty() || !running_;
            });
            if (!running_ && task_queue_.empty()) return;
            task = std::move(task_queue_.front());
            task_queue_.pop();
        }
        task();
    }
}
