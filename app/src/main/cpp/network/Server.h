//
// Created by Rokas on 2021-11-01.
//

#ifndef WIRELESS_MICROPHONE_SERVER_H
#define WIRELESS_MICROPHONE_SERVER_H

#include <string>
#include <cstring>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../log/Log.h"
#include "../queue/Queue.h"
#include <unistd.h>
#include <netinet/tcp.h>
#include <sstream>
using namespace std;

class Server {
public:
    Server(const char* address, unsigned short port, AudioQueue *queue);
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
        uint64_t pingTimeOut = 2 * 1000;
        string getIP();
    };
    void acceptLoop();
    void sendLoop();
    void recvLoop(shared_ptr<Connection> connection);
    void send(const char *data, int len);
    void send(string data);
    bool send(shared_ptr<Connection> connection, const char *data, int len);
    static string getAddress(sockaddr_in addr);
    unsigned short port;
    int serverSocket;
    map<string, shared_ptr<Connection>> connections;
    bool valid = true;
    bool running = false;
    unique_ptr<thread> acceptThread = NULL;
    unique_ptr<thread> sendThread = NULL;

    std::mutex connectionsMutex;
    AudioQueue *queue;
};


#endif //WIRELESS_MICROPHONE_SERVER_H
