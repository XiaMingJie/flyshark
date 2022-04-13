//
// Created by Mingjie on 2022/4/8.
//

#ifndef FLYSHARK_TIMER_H
#define FLYSHARK_TIMER_H

#include <vector>
#include <ctime>
#include <memory>
#include <functional>
#include <unordered_map>

namespace flyshark {

    using TimeoutCallBack = std::function<void()>;

    struct TimerNode {
        int id;
        time_t expires;
        TimeoutCallBack callBack;
        TimerNode(int id, time_t exp, TimeoutCallBack &&cb);
        bool operator<(const TimerNode &t) const;
    };

    class Timer {
    public:
        Timer();
        explicit Timer(size_t capacity);

        Timer(const Timer& timer) = delete;
        Timer& operator = (const Timer& timer) = delete;

        ~Timer() = default;

        void Add(int id, unsigned int seconds, TimeoutCallBack &&callBack);
        void Update(int id, unsigned int seconds);
        void Update(int id, unsigned int seconds, TimeoutCallBack &&callBack);
        void Remove(int id);
        bool Empty();
        void Tick();

    private:
        void Pop_();
        void PercolateUp_(int hole);
        void PercolateDown_(int hole);

    private:
        std::vector<TimerNode> heap_;
        std::unordered_map<int, size_t> index_;
    };
}

#endif //FLYSHARK_TIMER_H
