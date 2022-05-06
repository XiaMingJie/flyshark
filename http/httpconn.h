//
// Created by Mingjie on 2022/4/8.
//

#ifndef FLYSHARK_HTTPCONN_H
#define FLYSHARK_HTTPCONN_H

#include "httprequest.h"
#include "httpresponse.h"
#include "buffer.h"
#include <unordered_map>
#include <functional>
#include <memory>
#include <atomic>

namespace flyshark {

    class Epoller;

    enum class OP_TYPE {
        READ,
        WRITE
    };

    using RequestHandler = std::function<void(HttpRequest&, HttpResponse&)>;

    class HttpConn {
    public:
        HttpConn();
        void Init(int fd);
        void Init();

        void Process(OP_TYPE type);
        void CloseConn();

        bool Closed() const;

        static void AddGetHandler(const std::string &url, RequestHandler &&request_handler);
        static void AddPostHandler(const std::string &url, RequestHandler &&request_handler);

    private:
        HTTP_CODE ProcessRead_();
        HTTP_CODE Read_();
        bool ProcessWrite_(HTTP_CODE httpCode);
        bool Write_();

        HTTP_CODE DoRequest_();
        HTTP_CODE DoFile_();

    public:
        HttpRequest httpRequest;
        HttpResponse httpResponse;

        static Epoller *epoller;

        //根目录
        static const char *webroot;

    private:
        //GET方法回调集合
        static std::unordered_map<std::string, RequestHandler> getCallBack_;
        //POST方法回调集合
        static std::unordered_map<std::string, RequestHandler> postCallBack_;

    private:
        int fd_;
        Buffer buffer_;
    };
}

#endif //FLYSHARK_HTTPCONN_H
