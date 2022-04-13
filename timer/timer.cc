//
// Created by Mingjie on 2022/4/8.
//

#include "timer.h"

#include <utility>

namespace flyshark {

    TimerNode::TimerNode(int id, time_t exp, TimeoutCallBack &&cb) : id(id), expires(exp), callBack(std::move(cb)) {}

    bool TimerNode::operator<(const TimerNode &t) const {
        return expires < t.expires;
    }

    Timer::Timer() {
        heap_.reserve(64);
    }

    Timer::Timer(size_t capacity) {
        heap_.reserve(capacity);
    }

    void Timer::Add(int id, unsigned int seconds, TimeoutCallBack &&callBack) {
        if (index_.count(id) == 1 && index_[id] < heap_.size()) {
            Update(id, seconds, std::move(callBack));
            return;
        }

        heap_.emplace_back(id, time(nullptr) + seconds, std::forward<decltype(callBack)>(callBack));

        size_t curIndex = heap_.size() - 1;
        PercolateUp_(curIndex);
    }

    void Timer::Update(int id, unsigned int seconds) {
        Update(id, seconds, nullptr);
    }

    void Timer::Update(int id, unsigned int seconds, TimeoutCallBack &&callBack) {
        if (index_.count(id) == 0) return;
        size_t index = index_[id];
        if (index >= heap_.size()) return;
        heap_[index].expires = time(nullptr) + seconds;
        if (callBack) {
            heap_[index].callBack = std::forward<decltype(callBack)>(callBack);
        }

        PercolateDown_(static_cast<int>(index));
    }

    void Timer::Remove(int id) {
        if (index_.count(id) == 0) return;
        size_t index = index_[id];
        if (index >= heap_.size()) return;

        heap_[index].expires = INT64_MAX;
        PercolateDown_(static_cast<int>(index));
        index_.erase(id);
        heap_.pop_back();
    }

    void Timer::Pop_() {
        if (heap_.empty()) return;

        size_t index = heap_.size() - 1;
        heap_[0] = heap_[index];
        heap_.pop_back();
        index_.erase(heap_[index].id);
        PercolateDown_(0);
    }

    void Timer::Tick() {
        auto curTime = time(nullptr);

        while (!heap_.empty()) {
            auto topTask = heap_.front();

            if (topTask.expires > curTime) {
                break;
            }

            if (topTask.callBack) {
                topTask.callBack();
            }

            Pop_();
        }
    }

    //上滤操作
    void Timer::PercolateUp_(int hole) {
        auto temp = heap_[hole];
        size_t parent = 0;

        for (; hole > 0; hole = parent) {
            parent = (hole - 1) / 2;
            if (heap_[parent].expires <= temp.expires) {
                break;
            }
            index_[heap_[parent].id] = hole;
            heap_[hole] = heap_[parent];
        }
        index_[temp.id] = hole;
        heap_[hole] = temp;
    }

    //下滤操作
    void Timer::PercolateDown_(int hole) {
        auto temp = heap_[hole];
        int child = 0;

        int size = heap_.size();
        while (hole * 2 + 1 <= size - 1) {
            child = hole * 2 + 1;
            if ((child < size - 1) && (heap_[child + 1].expires < heap_[child].expires)) {
                ++child;
            }

            if (heap_[child].expires < temp.expires) {
                index_[heap_[child].id] = hole;
                heap_[hole] = heap_[child];
            } else {
                break;
            }
            hole = child;
        }

        index_[temp.id] = hole;
        heap_[hole] = temp;
    }

    bool Timer::Empty() {
        return heap_.empty();
    }
}