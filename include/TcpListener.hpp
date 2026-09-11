#pragma once
#include "Socket.hpp"
#include <cstdint>

//Encapsula o logica de criacao, e tranforma em porta TCP
class TcpListener {
    public:
        explicit TcpListener(uint16_t port);

        //bloqueia ate uma conexao e retorna o socket do cliente
        Socket accept();

    private:
    Socket socket_;
    uint16_t port_;

    void bindAndListen();
};