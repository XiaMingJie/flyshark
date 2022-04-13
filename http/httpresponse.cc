//
// Created by Mingjie on 2022/4/8.
//

#include "httpresponse.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

namespace flyshark {
    using std::unordered_map;
    using std::string;

    const unordered_map<int, string>  HttpResponse::STATUS_CODE = {
            {200, "OK"},
            {400, "Bad Request"},
            {403, "Forbidden"},
            {404, "Not Found"},
            {500, "Internal Error"}
    };

    const unordered_map<int, string> HttpResponse::ERROR_MESSAGE = {
            {400, "Your request has bad syntax or is inherently impossible to satisfy."},
            {403, "You do not have permission to get file from this server."},
            {404, "The requested file was not found on this server."},
            {500, "There was an unusual problem serving the requested file."}
    };

    const unordered_map<string, string> HttpResponse::CONTENT_TYPE = {
            {".html",  "text/html"},
            {".xml",   "text/xml"},
            {".xhtml", "application/xhtml+xml"},
            {".txt",   "text/plain"},
            {".rtf",   "application/rtf"},
            {".pdf",   "application/pdf"},
            {".word",  "application/nsword"},
            {".json",  "application/json"},
            {".png",   "image/png"},
            {".gif",   "image/gif"},
            {".jpg",   "image/jpeg"},
            {".jpeg",  "image/jpeg"},
            {".au",    "audio/basic"},
            {".mpeg",  "video/mpeg"},
            {".mpg",   "video/mpeg"},
            {".avi",   "video/x-msvideo"},
            {".mp4",   "video/mpeg4"},
            {".gz",    "application/x-gzip"},
            {".tar",   "application/x-tar"},
            {".css",   "text/css "},
            {".js",    "text/javascript "}
    };

    HttpResponse::HttpResponse() : fileAddress_(nullptr), iovCount_(0), keepAlive_(false) {
        memset(&fileStat_, 0, sizeof(fileStat_));
    }

    void HttpResponse::AddStatusLine_(int code) {
        string status_line;
        status_line.append("HTTP/1.1 ");
        status_line.append(std::to_string(code) + " ");
        if (STATUS_CODE.count(code) == 1) {
            status_line.append(STATUS_CODE.find(code)->second + "\r\n");
        } else {
            status_line.append(STATUS_CODE.find(400)->second + "\r\n");
        }
        respLine_ = status_line;
    }

    void HttpResponse::AddHeader(const string &name, const string &value) {
        respHeaders_.emplace(name, value);
    }

    void HttpResponse::SetContentLen(size_t content_len) {
        AddHeader("Content-Length", std::to_string(content_len));
    }

    void HttpResponse::SetContentType(const string &content_type) {
        AddHeader("Content-Type", content_type);
    }

    void HttpResponse::SetBody(const string &content) {
        SetContentLen(content.length());
        respBody_ = content;
    }

    void HttpResponse::SetBody(const char *content, size_t len) {
        SetContentLen(len);
        respBody_ = string(content, len);
    }

    void HttpResponse::SetKeepAlive_(bool ka) {
        keepAlive_ = ka;
    }

    void HttpResponse::Reset_() {
        respLine_.clear();
        respHeaders_.clear();
        respBody_.clear();
        respContent_.clear();
        keepAlive_ = false;
        iovCount_ = 0;
        memset(&fileStat_, 0, sizeof(fileStat_));
        fileAddress_ = nullptr;
    }

    void HttpResponse::Mmap_(const string &file_path) {
        int file_fd = open(file_path.c_str(), O_RDONLY);
        fileAddress_ = (char *) mmap(nullptr, fileStat_.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
        close(file_fd);
    }

    void HttpResponse::Unmap_() {
        if (fileAddress_) {
            munmap(fileAddress_, fileStat_.st_size);
            fileAddress_ = nullptr;
        }
    }

    void HttpResponse::SetErrorHtml_(int code) {
        string body;
        body += "<html><title>" + STATUS_CODE.find(code)->second + "</title>";
        body += "<h1>" + STATUS_CODE.find(code)->second + "</title>";
        body += "<p>" + ERROR_MESSAGE.find(code)->second + "</p>";
        body += "<hr><em>flyshark</em></body></html>";
        SetContentType(CONTENT_TYPE.find(".html")->second);
        SetBody(body);
    }

    void HttpResponse::Done_() {
        respContent_.append(respLine_);

        //检查 Content-Type 和 Content-Length 字段
        if (respHeaders_.count("Content-Type") == 0) {
            SetContentType("text/plain");
        }

        if (respHeaders_.count("Content-Length") == 0) {
            SetContentLen(respBody_.length());
        }

        for (auto &&header : respHeaders_) {
            respContent_.append(header.first);
            respContent_.append(": ");
            respContent_.append(header.second);
            respContent_.append("\r\n");
        }

        if (keepAlive_) {
            respContent_.append("Connection: keep-alive\r\n");
        } else {
            respContent_.append("Connection: close\r\n");
        }
        respContent_.append("Server: flyshark\r\n");
        respContent_.append("\r\n");
        if (!respBody_.empty()) {
            respContent_.append(respBody_);
        }
    }

    std::string HttpResponse::Content_() const {
        return respContent_;
    }

    void HttpResponse::MakeFile_() {
            char *resp_msg = const_cast<char *>(respContent_.c_str());
            iov_[0].iov_base = resp_msg;
            iov_[0].iov_len = respContent_.length();
            iov_[1].iov_base = fileAddress_;
            iov_[1].iov_len = fileStat_.st_size;
            iovCount_ = 2;
    }

    void HttpResponse::MakeResponse_() {
        char *resp_msg = const_cast<char *>(respContent_.c_str());
        iov_[0].iov_base = resp_msg;
        iov_[0].iov_len = respContent_.length();
        iovCount_ = 1;
    }
}