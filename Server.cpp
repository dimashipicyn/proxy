#include "Server.h"
#include <memory>
#include <iostream>

Server::Server(const char* host, short port)
{
    listener_ = TcpSocket::create();

    if (initSockets())
    {
        listener_->open(host, port);
        listener_->makeNonBlock();
        listener_->listen();
    }
}

void Server::run()
{
    fd_set readSockets;
    fd_set writeSockets;
    
    int maxSocket = listener_->getFd();
    int startSocket = maxSocket;

    FD_ZERO(&readSockets);
    FD_ZERO(&writeSockets);
    FD_SET(listener_->getFd(), &readSockets);

    // Основной цикл прокси-сервера
    while (true) {
        // Создаем копию множества clientSockets
        fd_set rs = readSockets;
        fd_set ws = writeSockets;

        // Используем select для ожидания активности на сокетах
        if (select(maxSocket + 1, &rs, &ws, nullptr, nullptr) == -1) {
            break;
        }

        // Проверяем каждый клиентский сокет на активность
        for (int clientSocket = startSocket; clientSocket <= maxSocket; ++clientSocket) {
            if (FD_ISSET(clientSocket, &rs))
            {
                // Если новое клиентское подключение
                if (isListener(clientSocket))
                {
                    auto newClient = listener_->accept();
                    newClient->makeNonBlock();
                    int newClientFd = newClient->getFd();

                    auto partner = TcpSocket::create();
                    if (!partner->open("127.0.0.1", 5432))
                    {
                        continue;
                    }
                    partner->connect();
                    partner->makeNonBlock();
                    int partnerFd = partner->getFd();

                    // Добавляем новый клиентский сокет в множество
                    FD_SET(newClientFd, &readSockets);
                    FD_SET(partnerFd, &readSockets);

                    // Обновляем значение maxSocket, если необходимо
                    maxSocket = std::max<int>(maxSocket, newClientFd);
                    maxSocket = std::max<int>(maxSocket, partnerFd);
                    
                    auto newClientIter = sockets_.insert(sockets_.end(), std::move(newClient));
                    auto partnerIter = sockets_.insert(sockets_.end(), std::move(partner));
                    
                    clients_.emplace(newClientFd, Connection{newClientIter, partnerIter});
                    clients_.emplace(partnerFd, Connection{partnerIter, newClientIter});

                    std::cout << "Client connected: " << newClientFd << "-" << partnerFd << std::endl;
                }
                // Если есть активность на клиентском сокете
                else
                {
                    if (clients_.count(clientSocket) > 0)
                    {
                        auto& client = clients_.at(clientSocket);

                        int fromFd = client.from->get()->getFd();
                        int toFd = client.to->get()->getFd();

                        char buffer[1024];
                        int size = 0;
                        memset(buffer, 0, sizeof(buffer));

                        // Читаем данные из клиентского сокета
                        size = client.from->get()->recv(buffer, sizeof(buffer));
                        if (size <= 0)
                        {
                            // Клиент отключился или произошла ошибка, удаляем сокет из множества
                            sockets_.erase(client.from);
                            sockets_.erase(client.to);

                            clients_.erase(fromFd);
                            clients_.erase(toFd);

                            FD_CLR(fromFd, &readSockets);
                            FD_CLR(toFd, &readSockets);
                            FD_CLR(fromFd, &writeSockets);
                            FD_CLR(toFd, &writeSockets);

                            std::cout << "Client disconnected: " << fromFd << std::endl;
                            continue;
                        }
                        else
                        {
                            FD_SET(toFd, &writeSockets);
                            client.data.append(buffer, size);
                        }
                    }
                }
            }
            if (FD_ISSET(clientSocket, &ws))
            {
                if (clients_.count(clientSocket) == 0)
                    continue;

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

                    std::cout << "Client disconnected: " << clientSocket << std::endl;
                }
                else
                {
                    fromClient.data.erase(0, size);
                    if (fromClient.data.empty())
                    {
                        FD_CLR(clientSocket, &writeSockets);
                    }
                }
                //std::cout << std::string(buffer, bytesRead) << std::endl;
            }
        }
    }
}