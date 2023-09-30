#include "Tcpsocket.h"
#include <memory>

bool initSockets()
{
#ifdef _WIN32
    WSADATA WsaData;
    return WSAStartup(MAKEWORD(2,2), &WsaData) == NO_ERROR;
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

#ifdef _WIN32
    void close(int fd)
    {
        closesocket(fd);
    }
#endif

TcpSocketPtr TcpSocket::create()
{
    return std::unique_ptr<TcpSocket>(new TcpSocket);
}

TcpSocket::~TcpSocket()
{
    close(handle_);
}


bool TcpSocket::open(const char* host, short port)
{
    int handle = -1;
    if (handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); handle < 0)
    {
        return false;
    }

    int opt = 1;
    if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt))) {
        close(handle);
        return false;
    }

    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = inet_addr(host);
    addr_.sin_port = htons(port);

    handle_ = handle;

    return true;
}

bool TcpSocket::makeNonBlock()
{
#ifdef _WIN32
    DWORD nonBlocking = 1;
    if (ioctlsocket( handle_, FIONBIO, &nonBlocking) != 0)
    {
        return false;
    }
#else
    int nonBlocking = 1;
    if (fcntl(handle, F_SETFL, O_NONBLOCK, nonBlocking) != 0)
    {
        return false;
    }
#endif
    return true;
}

bool TcpSocket::listen()
{
    if (::bind(handle_, (const sockaddr*)&addr_, sizeof(addr_)) == -1)
    {
        close(handle_);
        return false;
    }

    if (::listen(handle_, SOMAXCONN) == -1) {
        return false;
    }
    return true;
}

bool TcpSocket::connect() {
    if (::connect(handle_, (const sockaddr*)&addr_, sizeof(addr_)) == -1) {
        return false;
    }
    return true;
}

TcpSocketPtr TcpSocket::accept()
{
    int newSocket = -1;
    if (newSocket = ::accept(handle_, (struct sockaddr*)&addr_, (int[]){sizeof(addr_)}); newSocket == -1) {
        return nullptr;
    }

    auto sock = create();
    sock->handle_ = newSocket;
    sock->addr_ = addr_;

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