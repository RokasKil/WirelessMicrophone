//
// Created by Rokas on 2021-11-01.
//

#include "Server.h"

#include <memory>

Server::Server(const char* address, unsigned short port, AudioQueue *queue) {
    this->port = port;
    this->queue = queue;
    // Creating socket file descriptor
    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
    {
        LOGE("socket failed");
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
    int recieved = 0;
    unsigned int clientAddressLength = sizeof(clientAddress);

    char* buffer = new char[1024];

    LOGI("ACCEPTING");
    memset(&clientAddress, 0, clientAddressLength);
    while(running && (recieved = recvfrom(serverSocket, buffer, 1023, NULL, (struct sockaddr *) &clientAddress, &clientAddressLength)) > 0) {
        connectionsMutex.lock();
        if (running) {
            string address = getAddress(clientAddress);
            if (!connections.count(address)) {
                connections[address] = make_shared<Connection>(-1, clientAddress);

                //send audio stuff here
                LOGI("%s connected", connections[address]->getIP().c_str());
            }
            connections[address]->lastPinged = duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

        }
        connectionsMutex.unlock();
    }
    if (recieved <= 0) {
        LOGE("Error on accept %d %d", recieved, errno);
        valid = false;
    }
}

void Server::sendLoop() {
    char* buffer = new char[512];
    int used = 0;
    //LOGI("Send LOOP");
    while (running) {
        auto start = std::chrono::system_clock::now();


        //LOGI("BUILDING BUFFER");
        //buffer[0] = 'a';
        used = 0;
        unsigned int currentSize = queue->was_size();
        //LOGI("size %d", currentSize);
        for(unsigned int i = 0; i < currentSize && used + 2 <= 512; i++) {
            *((int16_t*)(buffer + used)) = (int16_t)queue->pop();
            used += 2;
        }
        //LOGI("used %d", used);
        //((unsigned char*) buffer)[1] = (unsigned char)((used - 2)/2);
        if (used == 0) {
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
        /*for (auto con = connections.begin(); con < connections.end(); con++) {
            shutdown((*con)->socket, SHUT_RDWR);
            close((*con)->socket);
            (*con)->status = 2;
        }
        for (auto con = connections.begin(); con < connections.end(); con++) {
            (*con)->thread->join();
        }*/
        connections.clear();
        connectionsMutex.unlock();
        acceptThread->join();
        sendThread->join();
    }
    return false;
}

void Server::send(const char *data, int len) {
    uint64_t time = duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    connectionsMutex.lock();
    vector<string> toRemove;
    for (auto & connection : connections) {
        if (connection.second->lastPinged + connection.second->pingTimeOut < time || !send(connection.second, data, len)) {
            toRemove.push_back(connection.first);
        }
    }
    for (auto & con : toRemove) {
        LOGI("%s removed", con.c_str());
        connections.erase(con);
    }
    connectionsMutex.unlock();
}

void Server::send(string data) {
    send(data.c_str(), data.length());
}

bool Server::send(shared_ptr<Connection> connection, const char *data, int len) {
    if (connection->status == 1) {
        connection->sendMutex.lock();
        int sent = sendto(serverSocket, data, len, MSG_CONFIRM, (const struct sockaddr *) &connection->addr, sizeof(connection->addr));
        if (sent == -1) {
            LOGE("Error on send %d %d", sent, errno);
            close(connection->socket);
            connection->status = 0;
            return false;
        }
        connection->sendMutex.unlock();
        return true;
    }
    return false;
}

string Server::getAddress(sockaddr_in addr) {
    stringstream ss;
    ss << inet_ntoa(addr.sin_addr) << ":" << htons(addr.sin_port);
    return ss.str();
}


Server::Connection::Connection(int socket, sockaddr_in addr) {
    this->socket = socket;
    this->status = 1;
    this->addr = addr;
}

string Server::Connection::getIP() {
    return Server::getAddress(addr);
}