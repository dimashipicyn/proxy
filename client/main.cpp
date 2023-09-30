#include <iostream>
#include <cstring>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    // Инициализация библиотеки Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка при инициализации Winsock" << std::endl;
        return 1;
    }

    // Создаем соксет для клиента
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Ошибка при создании клиентского соксета" << std::endl;
        WSACleanup();
        return 1;
    }

    // Настраиваем удаленный сервер
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5432);  // Порт сервера (замените на нужный вам порт)
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // IP-адрес сервера (замените на нужный вам IP)

    // Попытка установить соединение с сервером
    if (connect(clientSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "error connection " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "connection established" << std::endl;

    // Теперь вы можете использовать clientSocket для отправки и получения данных с сервера

    // Пример отправки данных на сервер
    const char* message = "helllp!";
    send(clientSocket, message, strlen(message), 0);

    // Пример чтения данных с сервера
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead == SOCKET_ERROR) {
        std::cerr << "error server read " << WSAGetLastError() << std::endl;
    } else if (bytesRead == 0) {
        std::cout << "server disconnect" << std::endl;
    } else {
        std::cout << "received data " << buffer << std::endl;
    }

    // Закрываем клиентский соксет и освобождаем ресурсы Winsock
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
