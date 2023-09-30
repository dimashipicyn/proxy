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
    fd_set clientSockets;
    
    int maxSocket = listener_->getFd();

    FD_ZERO(&clientSockets);
    FD_SET(listener_->getFd(), &clientSockets);

    // Основной цикл прокси-сервера
    while (true) {
        // Создаем копию множества clientSockets
        fd_set readSockets = clientSockets;

        // Используем select для ожидания активности на сокетах
        if (select(maxSocket + 1, &readSockets, nullptr, nullptr, nullptr) == -1) {
            perror("Error select");
            break;
        }

        // Проверяем каждый клиентский сокет на активность
        for (int clientSocket = 0; clientSocket <= maxSocket; ++clientSocket) {
            if (FD_ISSET(clientSocket, &readSockets))
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
                    FD_SET(newClientFd, &clientSockets);
                    FD_SET(partnerFd, &clientSockets);

                    // Обновляем значение maxSocket, если необходимо
                    maxSocket = std::max<int>(maxSocket, newClientFd);
                    maxSocket = std::max<int>(maxSocket, partnerFd);
                    
                    auto newClientIter = sockets_.insert(sockets_.end(), std::move(newClient));
                    auto partnerIter = sockets_.insert(sockets_.end(), std::move(partner));
                    
                    clients_.emplace(newClientFd, Connection{newClientIter, partnerIter});
                    clients_.emplace(partnerFd, Connection{partnerIter, newClientIter});
                }
                // Если есть активность на клиентском сокете
                else
                {
                    auto& client = clients_.at(clientSocket);

                    int senderFd = client.sender->get()->getFd();
                    int receiverFd = client.receiver->get()->getFd();

                    char buffer[256];
                    memset(buffer, 0, sizeof(buffer));

                    // Читаем данные из клиентского сокета
                    int bytesRead = client.sender->get()->recv(buffer, sizeof(buffer));
                    if (bytesRead <= 0) {
                        // Клиент отключился или произошла ошибка, удаляем сокет из множества
                        
                        sockets_.erase(client.sender);
                        sockets_.erase(client.receiver);

                        clients_.erase(senderFd);
                        clients_.erase(receiverFd);

                        FD_CLR(senderFd, &clientSockets);
                        FD_CLR(receiverFd, &clientSockets);

                        std::cout << "Client disconnected: " << senderFd << std::endl;
                    }
                    else {
                        client.receiver->get()->send(buffer, bytesRead);
                        std::cout << std::string(buffer, bytesRead) << std::endl;
                    }
                }
            }
        }
    }
}