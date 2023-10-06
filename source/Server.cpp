#include "Server.h"
#include "Log.h"
#include "TcpSocket.h"

#include <sys/epoll.h>

#include <cstring>
#include <iostream>
#include <memory>

namespace
{
void epollAdd(int epfd, int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        LOG_ERROR("Epoll_ctl failed! Error: %s\n", strerror(errno));
    }
}

void epollMod(int epfd, int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        LOG_ERROR("Epoll_ctl failed! Error: %s\n", strerror(errno));
    }
}
}

Server::Server(const char* host, short port, const char* dbHost, short dbPort)
    : dbHost_{ dbHost }
    , dbPort_{ dbPort }
{
    if (initSockets())
    {
        if (listener_ = TcpSocket::listen(host, port); !listener_.empty())
        {
            isOk_ = listener_.makeNonBlock();
        }
    }
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
        LOG_ERROR("Epoll failed! Error: %s\n", strerror(errno));
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
                    while (!newClient.empty())
                    {
                        newClient.makeNonBlock();
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
    if (partner.empty())
    {
        return;
    }

    partner.makeNonBlock();
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
    if (size < 0)
    {
        // Клиент отключился или произошла ошибка, удаляем
        onError(epollfd, clientSocket);
    }
    else
    {
        epollMod(epollfd, partnerFd, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLERR);
        conn.data.append(buffer, size);

        conn.pgParser.parse(buffer, size);
        auto packets = conn.pgParser.getPackets();
        for (auto& p : packets)
        {
            if (p.type == 'Q')
            {
                LOG_ERROR("Query: %s\n", p.data.c_str());
            }
        }
    }
}

void Server::onWrite(int epollfd, int clientSocket)
{
    auto& conn = connections_.at(clientSocket);
    auto& connPartner = connections_.at(conn.partner->getFd());;

    int size = conn.client->send(connPartner.data.c_str(), connPartner.data.size());
    if (size < 0)
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