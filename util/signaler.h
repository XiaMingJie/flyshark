//
// Created by Mingjie on 2022/4/8.
//

#ifndef FLYSHARK_SIGNALER_H
#define FLYSHARK_SIGNALER_H

namespace signaler {
    extern void register_handler(int sig, void(*call_back)(int));
}

#endif //FLYSHARK_SIGNALER_H
