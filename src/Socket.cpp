#include "Socket.hpp"

Socket::Socket(SOCKET handle) : handle_(handle) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : handle_(other.handle_){
    other.handle_ = INVALID_SOCKET;
}

Socket& Socket::operator=(Socket&& other) noexcept{
    if (this != &other){
        close(); //fecha o que este objeto tinha
        handle_ = other.handle_;
        other.handle_ = INVALID_SOCKET;
    }
    return *this;
}

void Socket::close(){
    if (handle_ != INVALID_SOCKET){
        closesocket(handle_);
        handle_ = INVALID_SOCKET;
    }
}