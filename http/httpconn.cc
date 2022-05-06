//
// Created by Mingjie on 2022/4/8.
//

#include "httpconn.h"
#include "epoller.h"
#include "logger.h"
#include "fdwrapper.h"
#include "timer.h"

namespace flyshark {

    using std::string;
    using std::function;

    Epoller *HttpConn::epoller = nullptr;
    const char *HttpConn::webroot;
    std::unordered_map<std::string, RequestHandler> HttpConn::getCallBack_;
    std::unordered_map<std::string, RequestHandler> HttpConn::postCallBack_;

    HttpConn::HttpConn() : fd_(-1) {}

    void HttpConn::Init(int fd) {
        assert(epoller != nullptr);
        fd_ = fd;
        Init();
    }

    void HttpConn::Init() {
        buffer_.RetrieveAll();
        httpRequest.Reset_();
        httpResponse.Reset_();
    }

    HTTP_CODE HttpConn::Read_() {
        int error_no;
        while (true) {
            ssize_t read_bytes = buffer_.ReadFd(fd_, &error_no);
            if (read_bytes == -1) {
                if (error_no == EAGAIN) {
                    break;
                }
                return HTTP_CODE::INTERNAL_ERROR;
            } else if (read_bytes == 0) {
                return HTTP_CODE::CLOSED_CONNECTION;
            }
        }
        return HTTP_CODE::GET_REQUEST;
    }

    HTTP_CODE HttpConn::ProcessRead_() {
        HTTP_CODE ret = Read_();
        if (ret != HTTP_CODE::GET_REQUEST) {
            return ret;
        }

        ret = httpRequest.Parse_(buffer_);
        if (ret != HTTP_CODE::GET_REQUEST) {
            return ret;
        }

        return DoRequest_();
    }

    HTTP_CODE HttpConn::DoRequest_() {
        string request_method = httpRequest.Method();
        string service_handler = httpRequest.Url();
        //去除'?'后的参数
        string::size_type n = service_handler.find('?');
        if (n != string::npos) {
            service_handler = service_handler.substr(0, n);
        }
        //判断是否存在处理该请求的回调函数
        if ((request_method == "GET") && (getCallBack_.count(service_handler) == 1)) {
            getCallBack_[service_handler](httpRequest, httpResponse);
        } else if ((request_method == "POST") && (postCallBack_.count(service_handler) == 1)) {
            postCallBack_[service_handler](httpRequest, httpResponse);
        } else {    //没有处理该请求的回调函数，则寻找静态文件
            return DoFile_();
        }

        return HTTP_CODE::GET_REQUEST;
    }

    HTTP_CODE HttpConn::DoFile_() {
        string url = httpRequest.Url();
        string file_path = webroot;
        if (url == "/") {
            file_path += "/index.html";
        } else {
            file_path += url;
        }

        if (stat(file_path.c_str(), &httpResponse.fileStat_) < 0) {  //文件不存在
            LOG_ERROR("Request URL: %s does not exist.", url.c_str());
            httpResponse.SetErrorHtml_(404);
            return HTTP_CODE::NO_RESOURCE;
        }

        if (!(httpResponse.fileStat_.st_mode & S_IROTH)) {           //文件没有权限
            LOG_ERROR("Insufficient permissions.");
            httpResponse.SetErrorHtml_(403);
            return HTTP_CODE::FORBIDDEN_REQUEST;
        }

        if (S_ISDIR(httpResponse.fileStat_.st_mode)) {         //此文件路径是个目录
            LOG_ERROR("The file path is a directory.");
            httpResponse.SetErrorHtml_(400);
            return HTTP_CODE::BAD_REQUEST;
        }

        httpResponse.Mmap_(file_path);
        return HTTP_CODE::FILE_REQUEST;
    }

