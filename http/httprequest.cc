//
// Created by Mingjie on 2022/4/8.
//

#include "httprequest.h"
#include <cstring>
#include <algorithm>

namespace flyshark {
    using std::string;

    HttpRequest::HttpRequest() : parseState_(PARSE_STATE::REQUEST_LINE) {}

    bool HttpRequest::ParseReqLine_(const string &line) {
        /**
         * [请求方法]空格[URL]空格[协议版本]回车换行
         * */

        if (line.empty()) {
            return false;
        }

        string::size_type check_idx = 0;
        //请求方法
        string::size_type space_idx = line.find(' ', check_idx);
        if (space_idx == string::npos) {
            return false;
        }
        string method = line.substr(check_idx, space_idx);
        //仅支持GET和POST
        if (strcasecmp(method.c_str(), "GET") == 0) {
            method_ = "GET";
        } else if (strcasecmp(method.c_str(), "POST") == 0) {
            method_ = "POST";
        } else {
            return false;
        }

        //URL
        check_idx = space_idx + 1;
        space_idx = line.find(' ', check_idx);
        if (space_idx == string::npos) {
            return false;
        }
        string url = line.substr(check_idx, space_idx - check_idx);
        if (url.empty() || url[0] != '/') {
            return false;
        }
        url_ = url;

        //协议版本
        check_idx = space_idx + 1;
        string version = line.substr(check_idx, line.length() - check_idx);
        //仅支持HTTP1.1
        if (strcasecmp(version.c_str(), "HTTP/1.1") != 0) {
            return false;
        }
        version_ = version;

        //检查URL是否带参数
        check_idx = url.find('?');
        if (check_idx != string::npos && check_idx < url.length() - 1) {
            //解析参数
            string request_parm = url.substr(check_idx + 1, url.length() - check_idx - 1);
            ParseParameters_(request_parm);
        }

        //状态转移到头部字段的解析
        parseState_ = PARSE_STATE::REQUEST_HEADER;
        return true;
    }

    bool HttpRequest::ParseHeaders_(const string &line) {
        /**
         * [键]:空格[值]回车换行
         * */
         //遇到空行
        if (line.empty()) {
            //状态转移到消息体的解析
            parseState_ = PARSE_STATE::REQUEST_BODY;
            return true;
        }

        string::size_type mid_idx = line.find(": ");
        if (mid_idx == string::npos || mid_idx == 0) {
            return false;
        }

        string key = line.substr(0, mid_idx);
        string value = line.substr(mid_idx + 2, line.length() - mid_idx + 1);

        headers_[key] = value;

        return true;
    }

    int conver_hex(char ch) {
        if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
        if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
        return ch;
    }

    void HttpRequest::ParseBody_(string &body) {
        if (method_ == "POST" && headers_["Content-Type"] == "application/x-www-form-urlencoded") {
            ParseParameters_(body);
        }

        parseState_ = PARSE_STATE::FINISH;
    }

    void HttpRequest::ParseParameters_(string &content) {
        string key, value;
        int num = 0;
        int n = content.length();
        int i = 0, j = 0;

        for (; i < n; ++i) {
            char ch = content[i];
            switch (ch) {
                case '=':
                    key = content.substr(j, i - j);
                    j = i + 1;
                    break;
                case '+':
                    content[i] = ' ';
                    break;
                case '%':
                    num = conver_hex(content[i + 1]) * 16 + conver_hex(content[i + 2]);
                    content[i + 2] = num % 10 + '0';
                    content[i + 1] = num / 10 + '0';
                    i += 2;
                    break;
                case '&':
                    value = content.substr(j, i - j);
                    j = i + 1;
                    parameters_[key] = value;
                    break;
                default:
                    break;
            }
        }

        if (parameters_.count(key) == 0 && j < i) {
            value = content.substr(j, i - j);
            parameters_[key] = value;
        }
    }

    HTTP_CODE HttpRequest::Parse_(Buffer &buffer) {
        if (buffer.ReadableBytes() <= 0) {
            return HTTP_CODE::NO_REQUEST;
        }

        const char *CRLF = "\r\n";
        /* 主状态机，用于从msg中取出所有完整的行 */
        while (parseState_ != PARSE_STATE::FINISH) {
            const char *line_end = std::search(buffer.Peek(), buffer.BeginWriteConst(), CRLF, CRLF + 2);
            string line(buffer.Peek(), line_end);

            switch (parseState_) {
                case PARSE_STATE::REQUEST_LINE: {   /* 分析请求行 */
                    if (!ParseReqLine_(line)) {
                        return HTTP_CODE::BAD_REQUEST;
                    }
                    break;
                }
                case PARSE_STATE::REQUEST_HEADER: { /* 分析请求头部 */
                    if (!ParseHeaders_(line)) {
                        return HTTP_CODE::BAD_REQUEST;
                    }
                    if (buffer.ReadableBytes() <= 2) {
                        parseState_ = PARSE_STATE::FINISH;
                    }
                    break;
                }
                case PARSE_STATE::REQUEST_BODY: {  /* 分析请求体 */
                    ParseBody_(line);
                    break;
                }
                default: {
                    return HTTP_CODE::BAD_REQUEST;
                }
            }
            if (line_end == buffer.BeginWrite()) {
                break;
            }
            buffer.RetrieveUntil(line_end + 2);
        }

        return HTTP_CODE::GET_REQUEST;
    }

    string HttpRequest::Method() const {
        return method_;
    }

    string HttpRequest::Url() const {
        return url_;
    }

    string HttpRequest::Version() const {
        return version_;
    }

    string HttpRequest::Body() const {
        return content_;
    }

    bool HttpRequest::KeepAlive() {
        if (headers_.find("Connection") != headers_.end()) {
            string connection = headers_["Connection"];
            if (strcasecmp(connection.c_str(), "keep-alive") == 0) {
                return true;
            }
        }
        return false;
    }

    string HttpRequest::GetHeaders(const string &name) {
        if (headers_.count(name) == 0) {
            return "";
        }
        return headers_[name];
    }

    string HttpRequest::GetParameter(const string &name) {
        if (parameters_.count(name) == 0) {
            return "";
        }
        return parameters_[name];
    }

    void HttpRequest::Reset_() {
        parseState_ = PARSE_STATE::REQUEST_LINE;
        content_.clear();
        url_.clear();
        version_.clear();
        method_.clear();
        headers_.clear();
        parameters_.clear();
    }
}