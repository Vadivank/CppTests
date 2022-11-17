// client.cpp
#pragma comment(lib, "ws2_32.lib");
#pragma warning(disable : 4996)
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // макрос для обхода некоторых ошибок
#include <winsock2.h>

#include <iostream>
#include <string>  // подключение библиотеки для работы со строками
#include <thread>

#define WIN(exp) exp
#define NIX(exp)

SOCKET connection;

/* packet type */
enum Packet { P_CHAT_MESSAGE, P_TEST };

bool isClosedSocket(int sock, timeval timeOut) {
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(sock, &rfd);
    select(sock + 1, &rfd, 0, 0, &timeOut);
    if (!FD_ISSET(sock, &rfd)) {
        return false;
    }

    NIX(int n = 0; ioctl(sock, FIONREAD, &n); return n == 0;)

    return true;
}
/* logic of message reception processing */
bool processPacket(Packet packetType) {
    switch (packetType) {
        case P_CHAT_MESSAGE: {
            int msgSize;
            recv(connection, (char *)&msgSize, sizeof(int), 0);
            char *msg = new char[msgSize + 1];
            msg[msgSize] = '\0';
            recv(connection, msg, msgSize, 0);
            std::cout << msg << std::endl;
            delete[] msg;
            break;
        }
        case P_TEST: {
            std::cout << "Info: Test packet." << std::endl;
            break;
        }
        default: {
            std::cout << "Warning: Unrecognized packet -> " << packetType
                      << std::endl;
            break;
        }
    }
    return true;
}
/* handler; in same thread */
void clientHandler() {
    Packet packetType;
    int servStatus;

    while (true) {
        if (!isClosedSocket(connection, {0, 100000})) {
            servStatus =
                recv(connection, (char *)&packetType, sizeof(Packet), 0);
        }

        if (servStatus < 0) break;

        if (!processPacket(packetType) && (servStatus >= 0)) {
            break;
        }
    }
    std::cout << "Server closed! Buy-buy!";
    closesocket(connection);
    Sleep(2000);
    exit(1);
}
/* a thread working on receiving messages */
void loopConnection() {
    std::string msg1{};
    while (true) {
        msg1.erase();
        std::cin.clear();
        std::getline(std::cin, msg1);
        int msgSize = msg1.size();

        if (msgSize) {
            if (!isClosedSocket(connection, {1, 0})) {
                Packet packetType = P_CHAT_MESSAGE;
                send(connection, (char *)&packetType, sizeof(Packet), 0);
                send(connection, (char *)&msgSize, sizeof(int), 0);
                send(connection, msg1.c_str(), msgSize, 0);
            }
        } else {
        }
    }
}
/* client initialization */
void connectionInit(const char *address, unsigned short port) {
    /* WSA init */
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 2);
    if (WSAStartup(DLLVersion, &wsaData)) {
        std::cout << "Error #" << WSAGetLastError()
                  << ": Application can not load WSA library!" << std::endl;
        exit(1);
    } else {
        std::cout << "WSA initialization is OK" << std::endl;
    }

    /* get addr info */
    SOCKADDR_IN addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address);  // legacy

    /* socket init */
    connection = socket(AF_INET, SOCK_STREAM, 0);
    if (connection == INVALID_SOCKET) {
        std::cout << "Error #" << WSAGetLastError()
                  << ": Can`t intialize socket!" << std::endl;
        closesocket(connection);
        WSACleanup();
        exit(1);
    } else {
        std::cout << "Client socket initialization is OK" << std::endl;
    }

    /* bind address*/
    int errorState = connect(connection, (sockaddr *)&addr, sizeof(addr));
    if (errorState != 0) {
        std::cout << "Error #" << WSAGetLastError()
                  << ": incorrect client connection to server!" << std::endl;
        closesocket(connection);
        WSACleanup();
        exit(1);
    } else {
        std::cout << "Client connection is OK" << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Need ip address: <app> <ip> <port>" << std::endl;
        Sleep(2000);
        return 1;
    } else
        std::cout << "Connection to\nIP: " << argv[1] << "\t Port: " << argv[2]
                  << std::endl;

    connectionInit(argv[1], std::stoi(std::string(argv[2])));
    
    std::thread cThread(clientHandler);
    loopConnection();
    cThread.join();

    return 0;
}
