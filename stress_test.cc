//
// Created by Mingjie on 2022/4/12.
//

/**
 * 压力测试
 * */
#include <cstring>
#include "fdwrapper.h"
#include "epoller.h"
#include <thread>

static const char *request = "GET /get/test HTTP/1.1\r\nConnection: keep-alive\r\n\r\n";

void start_conn(flyshark::Epoller &epoller, int num, const char *ip, int port) {
    struct sockaddr_in addr{};
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    for (int i = 0; i < num; ++i) {
        if (i % 50 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        int conn_fd = fdwrapper::connection(addr);
        if (conn_fd < 0) continue;
        epoller.AddWriteEt(conn_fd, false);
    }
}

void close_conn(flyshark::Epoller &epoller, int fd) {
    epoller.RemoveFd(fd);
    fdwrapper::close(fd);
}

bool write_nbytes(int sockfd, const char *buffer, int len) {
    int send_bytes = 0;
    while (true) {
        send_bytes = send(sockfd, buffer, len, 0);
        if (send_bytes <= 0) {
            return false;
        }

        len -= send_bytes;
        buffer = buffer + send_bytes;
        if (len <= 0) {
//            printf("write %d bytes to sockfd %d with content: %s\n", send_bytes, sockfd, request);
            return true;
        }
    }
}

bool read_once(int sockfd, char *buffer, int len) {
    int recv_bytes = 0;
    memset(buffer, 0, len);
    recv_bytes = recv(sockfd, buffer, len, 0);
    if (recv_bytes <= 0) {
        return false;
    }
    printf("read in %d bytes from sockfd %d with content: %s\n", recv_bytes, sockfd, buffer);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("example: stress [host] [port] [connNum]\n");
        return 0;
    }

    flyshark::Epoller epoller;
    start_conn(epoller, atoi(argv[3]), argv[1], atoi(argv[2]));

    epoll_event events[10000];
    char buffer[2048];
    while (true) {
        int number = epoller.Wait(events, 10000, 2000);
        if (number == -1) {
            printf("epoll call failure.\n");
            break;
        }

        for (int i = 0; i < number; ++i) {
            int io_fd = events[i].data.fd;
            uint32_t io_event = events[i].events;

            if (io_event & EPOLLIN) {
                if (!read_once(io_fd, buffer, sizeof(buffer))) {
                    close_conn(epoller, io_fd);
                    continue;
                }
                epoller.ModifyFd(io_fd, EPOLLOUT);
            } else if (io_event & EPOLLOUT) {
                if (!write_nbytes(io_fd, request, strlen(request))) {
                    close_conn(epoller, io_fd);
                    continue;
                }
                epoller.ModifyFd(io_fd, EPOLLIN);
            } else if (io_event & (EPOLLERR | EPOLLHUP)) {
                close_conn(epoller, io_fd);
                continue;
            } else {}
        }
    }
}