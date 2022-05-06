//
// Created by Mingjie on 2022/4/8.
//

#ifndef FLYSHARK_HTTPSERVICE_H
#define FLYSHARK_HTTPSERVICE_H

#include "httpconn.h"
#include "threadpool.h"
#include "epoller.h"
#include "timer.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>

namespace flyshark {

    class HttpService {
    public:
        explicit HttpService(bool multiThread);
        ~HttpService();

        void OnGet(const std::string &url, RequestHandler &&call_back);

        void OnPost(const std::string &url, RequestHandler &&call_back);

        void Bind(int port);

        void SetTimeOut(unsigned int seconds);

        void SetWebRoot(const char *webroot);

        void Loop();

    private:
        void Accept_();

    private:
        static const size_t MAX_EVENT_NUMBER = 10000;

        int listenFd_;
        unsigned int timeslot_; //超时时间
        char *srcDir_;
        std::unique_ptr<Epoller> epoller_;
        std::unique_ptr<ThreadPool> threadPool_;
        std::unordered_map<int, HttpConn> conns_;
        std::unique_ptr<Timer> timer_;

        bool multiThread_;
    };
}


#endif //FLYSHARK_HTTPSERVICE_H
