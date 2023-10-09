#pragma once

#include "TcpSocket.h"
#include "PostgresqlParser.h"

#include <list>
#include <string>
#include <unordered_map>
#include <fstream>

class Server
{
public:
    Server(const char* host, short port, const char* dbHost, short dbPort);

    void run();

private:
    bool isListener(int fd) const
    {
        return fd == listener_.getFd();
    }

    void onAccept(int epollfd, TcpSocket&& newClient);
    void onRead(int epollfd, int clientSocket);
    void onWrite(int epollfd, int clientSocket);
    void onError(int epollfd, int clientSocket);

    bool openSqlLogFile();
    void processPacket(PostgresqlParser& pgParser, const char* buffer, int size);

private:
    TcpSocket listener_;
    std::list<TcpSocket> sockets_;

    using SocketsIter = std::list<TcpSocket>::iterator;
    struct Connection
    {
        SocketsIter client;
        SocketsIter partner;
        std::string data;
        PostgresqlParser pgParser;
    };

    std::unordered_map<int, Connection> connections_;

    std::ofstream sqlLogFile_;

    const char* dbHost_ = nullptr;
    short dbPort_ = 0;

    bool isOk_ = false;

    enum { BUFFER_SIZE = 4096 };
};