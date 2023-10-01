#pragma once

#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "wsock32.lib")
#elif __APPLE__
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#else
#endif

#include "NonCopyable.h"

bool initSockets();
void shotdownSockets();

class TcpSocket;
using TcpSocketPtr = std::unique_ptr<TcpSocket>;

class TcpSocket : public NonCopyable
{
public:
    static TcpSocketPtr create();
    
    ~TcpSocket();
    bool open(const char* host, short port);
    bool makeNonBlock();
    bool connect();
    bool listen();
    TcpSocketPtr accept();

    int getFd() const;

    int recv(const char* s, size_t size);
    int send(const char* s, size_t size);

private:
    TcpSocket() = default;

private:
    int handle_ = -1;
    sockaddr_in addr_;
};