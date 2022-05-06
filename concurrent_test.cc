//
// Created by Mingjie on 2022/5/4.
//

#include <thread>
#include "fdwrapper.h"
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("please input [host] [port] [threadNum] [preThreadConnNum].\n");
        return 1;
    }
    const char *host = argv[1];
    int port = atoi(argv[2]);
    int thread_number = atoi(argv[3]);
    int per_conn_number = atoi(argv[4]);

    vector<thread> threads;

    threads.reserve(thread_number);

    for (int i = 0; i < thread_number; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < per_conn_number; ++j) {
                int connfd = fdwrapper::connect(host, port);
                if (connfd < 0) continue;
            }
        });
    }

    for (int i = 0; i < thread_number; ++i) {
        threads[i].join();
    }

    getchar();

    return 0;
}