#pragma once

#include <cstddef>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

#include "NonCopyable.h"

bool initSockets();
void shotdownSockets();

class TcpSocket : public NonCopyable
{
public:
    TcpSocket() = default;
    ~TcpSocket();

    TcpSocket(TcpSocket&& other)
    {
        std::swap(handle_, other.handle_);
    }
    
    TcpSocket& operator=(TcpSocket&& other)
    {
        std::swap(handle_, other.handle_);
        return *this;
    }

    static TcpSocket listen(const char* host, short port);
    static TcpSocket connect(const char* host, short port);

    bool makeNonBlock();
    TcpSocket accept();

    int getFd() const;
    bool empty() const { return handle_ == -1; }

    int recv(const char* s, size_t size);
    int send(const char* s, size_t size);

private:
    int handle_ = -1;
    sockaddr addr_;
};