#pragma once

#include "TcpSocket.h"

#include <sys/select.h>

#include <functional>
#include <list>
#include <string>
#include <unordered_map>

class Server
{
public:
    Server(const char* host, short port, const char* dbHost, short dbPort);

    void run();

private:
    bool isListener(int fd) const
    {
        return fd == listener_->getFd();
    }

    void onAccept(fd_set& readSockets);
    void onRead(int clientSocket, fd_set& readSockets, fd_set& writeSockets);
    void onWrite(int clientSocket, fd_set& readSockets, fd_set& writeSockets);

private:
    TcpSocketPtr listener_;
    std::list<TcpSocketPtr> sockets_;

    using SocketsIter = std::list<TcpSocketPtr>::iterator;
    struct Connection
    {
        SocketsIter from;
        SocketsIter to;
        std::string data;
    };
    std::unordered_map<int, Connection> clients_;

    const char* dbHost_ = nullptr;
    short dbPort_ = 0;

    int maxSocket_ = 0;
    bool isOk_ = false;
};