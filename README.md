# flyshark
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;使用C++实现的简单的Web服务器，支持GET/POST请求，支持静态资源的访问和动态请求的处理。<br>
* 基于Reactor事件处理模式，使用线程池处理任务，能够快速响应HTTP请求。
* 实现了高性能定时器，超时关闭空闲连接。
* 使用mmap读取静态文件资源，减少对磁盘I/O操作，提高性能。
* 基于回调函数处理动态请求，简单易用。

## 使用方法
```c++
#include "httpservice.h"

using namespace std;
using namespace flyshark;

int main() {
    //创建HttpService对象
    HttpService service;

    //监听端口
    service.Bind(8090);
    //超时关闭连接 单位:秒
    service.SetTimeOut(180);

    //定义GET方法URL处理回调
    service.OnGet("/test/get", [](HttpRequest &request, HttpResponse &response) {
        string json_res = R"({"method": "GET", "name": "flyshark", "sex": "male", "age": 1, "phone": "xxxxxxxxxxx"})";
        //返回json格式串
        response.SetContentType("application/json");
        response.SetBody(json_res);
    });

    //定义POST方法URL处理回调
    service.OnPost("/test/post", [](HttpRequest &request, HttpResponse &response) {
        string json_res = R"({"method": "POST", "name": "flyshark", "sex": "male", "age": 1, "phone": "xxxxxxxxxxx"})";
        response.SetContentType("application/json");
        response.SetBody(json_res);
    });
    
    //服务器循环监听
    service.Loop();
    return 0;
}
```
## 测试
### 功能测试
GET请求访问/test/get路径，POST请求访问/test/post路径。
### 性能测试
未完成