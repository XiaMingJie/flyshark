//
// Created by xiamingjie on 2022/1/21.
//

#include "fdwrapper.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <csignal>
#include "logger.h"

namespace fdwrapper {

    using namespace flyshark;

    int set_nonblocking(int fd) {
        int old_op = fcntl(fd, F_GETFL);
        int new_op = old_op | O_NONBLOCK;
        fcntl(fd, F_SETFL, new_op);
        return old_op;
    }

    int listen(struct sockaddr_in &address) {
        int sockFd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockFd < 0) return -1;

        int val = 1;
        if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1) {
            LOG_ERROR("%s\n", strerror(errno));
            return -1;
        }

        if ((bind(sockFd, (struct sockaddr *) &address, sizeof(address)) == -1) || (::listen(sockFd, 300) == -1)) {
            LOG_ERROR("%s\n", strerror(errno));
            close(sockFd);
            return -1;
        }

        return sockFd;
    }

    int connection(struct sockaddr_in &address) {
        int sockFd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockFd < 0) return -1;

        if (connect(sockFd, (struct sockaddr *) &address, sizeof(address)) == -1) {
            close(sockFd);
            return -1;
        }

        return sockFd;
    }

    void close(int fd) {
        ::close(fd);
    }
}