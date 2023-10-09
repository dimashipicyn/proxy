#include "Server.h"
#include "Log.h"
#include "TcpSocket.h"
#include "Utils.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <unordered_map>

#ifdef __linux__
#include <sys/epoll.h>
#else

#ifdef __APPLE__
#include <sys/select.h>
typedef int SOCKET;
#else
#include <winsock2.h>
#endif

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_MOD 2
#define EPOLL_CTL_DEL 3

#define EPOLLIN (1 << 1)
#define EPOLLOUT (1 << 2)
#define EPOLLHUP (1 << 3)
#define EPOLLRDHUP (1 << 4)
#define EPOLLERR (1 << 5)

union epoll_data
{
    void* ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
};

struct epoll_event
{
    uint32_t events; /* Epoll events */
    epoll_data data; /* User data variable */
};

namespace
{
fd_set readSockets;
fd_set writeSockets;
fd_set exceptSockets;
int maxSocket = -1;
std::unordered_map<int, epoll_data> epollData;
}

int epoll_create1(int /*flags*/)
{
    FD_ZERO(&readSockets);
    FD_ZERO(&writeSockets);
    return 0;
}

int epoll_ctl(int /*flags*/, int action, int fd, epoll_event* event)
{
    maxSocket = std::max<int>(maxSocket, fd);
    if (event)
    {
        epollData[fd] = event->data;
    }

    switch (action)
    {
    case EPOLL_CTL_ADD:
    case EPOLL_CTL_MOD:
    {
        if (!event)
        {
            break;
        }

        FD_CLR((SOCKET)fd, &readSockets);
        FD_CLR((SOCKET)fd, &writeSockets);
        FD_CLR((SOCKET)fd, &exceptSockets);

        if (event->events & EPOLLIN)
        {
            FD_SET((SOCKET)fd, &readSockets);
        }
        if (event->events & EPOLLOUT)
        {
            FD_SET((SOCKET)fd, &writeSockets);
        }
        if (event->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
        {
            FD_SET((SOCKET)fd, &exceptSockets);
        }
        break;
    }
    case EPOLL_CTL_DEL:
    {
        if (!event)
        {
            FD_CLR((SOCKET)fd, &readSockets);
            FD_CLR((SOCKET)fd, &writeSockets);
            FD_CLR((SOCKET)fd, &exceptSockets);
            break;
        }
        if (event->events & EPOLLIN)
        {
            FD_CLR((SOCKET)fd, &readSockets);
        }
        if (event->events & EPOLLOUT)
        {
            FD_CLR((SOCKET)fd, &writeSockets);
        }
        if (event->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
        {
            FD_CLR((SOCKET)fd, &exceptSockets);
        }
        break;
    }
    default:
        assert(false);
    }
    return 0;
}

int epoll_wait(int /*epollfd*/, epoll_event* events, int maxEvents, int timeout)
{
    fd_set rs = readSockets, ws = writeSockets, es = exceptSockets;
    timeval tv, *tvp = (timeout > 0 ? &tv : nullptr);
    tv.tv_usec = timeout * 1000000;
    if (select(maxSocket + 1, &rs, &ws, &es, tvp) == -1)
    {
        return -1;
    }

    int nfds = 0;
    for (int i = 0; i <= maxSocket && nfds < maxEvents; i++)
    {
        events[nfds].events = 0;
        if (FD_ISSET(i, &rs))
        {
            events[nfds].events |= EPOLLIN;
        }
        if (FD_ISSET(i, &ws))
        {
            events[nfds].events |= EPOLLOUT;
        }
        if (FD_ISSET(i, &es))
        {
            events[nfds].events |= (EPOLLERR | EPOLLHUP | EPOLLRDHUP);
        }
        if (events[nfds].events)
        {
            events[nfds].data = epollData[i];
            ++nfds;
        }
    }

    return nfds;
}

#endif

namespace
{
void epollAdd(int epfd, int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        LOG_ERROR("Epoll_ctl failed! Error: %s\n", getError().c_str());
    }
}

void epollMod(int epfd, int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        LOG_ERROR("Epoll_ctl failed! Error: %s\n", getError().c_str());
    }
}
}

Server::Server(const char* host, short port, const char* dbHost, short dbPort)
    : dbHost_ { dbHost }
    , dbPort_ { dbPort }
{
    if (initSockets())
    {
        if (listener_ = TcpSocket::listen(host, port); !listener_.empty())
        {
            isOk_ = listener_.makeNonBlock();
        }
    }

    isOk_ &= openSqlLogFile();
}

