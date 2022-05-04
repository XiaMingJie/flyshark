//
// Created by Mingjie on 2022/4/8.
//

#include "logger.h"
#include "httpservice.h"

using namespace std;
using namespace flyshark;

int main() {
    HttpService service;

    //超时关闭连接
    service.SetTimeOut(180);

    service.OnGet("/get/test", [](HttpRequest &request, HttpResponse &response) {
        string json_res = R"({"method": "GET", "url": "/get/test"})";
        response.SetContentType("application/json");
        response.SetBody(json_res);
    });

    service.OnPost("/post/test", [](HttpRequest &request, HttpResponse &response) {
        string json_res = R"({"method": "POST", "url": "/post/test"})";
        response.SetContentType("application/json");
        response.SetBody(json_res);
    });

    service.Bind(8090);
    service.Loop();
    return 0;
}