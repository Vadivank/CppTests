// server.cpp
#pragma comment(lib, "ws2_32.lib");
#pragma warning(disable : 4996)

#define WIN(exp) exp  // быстрая обвязка макросами участков кода под ОС
#define NIX(exp)

#include <winsock2.h>

#ifdef __WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // для обхода некоторых ошибок
#include <windows.h>
#include <ws2tcpip.h>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// #include "common.h"

// bbej
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netdb.h>
// #include <arpa/inet.h>
// #include <sys/wait.h>
#include <signal.h>
// bbej

#ifndef __WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdexcept>
#endif

#endif

#ifdef __linux__
// #include <cstdlib>
// #include <cstring>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
#endif

const int AMOUNT_CONNECTIONS = 10;
SOCKET connections[AMOUNT_CONNECTIONS];
#define BACKLOG 10  // размер очереди ожидающих подключений

/* тип пакета данных позволяет идентифицировать данные */
enum Packet { P_CHAT_MESSAGE, P_TEST };

/* позволит вывести IP и PORT в удобном для четния виде */
struct GetPeerIP {
   private:
    SOCKET m_socket;
    const short portStrSize{sizeof(unsigned short) + 1};  // 65535 + '\0'

   public:
    std::string ipStr;
    char ipChar[INET_ADDRSTRLEN];

    unsigned short port;
    std::string portStr;
    char *portChar = new char[portStrSize];

    std::string ipMessage;

    int errorState;
    std::string errorMsg;

    GetPeerIP(SOCKET peerSocket) : m_socket(peerSocket) {
        sockaddr_in t_sock;
        socklen_t t_len = sizeof(t_sock);
        memset(&t_sock, 0, t_len);

        if ((errorState =
                 getpeername(peerSocket, (sockaddr *)&t_sock, &t_len))) {
            errorMsg = "GetPeerIP error #" + std::to_string(errorState);
        } else {
            errorMsg = "OK";
        }

        ipStr = inet_ntoa(t_sock.sin_addr);
        strncpy(ipChar, ipStr.c_str(), INET_ADDRSTRLEN);

        unsigned short port(ntohs(t_sock.sin_port));
        portStr = std::to_string(port);
        strncpy(portChar, portStr.c_str(), portStrSize);

        ipMessage = "(IP: " + ipStr + ":" + portStr + ")";
    }

    ~GetPeerIP() { delete[] portChar; }
};

/* сокет готов к четнию? <timeOut> - время ожидания ответа от сокета */
bool isClosedSocket(SOCKET sock, timeval timeOut = {0, 0}) {
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(sock, &rfd);

    if ((timeOut.tv_sec == -1) || (timeOut.tv_usec == -1)) {
        /* вернуть rfd, как только в нем, что-то появится */
        select(sock + 1, &rfd, 0, 0, NULL);
    } else {
        /* ждать наполнение rfd в течение timeOut */
        select(sock + 1, &rfd, 0, 0, &timeOut);
    }
    /* проверить rfd на наличие в нем sock*/
    if (!FD_ISSET(sock, &rfd)) {
        return false;
    }
    NIX(int n = 0; ioctl(sock, FIONREAD, &n); return n == 0;)
    return true;
}

/* текущее время */
std::ostringstream currentTime() {
    std::time_t t_time{std::time(nullptr)};
    std::tm *tm{std::localtime(&t_time)};
    std::ostringstream oss;

    oss << tm->tm_mday << "." << tm->tm_mon << "." << tm->tm_year + 1900 << " "
        << tm->tm_hour << ":";
    if (tm->tm_min > 10)
        oss << tm->tm_min << ":";
    else
        oss << "0" << tm->tm_min << ":";
    if (tm->tm_sec > 10)
        oss << tm->tm_sec << " ";
    else
        oss << "0" << tm->tm_sec << " ";
    return oss;
}

/* логика обработки сообщения */
bool processPacket(GetPeerIP &gpip, int index, Packet packetType) {
    switch (packetType) {
        case P_CHAT_MESSAGE: {
            int msgSize;
            recv(connections[index], (char *)&msgSize, sizeof(int), 0);
            std::cout << msgSize << " bytes\n";

            char *msg = new char[msgSize + 1];
            msg[msgSize] = '\0';
            recv(connections[index], msg, msgSize, 0);

            std::string t_addtext = "=> ";
            std::string t_climsg = std::string(msg);
            std::string t_ct{currentTime().str()};

            std::string servMsg(t_ct + gpip.ipMessage + t_addtext + t_climsg);
            int servMsgSize = gpip.ipMessage.size() + t_addtext.size() +
                              msgSize + t_ct.size();
            char *sMsg = new char[servMsgSize + 1];
            sMsg[servMsgSize] = '\0';
            strncpy(sMsg, servMsg.c_str(), servMsgSize);

            std::cout << t_ct << "---" << std::endl;
            for (int i = 0; i < AMOUNT_CONNECTIONS; ++i) {
                if (i == index) {
                    continue;
                }

                Packet msgType = P_CHAT_MESSAGE;

                if (!isClosedSocket(connections[i], {0, 0}) && (msgSize > 0)) {
                    std::cout << "Send packets to client #" << i << " "
                              << GetPeerIP(connections[i]).ipMessage << ": ";
                    std::cout << "Pcket type: "
                              << send(connections[i], (char *)&msgType,
                                      sizeof(Packet), 0)
                              << "b => ";
                    std::cout << "Msg size: "
                              << send(connections[i], (char *)&servMsgSize,
                                      sizeof(int), 0)
                              << "b => ";
                    std::cout << "Message: "
                              << send(connections[i], sMsg, servMsgSize, 0)
                              << "b" << std::endl;
                }
            }
            delete[] msg;
            delete[] sMsg;
            break;
        }
        case P_TEST: {
            break;
        }
        default: {
            std::cout << "Warning: Unrecognized packet -> " << packetType
                      << std::endl;
            exit(1);
            break;
        }
    }

    return true;
}

