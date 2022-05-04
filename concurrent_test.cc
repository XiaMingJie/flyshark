//
// Created by Mingjie on 2022/5/4.
//

#include <thread>
#include "fdwrapper.h"
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("please input host and port.\n");
        return 1;
    }
    const char *host = argv[1];
    int port = atoi(argv[2]);
    int thread_number = 5;
    int per_conn_number = 200;

    struct sockaddr_in remote{};
    remote.sin_family = AF_INET;
    inet_pton(AF_INET, host, &remote.sin_addr.s_addr);
    remote.sin_port = htons(port);

    vector<thread> threads;

    threads.reserve(thread_number);

    for (int i = 0; i < thread_number; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < per_conn_number; ++j) {
                int connfd = fdwrapper::connection(remote);
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