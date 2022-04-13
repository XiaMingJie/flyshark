//
// Created by Mingjie on 2022/4/8.
//

#include "signaler.h"
#include <csignal>
#include <cstring>
#include <cassert>

namespace signaler {

    void register_handler(int sig, void(*handler)(int)) {
        struct sigaction sa{};
        memset(&sa, '\0', sizeof(sa));
        sa.sa_handler = handler;
        /* 重新调用被信号中断的系统调用 */
        sa.sa_flags |= SA_RESTART;
        sigfillset(&sa.sa_mask);
        assert(sigaction(sig, &sa, nullptr) != -1);
    }
}