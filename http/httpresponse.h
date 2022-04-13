//
// Created by Mingjie on 2022/4/8.
//

#ifndef FLYSHARK_HTTPRESPONSE_H
#define FLYSHARK_HTTPRESPONSE_H

#include <string>
#include <unordered_map>
#include <map>
#include <sys/stat.h>
#include <sys/uio.h>

namespace flyshark {

    class HttpResponse {
        friend class HttpConn;
    public:
        HttpResponse();
        void AddHeader(const std::string &name, const std::string &value);
        void SetContentLen(size_t content_len);
        void SetContentType(const std::string &content_type);
        void SetBody(const std::string &content);
        void SetBody(const char *content, size_t len);

    private:
        void Done_();
        void AddStatusLine_(int code);
        void SetKeepAlive_(bool ka);
        std::string Content_() const;
        void Reset_();
        void Mmap_(const std::string &file_path);
        void Unmap_();
        void SetErrorHtml_(int code);
        void MakeFile_();
        void MakeResponse_();

    private:
        std::string respLine_;
        std::map<std::string, std::string> respHeaders_;
        std::string respBody_;
        std::string respContent_;

        struct stat fileStat_{};    //文件操作结构体
        char *fileAddress_;         //mmap映射内存地址
        struct iovec iov_[2]{};     //集中写
        int iovCount_;

        bool keepAlive_;

        static const std::unordered_map<int, std::string> STATUS_CODE;
        static const std::unordered_map<int, std::string> ERROR_MESSAGE;
        static const std::unordered_map<std::string, std::string> CONTENT_TYPE;
    };
}

#endif //FLYSHARK_HTTPRESPONSE_H
