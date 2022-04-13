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
    }

    Epoller::~Epoller() {
        close(epollFd_);
    }

    int Epoller::Wait(struct epoll_event *events, int maxEvent, int timeout) const {
        return ::epoll_wait(epollFd_, events, maxEvent, timeout);
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