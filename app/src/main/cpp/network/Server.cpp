//
// Created by Rokas on 2021-11-01.
//

#include "Server.h"

#include <memory>

Server::Server(const char* address, unsigned short port, AudioQueue *queue) {
    this->port = port;
    this->queue = queue;
    // Creating socket file descriptor
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        LOGE("socket failed");
        valid = false;
        return;
    }
    int flag = 1;
    int result = setsockopt(serverSocket,            /* socket affected */
                            IPPROTO_TCP,     /* set option at TCP level */
                            TCP_NODELAY,     /* name of option */
                            (char *) &flag,  /* the cast is historical cruft */
                            sizeof(int));    /* length of option value */

    if (result == -1) {
        LOGE("SETOPT FAILED");
        valid = false;
        return;
    }
}

bool Server::isValid() {
    return valid;
}
bool Server::start() {
    struct sockaddr_in addressStruct;
    addressStruct.sin_family = AF_INET;
    addressStruct.sin_addr.s_addr = INADDR_ANY;
    addressStruct.sin_port = htons(port );

    //Bind
    int result;
    if ((result = ::bind(serverSocket, (struct sockaddr *) &addressStruct, sizeof(addressStruct))) < 0)
    {
        LOGE("Bind failed %d %d", result, errno);
        valid = false;
        return false;
    }

    listen(serverSocket, 5);
    running = true;
    acceptThread = std::make_unique<thread>(&Server::acceptLoop, this);
    sendThread = std::make_unique<thread>(&Server::sendLoop, this);
    return true;
}

void Server::acceptLoop() {
    struct sockaddr_in clientAddress;
    int clientSocket;
    unsigned int clientAddressLength = sizeof(clientAddressLength);
    LOGI("ACCEPTING");
    while(running && (clientSocket = ::accept(serverSocket, (struct sockaddr *) &clientAddress, &clientAddressLength)) >= 0) {
        connectionsMutex.lock();
        if (running) {
            int flag = 1;
            int result = setsockopt(clientSocket,            /* socket affected */
                                    IPPROTO_TCP,     /* set option at TCP level */
                                    TCP_NODELAY,     /* name of option */
                                    (char *) &flag,  /* the cast is historical cruft */
                                    sizeof(int));    /* length of option value */

            if (result == -1) {
                LOGE("client SETOPT FAILED");
                close(clientSocket);
                continue;
            }
            shared_ptr<Connection> con = make_shared<Connection>(clientSocket, clientAddress);
            con->thread = make_unique<thread>(&Server::recvLoop, this, con);
            connections.push_back(con);
            LOGI("%s connected", con->getIP().c_str());
        }
        connectionsMutex.unlock();
    }
    if (clientSocket < 0) {
        LOGE("Error on accept %d", clientSocket);
        valid = false;
    }
}

void Server::sendLoop() {
    char* buffer = new char[1 + 1 + 510];
    int used = 0;
    //LOGI("Send LOOP");
    while (running) {
        auto start = std::chrono::system_clock::now();


        //LOGI("BUILDING BUFFER");
        buffer[0] = 'a';
        used = 2;
        unsigned int currentSize = queue->was_size();
        //LOGI("size %d", currentSize);
        for(unsigned int i = 0; i < currentSize && used + 2 <= 1 + 1 + 510; i++) {
            *((int16_t*)(buffer + used)) = (int16_t)queue->pop();
            used += 2;
        }
        //LOGI("used %d", used);
        ((unsigned char*) buffer)[1] = (unsigned char)((used - 2)/2);
        if (used == 2) {
            continue;
        }
        //LOGI("Sending");
        send(buffer, used);

        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        //LOGI("Send took %lld us for %d", elapsed.count(), used);
        //LOGI("Sleeping");
        //LOGI("Queue size %d", queue->was_size());
        std::this_thread::sleep_for(1ms);
        //LOGI("Slept");
    }
    delete[] buffer;
}


void Server::recvLoop(shared_ptr<Connection> connection) {
    char* buffer = new char[1024*1024];
    int received;
    while (running && connection->status == 1 && (received = ::recv(connection->socket, buffer, 1024 * 1024, NULL)) > 0)
    {
        if (received == -1) {

            connection->status = 0;
            close(connection->socket);
            LOGE("Error on recv %d", received);

        }
    }
    delete[] buffer;
}

bool Server::stop() {
    running = false;
    if (valid) {
        valid = false;
        shutdown(serverSocket, SHUT_RDWR);
        close(serverSocket);
        connectionsMutex.lock();
        for (auto con = connections.begin(); con < connections.end(); con++) {
            shutdown((*con)->socket, SHUT_RDWR);
            close((*con)->socket);
            (*con)->status = 2;
        }
        for (auto con = connections.begin(); con < connections.end(); con++) {
            (*con)->thread->join();
        }
        connections.clear();
        connectionsMutex.unlock();
        acceptThread->join();
        sendThread->join();
    }
    return false;
}

void Server::send(const char *data, int len) {
    connectionsMutex.lock();
    for (auto con = connections.begin(); con < connections.end(); con++) {
        send(*con, data, len);
    }
    connectionsMutex.unlock();
}

void Server::send(string data) {
    send(data.c_str(), data.length());
}

void Server::send(shared_ptr<Connection> connection, const char *data, int len) {
    if (connection->status == 1) {
        connection->sendMutex.lock();
        int sent = ::send(connection->socket, data, len, NULL);
        if (sent == -1) {
            LOGE("Error on send %d %d", sent, errno);
            close(connection->socket);
            connection->status = 0;
        }
        connection->sendMutex.unlock();
    }
}


Server::Connection::Connection(int socket, sockaddr_in addr) {
    this->socket = socket;
    this->status = 1;
    this->addr = addr;
}

string Server::Connection::getIP() {
    return string(inet_ntoa(addr.sin_addr));
}