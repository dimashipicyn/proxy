#include "Server.h"
#include "Log.h"
#include "TcpSocket.h"

#include <cstring>
#include <iostream>
#include <memory>

Server::Server(const char* host, short port, const char* dbHost, short dbPort)
    : dbHost_{ dbHost }
    , dbPort_{ dbPort }
{
    if (initSockets())
    {
        if (listener_ = TcpSocket::listen(host, port); listener_)
        {
            isOk_ = listener_->makeNonBlock();
        }
    }
}

void Server::run()
{
    if (!isOk_)
    {
        return;
    }

    fd_set readSockets;
    fd_set writeSockets;

    maxSocket_ = listener_->getFd();
    int startSocket = maxSocket_;

    FD_ZERO(&readSockets);
    FD_ZERO(&writeSockets);
    FD_SET(listener_->getFd(), &readSockets);

    // Основной цикл прокси-сервера
    while (true)
    {
        // Создаем копию множества clientSockets
        fd_set rs = readSockets;
        fd_set ws = writeSockets;

        // Используем select для ожидания активности на сокетах
        if (select(maxSocket_ + 1, &rs, &ws, nullptr, nullptr) == -1)
        {
            LOG_ERROR("Select error: %s\n", strerror(errno));
            break;
        }

        // Проверяем каждый клиентский сокет на активность
        for (int clientSocket = startSocket; clientSocket <= maxSocket_; ++clientSocket)
        {
            if (FD_ISSET(clientSocket, &rs))
            {
                // Если новое клиентское подключение
                if (isListener(clientSocket))
                {
                    LOG_DEBUG("Accept socket: %d\n", clientSocket);
                    onAccept(readSockets);
                }
                // Если есть активность на клиентском сокете
                else
                {
                    if (clients_.count(clientSocket) == 0)
                    {
                        continue;
                    }
                    LOG_DEBUG("Read socket: %d\n", clientSocket);
                    onRead(clientSocket, readSockets, writeSockets);
                }
            }
            if (FD_ISSET(clientSocket, &ws))
            {
                if (clients_.count(clientSocket) == 0)
                {
                    continue;
                }

                LOG_DEBUG("Write socket: %d\n", clientSocket);
                onWrite(clientSocket, readSockets, writeSockets);
            }
        }
    }
}

void Server::onAccept(fd_set& readSockets)
{
    LOG_DEBUG("New client on listener: %d\n", listener_->getFd());

    auto newClient = listener_->accept();
    if (!newClient)
    {
        return;
    }

    newClient->makeNonBlock();
    int newClientFd = newClient->getFd();

    auto partner = TcpSocket::connect(dbHost_, dbPort_);
    if (!partner)
    {
        return;
    }

    partner->makeNonBlock();
    int partnerFd = partner->getFd();

    // Добавляем новый клиентский сокет в множество
    FD_SET(newClientFd, &readSockets);
    FD_SET(partnerFd, &readSockets);

    // Обновляем значение maxSocket, если необходимо
    maxSocket_ = std::max<int>(maxSocket_, newClientFd);
    maxSocket_ = std::max<int>(maxSocket_, partnerFd);

    auto newClientIter = sockets_.insert(sockets_.end(), std::move(newClient));
    auto partnerIter = sockets_.insert(sockets_.end(), std::move(partner));

    clients_.emplace(newClientFd,
        Connection { newClientIter, partnerIter, "" });
    clients_.emplace(partnerFd,
        Connection { partnerIter, newClientIter, "" });

    LOG_DEBUG("Connected: %d - %d\n", newClientFd, partnerFd);
}

void Server::onRead(int clientSocket, fd_set& readSockets, fd_set& writeSockets)
{
    auto& client = clients_.at(clientSocket);

    int fromFd = client.from->get()->getFd();
    int toFd = client.to->get()->getFd();

    LOG_DEBUG("Read: %d - %d\n", fromFd, toFd);

    char buffer[1024];
    int size = 0;
    memset(buffer, 0, sizeof(buffer));

    // Читаем данные из клиентского сокета
    size = client.from->get()->recv(buffer, sizeof(buffer));
    if (size <= 0)
    {
        // Клиент отключился или произошла ошибка, удаляем сокет из
        // множества
        sockets_.erase(client.from);
        sockets_.erase(client.to);

        clients_.erase(fromFd);
        clients_.erase(toFd);

        FD_CLR(fromFd, &readSockets);
        FD_CLR(toFd, &readSockets);
        FD_CLR(fromFd, &writeSockets);
        FD_CLR(toFd, &writeSockets);

        LOG_DEBUG("Diconnect: %d - %d\n", fromFd, toFd);
    }
    else
    {
        FD_SET(toFd, &writeSockets);
        client.data.append(buffer, size);
    }
}

void Server::onWrite(int clientSocket, fd_set& readSockets, fd_set& writeSockets)
{
    auto& toClient = clients_.at(clientSocket);
    auto& fromClient = clients_.at(toClient.to->get()->getFd());

    int fromFd = toClient.from->get()->getFd();
    int toFd = toClient.to->get()->getFd();

    int size = toClient.from->get()->send(fromClient.data.c_str(), fromClient.data.size());
    if (size <= 0)
    {
        // Клиент отключился или произошла ошибка, удаляем сокет из множества
        sockets_.erase(toClient.from);
        sockets_.erase(toClient.to);

        clients_.erase(fromFd);
        clients_.erase(toFd);

        FD_CLR(fromFd, &readSockets);
        FD_CLR(toFd, &readSockets);
        FD_CLR(fromFd, &writeSockets);
        FD_CLR(toFd, &writeSockets);

        LOG_DEBUG("Disconnect: %d - %d\n", fromFd, toFd);
    }
    else
    {
        fromClient.data.erase(0, size);
        if (fromClient.data.empty())
        {
            FD_CLR(clientSocket, &writeSockets);
        }
    }
}