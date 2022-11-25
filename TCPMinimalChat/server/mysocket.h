// https://www.winsocketdotnetworkprogramming.com/winsock2programming/winsock2advancedsocketoptionioctl7.html
#pragma comment(lib, "ws2_32.lib");
#pragma warning(disable : 4996)

#define WIN(exp) exp  // quick macro binding of code sections under the OS
#define NIX(exp)
#define HOSTNAMEMAXLENGHT 255

#ifdef __WIN32
#define __WINSOCK_DEPRECATED_NO_WARNINGS  // skipping some errors
// #include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <cstring>
#include <ctime>
#include <iostream>
#include <list>
#include <sstream>

#endif

/* текущее время */
std::ostringstream currentTime() {
    std::time_t t_time{std::time(nullptr)};
    std::tm *tm{std::localtime(&t_time)};
    std::ostringstream oss;

    oss << tm->tm_mday << "." << tm->tm_mon << "." << tm->tm_year + 1900 << " "
        << tm->tm_hour << ":";
    if (tm->tm_min >= 10)
        oss << tm->tm_min << ":";
    else
        oss << "0" << tm->tm_min << ":";
    if (tm->tm_sec >= 10)
        oss << tm->tm_sec << " ";
    else
        oss << "0" << tm->tm_sec << " ";
    return oss;
}

struct GetHostInfo {
    std::list<struct in_addr> found_ip_addresses;
    char myip[INET6_ADDRSTRLEN];
    struct in_addr ia;
    struct addrinfo *servinfo;

    GetHostInfo(const char *host_name) {
        int iResult;
        DWORD dwError;
        int i = 0;
        struct hostent *remoteHost;
        struct in_addr addr;

        /* Т.к. один хост может содержать несколько IP адресов, их нужно где-то
        хранить. Данное поле содержит массив указателей на структуры  */
        char **pAlias;

        if (isalpha(host_name[0])) {  // host address is a name
            // printf("Calling host by name with %s\n", host_name);
            remoteHost = gethostbyname(host_name);
        } else {  // host address is ip
            // printf("Calling host by address with %s\n", host_name);
            addr.s_addr = inet_addr(host_name);
            if (addr.s_addr == INADDR_NONE) {
                printf("The IPv4 address entered must be a legal address\n");
                exit(1);
            } else
                remoteHost = gethostbyaddr((char *)&addr, 4, AF_INET);
        }

        if (remoteHost == NULL) {
            dwError = WSAGetLastError();
            if (dwError != 0) {
                if (dwError == WSAHOST_NOT_FOUND) {
                    printf("Host not found\n");
                    exit(1);
                } else if (dwError == WSANO_DATA) {
                    printf("No data record found\n");
                    exit(1);
                } else {
                    printf("Function failed with error: %ld\n", dwError);
                    exit(1);
                }
            }
        } else {
            printf("Host information:\n");
            printf("\tOfficial name: %s\n", remoteHost->h_name);
            for (pAlias = remoteHost->h_aliases; *pAlias != 0; ++pAlias) {
                printf("\tAlternate name #%d: %s\n", ++i, *pAlias);
            }
            printf("\tAddress type: ");
            switch (remoteHost->h_addrtype) {
                case AF_INET:
                    printf("AF_INET\n");
                    break;
                case AF_INET6:
                    printf("AF_INET6\n");
                    break;
                case AF_NETBIOS:
                    printf("AF_NETBIOS\n");
                    break;
                default:
                    printf(" %d\n", remoteHost->h_addrtype);
                    break;
            }
            printf("\tAddress length: %d\n", remoteHost->h_length);

            if (remoteHost->h_addrtype == AF_INET) {
                while (remoteHost->h_addr_list[i] != nullptr) {
                    addr.s_addr = *(u_long *)remoteHost->h_addr_list[i++];
                    found_ip_addresses.push_back(addr);
                    printf("\tIPv4 Address #%d\n", i, inet_ntoa(addr));
                }
            } else if (remoteHost->h_addrtype == AF_INET6)
                printf("\tRemotehost is an IPv6 address\n");
        }

        memset(&ia, 0, sizeof(struct in_addr));
    }

