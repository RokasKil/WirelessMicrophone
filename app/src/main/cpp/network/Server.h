//
// Created by Rokas on 2021-11-01.
//

#ifndef WIRELESS_MICROPHONE_SERVER_H
#define WIRELESS_MICROPHONE_SERVER_H

#include <string>
#include <cstring>
#include <thread>
#include <vector>
#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../log/Log.h"
#include <atomic_queue/atomic_queue.h>
#include <unistd.h>
#include <netinet/tcp.h>

using namespace std;

class Server {
public:
    Server(const char* address, unsigned short port, atomic_queue::AtomicQueue<int32_t, 1024*1024, 256*256*100> *queue);
    bool start();
    bool stop();
    bool isValid();
private:

    struct Connection {
        sockaddr_in addr;
        int status = 0; // 0 = Not connected, 1 = attempting to connect, 2 = connected WIP
        int socket = -1;
        unique_ptr<thread> thread = NULL;
        Connection(int socket, sockaddr_in addr);
        std::mutex sendMutex;
        uint64_t lastPinged = -1;
        uint64_t pingTimeOut = 1 * 60 * 1000;
        string getIP();
    };

    void acceptLoop();
    void sendLoop();
    void recvLoop(shared_ptr<Connection> connection);
    void send(const char *data, int len);
    void send(string data);
    void send(shared_ptr<Connection> connection, const char *data, int len);
    unsigned short port;
    int serverSocket;
    vector<shared_ptr<Connection>> connections;
    bool valid = true;
    bool running = false;
    unique_ptr<thread> acceptThread = NULL;
    unique_ptr<thread> sendThread = NULL;

    std::mutex connectionsMutex;
    atomic_queue::AtomicQueue<int32_t, 1024*1024, 256*256*100> *queue;
};


#endif //WIRELESS_MICROPHONE_SERVER_H
