#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

inline std::string getPeerIp(SOCKET s) {
    sockaddr_in addr{};
    int addrLen = sizeof(addr);
    if (getpeername(s, reinterpret_cast<sockaddr*>(&addr), &addrLen) != 0) {
        return "unknown";
    }
    char buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, buffer, sizeof(buffer));
    return std::string(buffer);
}