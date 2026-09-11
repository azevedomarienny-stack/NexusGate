#pragma once
#include <winsock2.h>

class Socket {
    public:
        explicit Socket(SOCKET handle = INVALID_SOCKET);
        ~Socket();

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        Socket(Socket&& other) noexcept;
        Socket& operator=(Socket&& other) noexcept;

        SOCKET get() const{ return handle_; }
        bool isValid() const{ return handle_ != INVALID_SOCKET;}
        void close();

    private: 
        SOCKET handle_;
};