//
// Created by Mingjie on 2022/4/8.
//

#ifndef FLYSHARK_THREAD_POOL_H
#define FLYSHARK_THREAD_POOL_H

#include <thread>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

namespace flyshark {

    class ThreadPool {
    public:
        explicit ThreadPool(size_t threadNumber);
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool();

        void Post(std::function<void()>&& task);

    private:
        struct Pool {
            bool stop = false;                          //线程停止标志
            std::queue<std::function<void()>> tasks;    //任务队列
            std::mutex mtx;
            std::condition_variable cond;
        };

        std::shared_ptr<Pool> pool_;
    };
}

#endif //FLYSHARK_THREAD_POOL_H
