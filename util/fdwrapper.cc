//
// Created by xiamingjie on 2022/1/21.
//

#include "fdwrapper.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
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

        if ((bind(sockFd, (struct sockaddr *) &address, sizeof(address)) == -1) || (::listen(sockFd, 4096) == -1)) {
            LOG_ERROR("%s\n", strerror(errno));
            close(sockFd);
            return -1;
        }

        return sockFd;
    }

    int listen(int port) {
        struct sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port);
        return listen(address);
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

    int connect(const char *host, int port) {
        struct sockaddr_in address {};
        address.sin_family = AF_INET;
        inet_pton(AF_INET, host, &address.sin_addr.s_addr);
        address.sin_port = htons(port);
        return connection(address);
    }

    void close(int fd) {
        ::close(fd);
    }
}