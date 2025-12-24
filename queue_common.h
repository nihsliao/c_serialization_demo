#ifndef QUEUE_COMMON_H
#define QUEUE_COMMON_H

#include "concurrentqueue/blockingconcurrentqueue.h"

using namespace moodycamel;
using Clock = std::chrono::high_resolution_clock;

enum class UserCommand {
    TIMEOUT = -2,
    UNINITIAL = -1,
    EXIT = 0,
    TPL_SINGLE_STRUCTURE,
    TPL_TWO_STRUCTURES_ARRAY,
    TPL_TEN_STRUCTURES_ARRAY,
    MPACK_SINGLE_STRUCTURE,
    MPACK_TWO_STRUCTURES_ARRAY,
    MPACK_TEN_STRUCTURES_ARRAY,
    NANOPB_SINGLE_STRUCTURE,
    NANOPB_TWO_STRUCTURES_ARRAY,
    NANOPB_TEN_STRUCTURES_ARRAY,    
};

struct QueuElement {
    UserCommand commandId = UserCommand::UNINITIAL;
    uint8_t buffer[MAX_BUFFER] = {0};
    size_t size = 0;
    void toString() {
        printf("commandId=%d, size=%ld\n", (int)commandId, size);
    }
};

template <typename T>
void clearQueue(moodycamel::BlockingConcurrentQueue<T>& q) {
    T tmp;
    while (q.try_dequeue(tmp)) {
        // discard
    }
}

template <typename T>
bool enqueueElement(moodycamel::BlockingConcurrentQueue<T>& q, const T& element) {
    bool success = q.try_enqueue(element);
    return success;
}
#endif
