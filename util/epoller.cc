//
// Created by xiamingjie on 2022/1/19.
//

#include "epoller.h"
#include "logger.h"
#include "fdwrapper.h"

#include <unistd.h>
#include <cstring>
#include <cassert>

namespace flyshark {

    Epoller::Epoller() {
        epollFd_ = epoll_create(5);
        assert(epollFd_ >= 0);
        events_.resize(100);
    }

    Epoller::~Epoller() {
        close(epollFd_);
    }

    int Epoller::Wait(struct epoll_event *events, int maxEvent, int timeout) const {
        return ::epoll_wait(epollFd_, events, maxEvent, timeout);
    }

    int Epoller::Wait(int timeout) {
        int eventsSize = static_cast<int>(events_.size());
        int eventNum = ::epoll_wait(epollFd_, events_.data(), static_cast<int>(events_.size()), timeout);
        if (eventNum == eventsSize) {
            events_.resize(eventNum * 2);
        }
        return eventNum;
    }

    int Epoller::GetFd(int index) const {
        return events_.at(index).data.fd;
    }

    uint32_t Epoller::GetEvent(int index) const {
        return events_.at(index).events;
    }

    void Epoller::AddReadEt(int fd, bool one_shot) const {
        struct epoll_event event = {};
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLERR;
        if (one_shot) {
            event.events |= EPOLLONESHOT;
        }
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == -1) {
            LOG_ERROR("fd: %d operation of add failure.", fd);
        }

        fdwrapper::set_nonblocking(fd);
    }

    void Epoller::AddWriteEt(int fd, bool one_shot) const {
        struct epoll_event event = {};
        event.data.fd = fd;
        event.events = EPOLLOUT | EPOLLET | EPOLLRDHUP | EPOLLERR;
        if (one_shot) {
            event.events |= EPOLLONESHOT;
        }
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == -1) {
            LOG_ERROR("fd: %d operation of add failure.", fd);
        }

        fdwrapper::set_nonblocking(fd);
    }

    void Epoller::RemoveFd(int fd) const {
        if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
            LOG_ERROR("fd: %d operation of del failure.", fd);
        }
    }

    void Epoller::ModifyFd(int fd, uint32_t eventType) const {
        struct epoll_event event = {};
        event.data.fd = fd;
        event.events = eventType | EPOLLET | EPOLLRDHUP | EPOLLERR;
        if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) == -1) {
            LOG_ERROR("fd: %d operation of mod failure.", fd);
        }
    }

    void Epoller::CloseFd(int fd) const {
        RemoveFd(fd);
        close(fd);
    }
}