/* обработчик событий на каждое соединение - живет в отдельном потоке */
void clientHandler(int index) {
    Packet packetType;
    GetPeerIP gpip{GetPeerIP(connections[index])};
    std::cout << "Client #" << index << " CONNECTED" << std::endl;
    int recvBytes{0};

    while (true) {
        recvBytes =
            recv(connections[index], (char *)&packetType, sizeof(Packet), 0);

        if (recvBytes < 0) {
            break;
        } else {
            std::cout << "=========================\n"
                      << "Received data packet from client #" << index << " "
                      << gpip.ipMessage << " ";
        }

        switch (packetType) {
            case P_CHAT_MESSAGE: {
                std::cout << "[Chat message] ";
                if (!processPacket(gpip, index, packetType)) {
                    std::cout << "cH: unrecognized message!\n";
                    break;
                }
                break;
            }
            case P_TEST: {
                std::cout << "[Test message]\n";
                break;
            }
            default: {
                std::cout << "[Undefined message]\n";
                break;
            }
        }

        if (isClosedSocket(connections[index], {0, 10000})) {
            std::cout << "cH: client #" << index << " not ready-to-read\n";
            break;
        }
    }
    std::cout << "Client #" << index << " " << gpip.ipMessage << " closed!"
              << std::endl;
    closesocket(connections[index]);
}

/* инициализация сервера */
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
    SOCKET sListen = socket(AF_INET, SOCK_STREAM, 0);
    if (sListen == INVALID_SOCKET) {
        std::cout << "Error #" << WSAGetLastError()
                  << ": Can`t intialize socket!" << std::endl;
        closesocket(sListen);
        WSACleanup();
        exit(1);
    } else {
        std::cout << "Server socket initialization is OK" << std::endl;
    }

    /* bind address*/
    int errorState = bind(sListen, (sockaddr *)&addr, sizeof(addr));
    if (errorState != 0) {
        std::cout << "Error #" << WSAGetLastError()
                  << ": incorrect binding server socket!" << std::endl;
        closesocket(sListen);
        WSACleanup();
        exit(1);
    } else {
        std::cout << "Server socket binding is OK" << std::endl;
    }

    /* listening */
    errorState = listen(sListen, SOMAXCONN);
    if (errorState != 0) {
        std::cout << "Error #" << WSAGetLastError()
                  << ": incorrect start listening!" << std::endl;
        closesocket(sListen);
        WSACleanup();
        exit(1);
    } else {
        char name[255];
        gethostname(name, 255);
        name[255] = '\0';
        std::cout << "\nHostname: " << name << " - ";
        // std::cout << "\nHostname: " << name << " (" <<
        // inet_ntoa(addr.sin_addr)
        //           << ") - ";
        std::cout << "Listening connections..." << std::endl;
    }

    /* loop connections */
    std::vector<std::thread> clientThreads;
    for (int i = 0; i < AMOUNT_CONNECTIONS; ++i) {
        int sizeofaddr = sizeof(addr);

        SOCKET newConnection = accept(sListen, (sockaddr *)&addr, &sizeofaddr);

        if (newConnection == INVALID_SOCKET) {
            std::cout << "Error #" << WSAGetLastError() << ": client #" << i
                      << " can not connect to server!" << std::endl;
            closesocket(sListen);
            closesocket(newConnection);
            WSACleanup();
            exit(1);
        } else {
            std::string msg = "\nClient #" + std::to_string(i) +
                              GetPeerIP(newConnection).ipMessage +
                              ", welcome to server!\n";
            int msgSize = msg.size();
            Packet msgType = P_CHAT_MESSAGE;
            send(newConnection, (char *)&msgType, sizeof(Packet), 0);
            send(newConnection, (char *)&msgSize, sizeof(int), 0);
            send(newConnection, msg.c_str(), msgSize, 0);
            /* Отправка клиенту тестового сообщения */
            // Packet testPacket = P_TEST;
            // send(newConnection, (char *)&testPacket, sizeof(Packet), 0);
        }
        connections[i] = newConnection;
        clientThreads.emplace_back(clientHandler, i);
    }
    for (int i = 0; i < AMOUNT_CONNECTIONS; ++i) {
        clientThreads[i].join();
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Need ip address: <app> <ip> <port>" << std::endl;
        Sleep(2000);
        return 1;
    }

    std::cout << "Server\nIP: " << argv[1] << "\t Port: " << argv[2]
              << std::endl;

    connectionInit(argv[1], std::stoi(std::string(argv[2])));

    return 0;
}
