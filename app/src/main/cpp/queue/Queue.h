//
// Created by Rokas on 2021-11-02.
//

#ifndef WIRELESS_MICROPHONE_QUEUE_H
#define WIRELESS_MICROPHONE_QUEUE_H
#include <atomic_queue/atomic_queue.h>
typedef atomic_queue::AtomicQueue<int32_t, 2048, 256*256*100> AudioQueue;
#endif //WIRELESS_MICROPHONE_QUEUE_H