    bool HttpConn::ProcessWrite_(HTTP_CODE httpCode) {
        switch (httpCode) {
            case HTTP_CODE::GET_REQUEST: {
                httpResponse.AddStatusLine_(200);
                break;
            }
            case HTTP_CODE::BAD_REQUEST: {
                httpResponse.AddStatusLine_(400);
                break;
            }
            case HTTP_CODE::FORBIDDEN_REQUEST: {
                httpResponse.AddStatusLine_(403);
                break;
            }
            case HTTP_CODE::NO_RESOURCE: {
                httpResponse.AddStatusLine_(404);
                break;
            }
            case HTTP_CODE::FILE_REQUEST: {
                httpResponse.AddStatusLine_(200);
                httpResponse.SetKeepAlive_(httpRequest.KeepAlive());

                //通过后缀名判断文件类型
                string url = httpRequest.Url();
                string::size_type n = url.find_last_of('.');
                if (n != string::npos) {
                    string suffix = url.substr(n, url.length() - n);
                    if (HttpResponse::CONTENT_TYPE.count(suffix) == 1) {
                        httpResponse.SetContentType(HttpResponse::CONTENT_TYPE.find(suffix)->second);
                    }
                }
                if (httpResponse.fileStat_.st_size != 0) {
                    httpResponse.SetContentType("text/html");
                    httpResponse.SetContentLen(httpResponse.fileStat_.st_size);
                    httpResponse.Done_();
                    httpResponse.MakeFile_();   //响应文件
                    return true;
                } else {
                    httpResponse.SetContentType("text/html");
                    httpResponse.SetBody("<html><body></body></html>");
                }
                break;
            }
            default: {
                return false;
            }
        }

        httpResponse.SetKeepAlive_(httpRequest.KeepAlive());
        httpResponse.Done_();
        httpResponse.MakeResponse_();   //普通响应
        return true;
    }

    bool HttpConn::Write_() {
        //已发送数据长度
        ssize_t send_to_bytes = 0;

        while (true) {
            send_to_bytes = writev(fd_, httpResponse.iov_, httpResponse.iovCount_);

            if (send_to_bytes == -1) {
                if (errno == EAGAIN) {
                    epoller->ModifyFd(fd_, EPOLLOUT | EPOLLONESHOT);
                    return true;
                }
                httpResponse.Unmap_();
            }

            if (httpResponse.iov_[0].iov_len + httpResponse.iov_[1].iov_len == 0) {
                httpResponse.Unmap_();
                if (httpRequest.KeepAlive()) {
                    Init();
                    epoller->ModifyFd(fd_, EPOLLIN | EPOLLONESHOT);
                    return true;
                } else {
                    epoller->ModifyFd(fd_, EPOLLIN | EPOLLONESHOT);
                    return false;
                }
            } else if (static_cast<size_t >(send_to_bytes) > httpResponse.iov_[0].iov_len) {
                httpResponse.iov_[1].iov_base = (uint8_t *) httpResponse.iov_[1].iov_base + (send_to_bytes - httpResponse.iov_[0].iov_len);
                httpResponse.iov_[1].iov_len -= (send_to_bytes - httpResponse.iov_[0].iov_len);
                httpResponse.iov_[0].iov_len = 0;
            } else {
                httpResponse.iov_[0].iov_base = (uint8_t *) httpResponse.iov_[0].iov_base + send_to_bytes;
                httpResponse.iov_[0].iov_len -= send_to_bytes;
            }
        }
    }

    void HttpConn::Process(OP_TYPE type) {
        if (type == OP_TYPE::READ) {
            HTTP_CODE ret = ProcessRead_();
            if (ret == HTTP_CODE::NO_REQUEST) {
                epoller->ModifyFd(fd_, EPOLLIN | EPOLLONESHOT);
                return;
            }
            bool writeable = ProcessWrite_(ret);
            if (!writeable) {
                CloseConn();
                return;
            }

            epoller->ModifyFd(fd_, EPOLLOUT | EPOLLONESHOT);
        } else if (type == OP_TYPE::WRITE) {
            if (!Write_()) {
                CloseConn();
            }
        } else {}
    }

    void HttpConn::CloseConn() {
        if (fd_ != -1) {
            epoller->RemoveFd(fd_);
            fdwrapper::close(fd_);
            fd_ = -1;
        }
    }

    bool HttpConn::Closed() const {
        return fd_ == -1;
    }

    void HttpConn::AddGetHandler(const std::string &url, RequestHandler &&request_handler) {
        getCallBack_.emplace(url, std::forward<decltype(request_handler)>(request_handler));
    }

    void HttpConn::AddPostHandler(const string &url, RequestHandler &&request_handler) {
        postCallBack_.emplace(url, std::forward<decltype(request_handler)>(request_handler));
    }
}