    struct in_addr getInAddrByMask(const char *mask = nullptr) {
        if (mask != nullptr) {
            for (struct in_addr const &it_addr : found_ip_addresses) {
                if (!std::strncmp(inet_ntoa(it_addr), mask, strlen(mask))) {
                    ia = it_addr;
                    return it_addr;
                }
            }
        }
        ia = found_ip_addresses.back();
        return ia;
    }
};

/* выведет IP и PORT удаленного клиента в удобном для четния виде */
struct GetPeerIP {
   private:
    SOCKET m_socket;
    const short portStrSize{sizeof(unsigned short) + 1};  // 65535 + '\0'

   public:
    std::string ipStr;
    char ipChar[INET_ADDRSTRLEN];
    char name[HOSTNAMEMAXLENGHT];

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

        ipMessage = "(" + ipStr + ":" + portStr + ")";
    }

    char *getPeerName() {
        sockaddr_in t_sock;
        socklen_t t_len = sizeof(t_sock);
        char t_ipStr[INET_ADDRSTRLEN];
        // t_ipStr[INET_ADDRSTRLEN] = '\0';
        struct in_addr addr;
        struct hostent *remoteHost;

        getpeername(m_socket, (sockaddr *)&t_sock, &t_len);
        strncpy(t_ipStr, inet_ntoa(t_sock.sin_addr), INET_ADDRSTRLEN);
        addr.s_addr = inet_addr(t_ipStr);
        remoteHost = gethostbyaddr((char *)&addr, 4, AF_INET);
        return remoteHost->h_name;
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

/* Check self IPv4 address */
struct IPv4 {
   private:
    unsigned char b1, b2, b3, b4;

   public:
    char ipDefault[16];  // xxx.xxx.xxx.xxx\0

    IPv4() {
        char hostname[255];
        memset(hostname, 0, sizeof(hostname));
        memset(ipDefault, 0, 16);
        gethostname(hostname, sizeof(hostname));
        struct hostent *hostip = gethostbyname(hostname);
        this->b1 = ((struct in_addr *)(hostip->h_addr))->S_un.S_un_b.s_b1;
        this->b2 = ((struct in_addr *)(hostip->h_addr))->S_un.S_un_b.s_b2;
        this->b3 = ((struct in_addr *)(hostip->h_addr))->S_un.S_un_b.s_b3;
        this->b4 = ((struct in_addr *)(hostip->h_addr))->S_un.S_un_b.s_b4;
        std::ostringstream oo;
        oo << (int)this->b1 << '.' << (int)this->b2 << '.' << (int)this->b3
           << '.' << (int)this->b4 << '\0';
        strncpy(ipDefault, oo.str().c_str(), 16);
    }
};

/* WSA init (Windows only) */
void initWSA() {
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 2);
    if (WSAStartup(DLLVersion, &wsaData)) {
        std::cout << "Error #" << WSAGetLastError()
                  << ": Application can not load WSA library!" << std::endl;
        exit(1);
    } else {
        std::cout << "WSA initialization is OK" << std::endl;
    }
}

/*
minimalistic inet_ntop() equivalent on Windows
---
Данная функция преобразует структуру сетевого адреса src в строку символов с
сетевым адресом (типа af), которая затем копируется в символьный буфер dst;
размер этого буфера составляет cnt байтов.
*/
const char *inet_ntop_win(int af, const void *src, char *dst, int &cnt) {
    TCHAR ip[INET6_ADDRSTRLEN];
    int es = 1;  // error state

    if (af == AF_INET) {
        struct sockaddr_in in {};
        in.sin_family = AF_INET;
        memcpy(&in.sin_addr, src, sizeof(struct in_addr));
        es = WSAAddressToString((struct sockaddr *)&in,
                                sizeof(struct sockaddr_in), 0, ip,
                                (LPDWORD)&cnt);
    } else if (af == AF_INET6) {
        struct sockaddr_in6 in {};
        in.sin6_family = AF_INET6;
        memcpy(&in.sin6_addr, src, sizeof(struct in_addr6));
        es = WSAAddressToString((struct sockaddr *)&in,
                                sizeof(struct sockaddr_in6), 0, ip,
                                (LPDWORD)&cnt);
    }

    if (es != 0) {
        return NULL;
    }

    for (int i = 0; i < cnt + 1; ++i) {
        dst[i] = static_cast<char>(ip[i]);
    }
    dst[cnt] = '\0';

    return dst;
}

