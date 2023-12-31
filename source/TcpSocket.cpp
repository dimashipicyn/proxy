#include "TcpSocket.h"

#include "Log.h"
#include "Utils.h"

#ifdef _WIN32
void close(int fd)
{
    closesocket(fd);
}

#pragma comment(lib, "wsock32.lib")
#pragma comment (lib, "Ws2_32.lib")
#endif

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#ifdef _WIN32
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

bool initSockets()
{
#ifdef _WIN32
    WSADATA WsaData;
    return WSAStartup(MAKEWORD(2, 2), &WsaData) == NO_ERROR;
#else
    return true;
#endif
}

void shotdownSockets()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

TcpSocket::~TcpSocket()
{
    close(handle_);
}

TcpSocket TcpSocket::listen(const char* host, short port)
{
    addrinfo* res = nullptr;
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    // По хосту и порту получаем список адресов
    hints.ai_family = AF_INET; // AF_INET или AF_INET6 если требуется
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(host, std::to_string(port).c_str(), &hints, &res) != 0)
    {
        LOG_ERROR("Getaddrinfo! Error: %s\n", getError().c_str());
        return TcpSocket();
    }

    // Пробуем создать сокет
    int handle = -1;
    int status = -1;
    sockaddr address;
    for (addrinfo* p = res; p != nullptr; p = p->ai_next)
    {
        if (handle = socket(p->ai_family, p->ai_socktype, p->ai_protocol); handle == -1)
        {
            continue;
        }

        int opt = 1;
        if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == -1)
        {
            close(handle);
            continue;
        }

        if (::bind(handle, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(handle);
            continue;
        }

        if (status = ::listen(handle, SOMAXCONN); status == -1)
        {
            close(handle);
            continue;
        }

        address = *p->ai_addr;
        break;
    }

    if (status == -1)
    {
        LOG_ERROR("Listen %s:%d failed! Error: %s\n", host, port, getError().c_str());
        return TcpSocket();
    }

    TcpSocket socket;// = std::unique_ptr<TcpSocket>(new TcpSocket);
    socket.handle_ = handle;
    socket.addr_ = address;

    return socket;
}

TcpSocket TcpSocket::connect(const char* host, short port)
{
    addrinfo* res = nullptr;
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    // По хосту и порту получаем список адресов
    hints.ai_family = AF_UNSPEC; // AF_INET или AF_INET6 если требуется
    hints.ai_socktype = SOCK_STREAM; // TCP
    if (getaddrinfo(host, std::to_string(port).c_str(), &hints, &res) != 0)
    {
        LOG_ERROR("Getaddrinfo! Error: %s\n", getError().c_str());
        return TcpSocket();
    }

    // Пробуем создать сокет
    int handle = -1;
    int status = -1;
    sockaddr address;
    for (addrinfo* p = res; p != nullptr; p = p->ai_next)
    {
        if (handle = socket(p->ai_family, p->ai_socktype, p->ai_protocol); handle == -1)
        {
            continue;
        }

        int opt = 1;
        if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == -1)
        {
            close(handle);
            continue;
        }

        if (status = ::connect(handle, p->ai_addr, p->ai_addrlen); status == -1)
        {
            close(handle);
            continue;
        }
        address = *p->ai_addr;
        break;
    }

    if (status == -1)
    {
        LOG_ERROR("Connect %s:%d failed! Error: %s\n", host, port, getError().c_str());
        return TcpSocket();
    }

    TcpSocket socket;// = std::unique_ptr<TcpSocket>(new TcpSocket);
    socket.handle_ = handle;
    socket.addr_ = address;

    return socket;
}

bool TcpSocket::makeNonBlock()
{
#ifdef _WIN32
    DWORD nonBlocking = 1;
    if (ioctlsocket(handle_, FIONBIO, &nonBlocking) != 0)
    {
        LOG_DEBUG("TcpSocket makeNonBlock failed! Error: %s\n", getError().c_str());
        return false;
    }
#else
    int flags = fcntl(handle_, F_GETFL, 0);
    if (flags == -1)
    {
        LOG_DEBUG("TcpSocket makeNonBlock failed! Error: %s\n", strerror(errno));
        return false;
    }

    if (fcntl(handle_, F_SETFL, flags | O_NONBLOCK) != 0)
    {
        LOG_DEBUG("TcpSocket makeNonBlock failed! Error: %s\n", strerror(errno));
        return false;
    }
#endif
    return true;
}

TcpSocket TcpSocket::accept()
{
    int newSocket = -1;
    sockaddr clientAddr;
    socklen_t len = sizeof(clientAddr);
    if (newSocket = ::accept(handle_, &clientAddr, &len); newSocket == -1)
    {
        LOG_DEBUG("TcpSocket accept failed! Error: %s\n", strerror(errno));
        return TcpSocket();
    }

    TcpSocket sock;// = std::unique_ptr<TcpSocket>(new TcpSocket);
    sock.handle_ = newSocket;
    sock.addr_ = clientAddr;

    LOG_DEBUG("New connection from %s port %d\n", inet_ntoa(((sockaddr_in*)&clientAddr)->sin_addr), ntohs(((sockaddr_in*)&clientAddr)->sin_port));

    return sock;
}

int TcpSocket::getFd() const
{
    return handle_;
}

int TcpSocket::recv(const char* s, size_t size)
{
    return ::recv(handle_, (char*)s, size, 0);
}

int TcpSocket::send(const char* s, size_t size)
{
    return ::send(handle_, s, size, 0);
}