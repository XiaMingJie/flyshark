//
// Created by Mingjie on 2022/4/8.
//

#include "threadpool.h"
#include <cassert>

namespace flyshark {

    using std::thread;

    ThreadPool::ThreadPool(size_t threadNumber) : pool_(std::make_shared<Pool>()) {
        assert(threadNumber > 0);

        for (size_t i = 0; i < threadNumber; ++i) {
            thread([p = pool_]() {
                std::unique_lock<std::mutex> lk(p->mtx);
                while (true) {
                    if (!p->tasks.empty()) {
                        auto task = std::move(p->tasks.front());
                        p->tasks.pop();
                        lk.unlock();
                        task();
                        lk.lock();
                    } else if (p->stop) {
                        break;
                    } else {
                        p->cond.wait(lk);
                    }
                }
            }).detach();
        }
    }

    void ThreadPool::Post(std::function<void()> &&task) {
        std::lock_guard<std::mutex> lockGuard(pool_->mtx);
        pool_->tasks.emplace(std::forward<decltype(task)>(task));
        pool_->cond.notify_one();
    }

    ThreadPool::~ThreadPool() {
        if (pool_) {
            std::lock_guard<std::mutex> lockGuard(pool_->mtx);
            pool_->stop = true;
            pool_->cond.notify_all();
        }
    }
};