/*
minimalistic inet_aton() or inet_pton() equivalent on Windows
---
Данная функция преобразует строку символов src в сетевой адрес (типа af), затем
копирует полученную структуру с адресом в dst.
*/
int inet_pton_win(int family, const char *src, void *dst) {
    TCHAR buf[(INET6_ADDRSTRLEN + 1) * 2];
    int buflen = sizeof(buf);
    int es = 1;  // error state

    if (family == AF_INET) {
        struct sockaddr_in sockaddr;
        // mbstowcs(buf, src, sizeof(buf));
        es = WSAStringToAddress(buf, AF_INET, NULL,
                                (struct sockaddr *)&sockaddr, &buflen);
        memcpy(dst, &sockaddr.sin_addr.s_addr,
               sizeof(sockaddr.sin_addr.s_addr));
    } else if (family == AF_INET6) {
        struct sockaddr_in6 sockaddr;
        // mbstowcs(buf, src, sizeof(buf));
        es = WSAStringToAddress(buf, AF_INET6, NULL,
                                (struct sockaddr *)&sockaddr, &buflen);
        memcpy(dst, &sockaddr.sin6_addr.s6_addr,
               sizeof(sockaddr.sin6_addr.s6_addr));
    }

    return es;
}

/*
minimalistic inet_aton() or inet_pton() equivalent on Windows
---
Данная функция преобразует строку символов src в сетевой адрес (типа af), затем
копирует полученную структуру с адресом в dst.
*/
int inet_pton_win32(int af, const char *src, void *dst) {
    switch (af) {
        case AF_INET: {
            struct sockaddr_in sa;
            int len = sizeof(sockaddr);
            int ret = -1;
            int strLen = strlen(src) + 1;

#ifdef UNICODE
            wchar_t *srcNonConst = (wchar_t *)malloc(strLen * sizeof(wchar_t));
            memset(srcNonConst, 0, strLen);
            MultiByteToWideChar(CP_ACP, 0, src, -1, srcNonConst, strLen);
#else
            char *srcNonConst = (char *)malloc(strLen);
            memset(srcNonConst, 0, strLen);
            strncpy(srcNonConst, src, strLen);
#endif

            if (WSAStringToAddress(srcNonConst, af, NULL, (LPSOCKADDR)&sa,
                                   &len) == 0) {
                ret = 1;
            } else {
                if (WSAGetLastError() == WSAEINVAL) {
                    ret = -1;
                }
            }
            free(srcNonConst);
            memcpy(dst, &sa.sin_addr, sizeof(struct in_addr));
            return ret;
            break;
        }

        case AF_INET6: {
            struct sockaddr_in6 sa6;
            // struct sockaddr_in sa;
            int len = sizeof(sockaddr_in6);
            int ret = -1;
            int strLen = strlen(src) + 1;

#ifdef UNICODE
            wchar_t *srcNonConst = (wchar_t *)malloc(strLen * sizeof(wchar_t));
            memset(srcNonConst, 0, strLen);
            MultiByteToWideChar(CP_ACP, 0, src, -1, srcNonConst, strLen);
#else
            char *srcNonConst = (char *)malloc(strLen);
            memset(srcNonConst, 0, strLen);
            strncpy(srcNonConst, src, strLen);
#endif

            if (WSAStringToAddress(srcNonConst, af, NULL, (LPSOCKADDR)&sa6,
                                   &len) == 0) {
                ret = 1;
            } else {
                if (WSAGetLastError() == WSAEINVAL) {
                    ret = -1;
                }
            }
            free(srcNonConst);
            memcpy(dst, &sa6.sin6_addr, sizeof(struct in_addr));
            return ret;
            break;
        }

        default: {
            // default
        }
    }
    return 0;
}

/* get IPv4 or IPv6 family address */
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}