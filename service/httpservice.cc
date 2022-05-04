//
// Created by Mingjie on 2022/4/8.
//

#include "httpservice.h"
#include "fdwrapper.h"
#include "logger.h"
#include "signaler.h"
#include <cassert>
#include <csignal>

namespace flyshark {

    //信号通知管道
    static int pipe_fd[2];

    //信号通知回调
    void sig_notify(int sig) {
        int save_errno = errno;
        int msg = sig;
        send(pipe_fd[1], (char *) &msg, 1, 0);
        errno = save_errno;
    }

    HttpService::HttpService() : listenFd_(-1), timeslot_(120),
            epoller_(std::make_unique<Epoller>()),
            threadPool_(std::make_unique<ThreadPool>(sysconf(_SC_NPROCESSORS_ONLN) * 4)),
            timer_(std::make_unique<Timer>()) {

        srcDir_ = getcwd(nullptr, 0);
        assert(srcDir_);
        strncat(srcDir_, "/resource", 10);
        HttpConn::webroot = srcDir_;

        HttpConn::epoller = epoller_.get();
        HttpConn::count = 0;
        assert(socketpair(PF_UNIX, SOCK_STREAM, 0, pipe_fd) != -1);
        fdwrapper::set_nonblocking(pipe_fd[1]);
        epoller_->AddReadEt(pipe_fd[0], false);
    }

    void HttpService::Bind(int port) {
        struct sockaddr_in listen_addr{};
        memset(&listen_addr, 0, sizeof(listen_addr));
        listen_addr.sin_family = AF_INET;
        listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        listen_addr.sin_port = htons(port);

        listenFd_ = fdwrapper::listen(listen_addr);
        assert(listenFd_ >= 0);

        epoller_->AddReadEt(listenFd_, false);
    }

    void HttpService::SetWebRoot(const char *webroot) {
        HttpConn::webroot = webroot;
    }

    void HttpService::SetTimeOut(unsigned int seconds) {
        timeslot_ = seconds;
    }

    void HttpService::OnGet(const std::string &url, RequestHandler &&call_back) {
        HttpConn::AddGetHandler(url, std::forward<decltype(call_back)>(call_back));
    }

    void HttpService::OnPost(const std::string &url, RequestHandler &&call_back) {
        HttpConn::AddPostHandler(url, std::forward<decltype(call_back)>(call_back));
    }

    void show_logo();

    void HttpService::Loop() {
        show_logo();

        //注册信号处理函数
        signaler::register_handler(SIGINT, sig_notify);
        signaler::register_handler(SIGTERM, sig_notify);
        signaler::register_handler(SIGALRM, sig_notify);

        alarm(timeslot_);

        struct epoll_event events[MAX_EVENT_NUMBER];
        bool stop = false;
        bool timeout = false;

        while (!stop) {
            int event_num = epoller_->Wait(events, MAX_EVENT_NUMBER, -1);
            if (event_num == -1 && errno != EINTR) {
                LOG_ERROR("epoll failure.");
                break;
            }

            for (int i = 0; i < event_num; ++i) {
                int io_fd = events[i].data.fd;
                uint32_t io_event = events[i].events;

                if (io_fd == listenFd_) {
                    Accept_();
                } else if ((io_fd == pipe_fd[0]) && (io_event & EPOLLIN)) {
                    char signals[1024];
                    int ret = recv(pipe_fd[0], signals, sizeof(signals), 0);
                    if (ret <= 0) {
                        continue;
                    } else {
                        for (int j = 0; j < ret; ++j) {
                            switch (signals[j]) {
                                case SIGALRM: {
                                    timeout = true;
                                    break;
                                }
                                case SIGINT:
                                case SIGTERM: {
                                    stop = true;
                                    break;
                                }
                            }
                        }
                    }
                } else if (io_event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    conns_[io_fd].CloseConn();
                    timer_->Remove(io_fd);
                } else if (io_event & EPOLLIN) {
                    timer_->Update(io_fd, timeslot_);
                    threadPool_->Post([this, fd = io_fd] {
                        this->conns_[fd].Process(OP_TYPE::READ);
                    });
                } else if (io_event & EPOLLOUT) {
                    timer_->Update(io_fd, timeslot_);
                    threadPool_->Post([this, fd = io_fd] {
                        this->conns_[fd].Process(OP_TYPE::WRITE);
                    });
                } else {}
            }

            if (timeout) {
                timer_->Tick();
                timeout = false;
                alarm(timeslot_);
            }
        }

        printf("\nbye!\n");
    }

    void HttpService::Accept_() {
        struct sockaddr_in cli_addr{};
        socklen_t cli_len = sizeof(cli_addr);

        while (true) {
            memset(&cli_addr, 0, sizeof(cli_addr));
            int conn_fd = accept(listenFd_, (struct sockaddr *) &cli_addr, &cli_len);
            if (conn_fd == -1) break;
            epoller_->AddReadEt(conn_fd);
            timer_->Add(conn_fd, timeslot_, [this, fd = conn_fd] {
                this->conns_[fd].CloseConn();
            });
            conns_[conn_fd].Init(conn_fd);
            HttpConn::count++;
            LOG_INFO("connection number: %d", HttpConn::count.load());
        }
    }

    void show_logo() {
        if (Logger::open) {
            LOG_INFO("  ______ _        _____ _                _    ");
            LOG_INFO(" |  ____| |      / ____| |              | |   ");
            LOG_INFO(" | |__  | |_   _| (___ | |__   __ _ _ __| | __");
            LOG_INFO(" |  __| | | | | |\\___ \\| '_ \\ / _` | '__| |/ /");
            LOG_INFO(" | |    | | |_| |____) | | | | (_| | |  |   < ");
            LOG_INFO(" |_|    |_|\\__, |_____/|_| |_|\\__,_|_|  |_|\\_\\");
            LOG_INFO("            __/ |                             ");
            LOG_INFO("           |___/                              ");
        } else {
            printf("  ______ _        _____ _                _    \n");
            printf(" |  ____| |      / ____| |              | |   \n");
            printf(" | |__  | |_   _| (___ | |__   __ _ _ __| | __\n");
            printf(" |  __| | | | | |\\___ \\| '_ \\ / _` | '__| |/ /\n");
            printf(" | |    | | |_| |____) | | | | (_| | |  |   < \n");
            printf(" |_|    |_|\\__, |_____/|_| |_|\\__,_|_|  |_|\\_\\\n");
            printf("            __/ |                             \n");
            printf("           |___/                              \n");
        }
    }

    HttpService::~HttpService() {
        free(srcDir_);
        fdwrapper::close(listenFd_);
        fdwrapper::close(pipe_fd[1]);
        fdwrapper::close(pipe_fd[0]);
    }
}
