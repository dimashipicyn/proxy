#pragma once

#include "Tcpsocket.h"

#include <functional>
#include <list>
#include <unordered_map>

class Server
{
public:
    Server(const char* host, short port);

    void run();


private:
    bool isListener(int fd) const
    {
        return fd == listener_->getFd();
    }

    void onAccept();
    void onRead();
    void onWrite();

private:
    TcpSocketPtr listener_;
    std::list<TcpSocketPtr> sockets_;

    using SocketsIter = std::list<TcpSocketPtr>::iterator;
    struct Connection
    {
        SocketsIter sender;
        SocketsIter receiver;
    };
    std::unordered_map<int, Connection> clients_;
};