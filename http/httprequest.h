//
// Created by Mingjie on 2022/4/8.
//

#ifndef FLYSHARK_HTTPREQUEST_H
#define FLYSHARK_HTTPREQUEST_H

#include "buffer.h"
#include <string>
#include <unordered_map>

namespace flyshark {

    //主状态机状态
    enum class PARSE_STATE {
        REQUEST_LINE,
        REQUEST_HEADER,
        REQUEST_BODY,
        FINISH
    };

    //HTTP请求结果
    enum class HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        FORBIDDEN_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
        FILE_REQUEST,
        NO_RESOURCE
    };

    class HttpRequest {
        friend class HttpConn;
    public:
        HttpRequest();
        std::string Method() const;
        std::string Url() const;
        std::string Version() const;
        std::string Body() const;
        bool KeepAlive();

        std::string GetParameter(const std::string &name);
        std::string GetHeaders(const std::string &name);

    public:
        //解析请求
        HTTP_CODE Parse_(Buffer &buffer);
        //分析请求行
        bool ParseReqLine_(const std::string &line);
        //分析头部字段
        bool ParseHeaders_(const std::string &line);
        //分析请求体
        void ParseBody_(std::string &body);
        //解析请求参数
        void ParseParameters_(std::string &content);
        void Reset_();

    private:
        std::string content_;
        PARSE_STATE parseState_;
        std::string method_;
        std::string url_;
        std::string version_;
        std::unordered_map<std::string, std::string> headers_;
        std::unordered_map<std::string, std::string> parameters_;
    };
}

#endif //FLYSHARK_HTTPREQUEST_H