void Server::run()
{
    if (!isOk_)
    {
        return;
    }

    const int MAX_EVENTS = 32;
    epoll_event events[MAX_EVENTS];

    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        LOG_ERROR("Epoll failed! Error: %s\n", getError().c_str());
        return;
    }

    epollAdd(epollfd, listener_.getFd(), EPOLLIN);

    // Основной цикл прокси-сервера
    while (true)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++)
        // Проверяем каждый сокет на активность
        {
            int clientSocket = events[i].data.fd;
            // Read
            if (events[i].events & EPOLLIN)
            {
                // Если новое соединение
                if (isListener(clientSocket))
                {
                    LOG_DEBUG("Accept socket: %d\n", events[i].data.fd);
                    TcpSocket newClient = listener_.accept();
                    while (!newClient.empty() && newClient.makeNonBlock())
                    {
                        // Принимаем соединения
                        onAccept(epollfd, std::move(newClient));
                        newClient = listener_.accept();
                    }
                }
                // Если есть активность на клиентском сокете
                else
                {
                    if (connections_.count(clientSocket) == 0)
                    {
                        continue;
                    }

                    LOG_DEBUG("Read socket: %d\n", clientSocket);
                    onRead(epollfd, clientSocket);
                }
            }
            // Write
            if (events[i].events & EPOLLOUT)
            {
                if (connections_.count(clientSocket) == 0)
                {
                    continue;
                }

                LOG_DEBUG("Write socket: %d\n", clientSocket);
                onWrite(epollfd, clientSocket);
            }
            // Disconnect
            if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                if (connections_.count(clientSocket) == 0)
                {
                    continue;
                }

                // Клиент отключился или произошла ошибка, удаляем
                onError(epollfd, clientSocket);
            }
        }
    }
}

void Server::onAccept(int epollfd, TcpSocket&& newClient)
{
    int newClientFd = newClient.getFd();

    auto partner = TcpSocket::connect(dbHost_, dbPort_);
    if (partner.empty() || !partner.makeNonBlock())
    {
        return;
    }

    int partnerFd = partner.getFd();

    // Добавляем новый клиентский сокет в множество
    epollAdd(epollfd, newClientFd, EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR);
    epollAdd(epollfd, partnerFd, EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR);

    auto newClientIter = sockets_.insert(sockets_.end(), std::move(newClient));
    auto partnerIter = sockets_.insert(sockets_.end(), std::move(partner));

    connections_.emplace(newClientFd, Connection { newClientIter, partnerIter, "", {} });
    connections_.emplace(partnerFd, Connection { partnerIter, newClientIter, "", {} });

    LOG_DEBUG("Connected: %d - %d\n", newClientFd, partnerFd);
}

void Server::onRead(int epollfd, int clientSocket)
{
    auto& conn = connections_.at(clientSocket);

    int partnerFd = conn.partner->getFd();
    LOG_DEBUG("Read: %d - %d\n", clientSocket, partnerFd);

    char buffer[BUFFER_SIZE];

    // Читаем данные из клиентского сокета
    int size = conn.client->recv(buffer, sizeof(buffer));
    if (size <= 0)
    {
        // Клиент отключился или произошла ошибка, удаляем
        onError(epollfd, clientSocket);
    }
    else
    {
        epollMod(epollfd, partnerFd, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLERR);
        conn.data.append(buffer, size);

        try
        {
            processPacket(conn.pgParser, buffer, size);
        }
        catch (std::bad_alloc& e)
        {
            LOG_ERROR("Error: %s\n", e.what());
            onError(epollfd, clientSocket);
        }
    }
}

void Server::onWrite(int epollfd, int clientSocket)
{
    auto& conn = connections_.at(clientSocket);
    auto& connPartner = connections_.at(conn.partner->getFd());

    int size = conn.client->send(connPartner.data.c_str(), connPartner.data.size());
    if (size <= 0)
    {
        // Клиент отключился или произошла ошибка, удаляем
        onError(epollfd, clientSocket);
    }
    else
    {
        connPartner.data.erase(0, size);
        if (connPartner.data.empty())
        {
            epollMod(epollfd, clientSocket, EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR);
        }
    }
}

void Server::onError(int epollfd, int clientSocket)
{
    auto& conn = connections_.at(clientSocket);

    int clientFd = conn.client->getFd();
    int partnerFd = conn.partner->getFd();

    // Клиент отключился или произошла ошибка, удаляем
    epoll_ctl(epollfd, EPOLL_CTL_DEL, clientFd, NULL);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, partnerFd, NULL);

    sockets_.erase(conn.client);
    sockets_.erase(conn.partner);

    connections_.erase(clientFd);
    connections_.erase(partnerFd);

    LOG_DEBUG("Disconnect: %d - %d\n", clientFd, partnerFd);
}

bool Server::openSqlLogFile()
{
    std::string filename = "sql_log_" + getTimeStamp() + ".txt";
    sqlLogFile_.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!sqlLogFile_.is_open())
    {
        LOG_ERROR("Failed open or create file: '%s' Error: %s\n", filename.c_str(), getError().c_str());
    }

    return sqlLogFile_.is_open();
}

void Server::processPacket(PostgresqlParser& pgParser, const char* buffer, int size)
{
    pgParser.parse(buffer, size);
    const auto packets = pgParser.getPackets();
    for (const auto& p : packets)
    {
        if (p.type == 'Q')
        {
            LOG_DEBUG("Query: %s\n", p.data.c_str());
            sqlLogFile_ << p.data << std::endl;
        }
    }
}