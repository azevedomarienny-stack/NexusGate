#pragma once
#include "BackendPool.hpp"
#include "Metrics.hpp"
#include <cstdint>

//Servido HTTP não generico, que responde a GET
//Roda na thread, separa da proxy principal

class HttpMetricsServer{
    public:
        HttpMetricsServer(BackendPool& pool, Metrics& metrics);
        void run(uint16_t port);

    private:
        BackendPool& pool_;
        Metrics& metrics_;

        std::string buildMetricsJson() const;
        void handleRequest(class Socket& client);
};