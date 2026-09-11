#include "TcpListener.hpp"
#include <ws2tcpip.h>
#include <stdexcept>

TcpListener::TcpListener(uint16_t port)
    : socket_(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)), port_(port)
    {
        if (!socket_.isValid()){
            throw std::runtime_error("Falha ao criar o socket do listener");
        }
        bindAndListen();
    }

void TcpListener::bindAndListen() {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(socket_.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        throw std::runtime_error("Falha no bind na porta " + std::to_string(port_));
    }

    if (listen(socket_.get(), SOMAXCONN) == SOCKET_ERROR) {
        throw std::runtime_error("Falha no listen");
    }
}

Socket TcpListener::accept() {
    SOCKET clientHandle = ::accept(socket_.get(), nullptr, nullptr);
    if (clientHandle == INVALID_SOCKET) {
        throw std::runtime_error("Falha no accept");
    }
    return Socket(clientHandle); // move implicito no retorno
}