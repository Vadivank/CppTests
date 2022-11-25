// server.cpp
#pragma comment(lib, "ws2_32.lib");
#pragma warning(disable : 4996)

#include <thread>
#include <vector>

#include "../../mysocket.h"

#define WIN(exp) exp  // быстрая обвязка макросами участков кода под ОС
#define NIX(exp)
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // для обхода некоторых ошибок
#define BACKLOG 10  // размер очереди ожидающих подключений

const int AMOUNT_CONNECTIONS = 10;
SOCKET connections[AMOUNT_CONNECTIONS];

/* тип пакета данных позволяет идентифицировать данные */
enum Packet { P_CHAT_MESSAGE, P_TEST };
bool processPacket(GetPeerIP &gpip, int index, Packet packetType);
void clientHandler(int index);
void connectionInit(const char *address, const char *port);

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "Usage: [app] [ip] [port]" << std::endl;
        Sleep(2000);
        return 1;
    }

    std::cout << "Server: " << argv[1] << ":" << argv[2] << std::endl;

    connectionInit(argv[1], argv[2]);

    // connectionInitLegacy(argv[1], std::stoi(std::string(argv[2])));  //
    // legacy

    return 0;
}
/*






*/
/* обработчик событий на каждое соединение - живет в отдельном потоке */
void clientHandler(int index) {
    Packet packetType;
    GetPeerIP gpip{GetPeerIP(connections[index])};
    std::cout << "Connected #" << index << " "
              << GetPeerIP(connections[index]).getPeerName() << std::endl;
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
    std::cout << "Disconnected #" << index << " " << gpip.getPeerName() << " "
              << gpip.ipMessage << std::endl;
    closesocket(connections[index]);
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

/* инициализация сервера */
void connectionInit(const char *address, const char *port) {
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

    int sockfd;                            // socket declaration
    int es;                                // error ststus
    struct addrinfo hints, *servinfo, *p;  // structs for address information
    memset(&hints, 0, sizeof hints);       // clean temporary struct
    hints.ai_family = PF_UNSPEC;           // automatic socket type detection
    hints.ai_socktype = SOCK_STREAM;       // TCP
    // hints.ai_flags = AI_NUMERICHOST;       // numeric value for address

    /* filling the structure with address data */
    if (getaddrinfo(address, port, &hints, &servinfo)) {
        perror("Getaddrinfo ERROR");
        exit(1);
    }

    /* loop through all available addresses and bind to the first possible one
     */
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
            -1) {
            perror("Socket initialisation ERROR");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            closesocket(sockfd);
            perror("Socket binding ERROR");
            continue;
        }
        printf("Client bind to %s\n",
               inet_ntoa(((sockaddr_in *)(p->ai_addr))->sin_addr));
        break;
    }
    if (p == NULL) {
        perror("Failed to bind socket");
        exit(1);
    }

    freeaddrinfo(servinfo);

    /* listening */
    int errorState = listen(sockfd, SOMAXCONN);
    if (errorState != 0) {
        std::cout << "Error #" << WSAGetLastError()
                  << ": incorrect start listening!" << std::endl;
        closesocket(sockfd);
        WSACleanup();
        exit(1);
    } else {
        char name[255];
        gethostname(name, 255);
        name[255] = '\0';
        std::cout << "\nHostname: " << name << " - ";
        std::cout << "Listening connections..." << std::endl;
    }

    /* loop connections */
    std::vector<std::thread> clientThreads;
    for (int i = 0; i < AMOUNT_CONNECTIONS; ++i) {
        int sizeofaddr = sizeof(sockaddr);

        SOCKET newConnection =
            accept(sockfd, (sockaddr *)&p->ai_addr, &sizeofaddr);

        if (newConnection == INVALID_SOCKET) {
            std::cout << "Error #" << WSAGetLastError() << ": client #" << i
                      << " can not connect to server!" << std::endl;
            closesocket(sockfd);
            closesocket(newConnection);
            WSACleanup();
            exit(1);
        } else {
            std::string msg = "\nClient #" + std::to_string(i) + " " +
                              GetPeerIP(newConnection).getPeerName() +
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

/* Legacy инициализация сервера */
void connectionInitLegacy(const char *address, unsigned short port) {
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
            std::string msg = "\nClient #" + std::to_string(i) + " " +
                              GetPeerIP(newConnection).getPeerName() +
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
