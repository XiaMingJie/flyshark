//
// Created by xiamingjie on 2022/1/19.
//

#ifndef FLYSHARK_EPOLLER_H
#define FLYSHARK_EPOLLER_H

#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

namespace flyshark {

    class Epoller {
    public:
        explicit Epoller();

        ~Epoller();

        void AddReadEt(int fd, bool one_shot = true) const;

        void AddWriteEt(int fd, bool one_shot = true) const;

        void RemoveFd(int fd) const;

        void ModifyFd(int fd, uint32_t event) const;

        void CloseFd(int fd) const;

        int Wait(struct epoll_event *events, int maxEvent, int timeout) const;

        int Wait(int timeout);

        int GetFd(int index) const;

        uint32_t GetEvent(int index) const;

    private:
        int epollFd_;
        std::vector<struct epoll_event> events_;
    };
}

#endif //FLYSHARK_EPOLLER_H
