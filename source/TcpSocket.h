#pragma once

#include <cstddef>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "wsock32.lib")

void close(int fd)
{
    closesocket(fd);
}

#else
#include <netinet/in.h>
#endif

#include "NonCopyable.h"

bool initSockets();
void shotdownSockets();

class TcpSocket;
using TcpSocketPtr = std::unique_ptr<TcpSocket>;

class TcpSocket : public NonCopyable
{
public:
    ~TcpSocket();

    static TcpSocketPtr listen(const char* host, short port);
    static TcpSocketPtr connect(const char* host, short port);

    bool makeNonBlock();
    TcpSocketPtr accept();

    int getFd() const;

    int recv(const char* s, size_t size);
    int send(const char* s, size_t size);

private:
    TcpSocket() = default;

private:
    int handle_ = -1;
    sockaddr addr_;
};