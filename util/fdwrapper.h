//
// Created by xiamingjie on 2022/1/21.
//

#ifndef FLYSHARK_FDWRAPPER_H
#define FLYSHARK_FDWRAPPER_H

#include <arpa/inet.h>

namespace fdwrapper {
    /**
     * 设置 fd 为非阻塞, 并保留原属性
     * */
    extern int set_nonblocking(int fd);

    /**
     * 创建一个 Tcp socket 并监听
     * */
    extern int listen(struct sockaddr_in &address);
    extern int listen(int port);

    /**
     * 创建一个 Tcp 连接
     * */
    extern int connection(struct sockaddr_in &address);
    extern int connect(const char *host, int port);

    extern void close(int fd);
};


#endif //FLYSHARK_FDWRAPPER_H
