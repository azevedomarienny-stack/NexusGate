#include <iostream>
#include <thread>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "TcpListener.hpp"
#include "ThreadPool.hpp"
#include "BackendPool.hpp"
#include "../include/Logger.hpp"
#include "Metrics.hpp"
#include "HttpMetricsServer.hpp"
#include "Database.hpp"
#include "RateLimiter.hpp"
#include "NetUtils.hpp"

#pragma comment(lib, "ws2_32.lib")

Socket connectToBackend(Backend* backend) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(backend->port);
    inet_pton(AF_INET, backend->host.c_str(), &addr.sin_addr);

    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closesocket(s);
        throw std::runtime_error("Falha ao conectar no backend");
    }
    return Socket(s);
}

void setSocketTimeout(SOCKET s, int seconds){
    DWORD timeout = seconds * 1000; 
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
}

void handleClient(Socket client, BackendPool& pool) {
    Backend* backend = pool.getNextHealthy();
    if (!backend) {
        Logger::error("Nenhum backend saudavel disponivel");
        return; // client fecha automaticamente (RAII)
    }

    Logger::info("Encaminhando conexao para " + backend->host + ":" + std::to_string(backend->port));

    try {
        Socket backendSocket = connectToBackend(backend);

        char buffer[1024];
        int received = recv(client.get(), buffer, sizeof(buffer), 0);
        if (received > 0) {
            send(backendSocket.get(), buffer, received, 0);

            int responseLen = recv(backendSocket.get(), buffer, sizeof(buffer), 0);
            if (responseLen > 0) {
                send(client.get(), buffer, responseLen, 0);
            }
        }
    } catch (const std::exception& e) {
        Logger::error(std::string("Erro ao encaminhar: ") + e.what());
    }
} 

void handleClient(Socket client, BackendPool& pool, Metrics& metrics, Database& db, RateLimiter& rateLimiter) {
    std::string clientIp = getPeerIp(client.get());

    if (!rateLimiter.allow(clientIp)) {
        Logger::warn("Rate limit excedido para IP " + clientIp + " - conexao rejeitada");
        return; // fecha a conexao sem processar (RAII)
    }

    setSocketTimeout(client.get(), 5); // 5 segundos para receber dados do cliente

    metrics.connectionOpened();
    auto start = std::chrono::steady_clock::now();
    bool success = false;

    Backend* backend = pool.getNextHealthy();
    if (backend) {
        try {
            Socket backendSocket = connectToBackend(backend);
            setSocketTimeout(backendSocket.get(), 5); // 5 segundos para o backend responder
            backend->requestCount.fetch_add(1);

            char buffer[1024];
            int received = recv(client.get(), buffer, sizeof(buffer), 0);
            if (received > 0) {
                send(backendSocket.get(), buffer, received, 0);
                int responseLen = recv(backendSocket.get(), buffer, sizeof(buffer), 0);
                if (responseLen > 0) {
                    send(client.get(), buffer, responseLen, 0);
                    success = true;
                }
            } else if (received == SOCKET_ERROR && WSAGetLastError() == WSAETIMEDOUT) {
                Logger::warn("Timeout aguardando dados do cliente " + clientIp);
            }
        } catch (const std::exception& e) {
            Logger::error(std::string("Erro ao encaminhar: ") + e.what());
        }
    } else {
        Logger::error("Nenhum backend saudavel disponivel");
    }

    auto end = std::chrono::steady_clock::now();
    double latencyMs = std::chrono::duration<double, std::milli>(end - start).count();
    metrics.recordRequest(success, latencyMs);
    metrics.connectionClosed();
}


void healthCheckLoop(BackendPool& pool, Database& db) {
    while (true) {
        for (auto& backendPtr : pool.all()) {
            Backend* b = backendPtr.get();

            SOCKET testSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(b->port);
            inet_pton(AF_INET, b->host.c_str(), &addr.sin_addr);

            bool ok = connect(testSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
            closesocket(testSocket);

            bool wasHealthy = b->healthy.load();
            b->healthy.store(ok);

            if (ok != wasHealthy) {
                Logger::warn("Backend " + b->host + ":" + std::to_string(b->port) +
                            (ok ? " voltou a ficar saudavel" : " ficou indisponivel"));
                db.logBackendEvent(b->host, b->port, ok);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void metricsSnapshotLoop(Metrics& metrics, Database& db) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        db.saveMetricsSnapshot(
            metrics.totalRequests(),
            metrics.totalErrors(),
            metrics.activeConnections(),
            metrics.averageLatencyMs()
        );
        Logger::info("Snapshot de metricas salvo no banco.");
    }
}


int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Logger::error("Falha ao inicializar Winsock.");
        return 1;
    }

    Database db("nexusgate.db");

    BackendPool backendPool;
    backendPool.addBackend("127.0.0.1", 9001);
    backendPool.addBackend("127.0.0.1", 9002);

    std::thread healthThread(healthCheckLoop, std::ref(backendPool), std::ref(db));
    healthThread.detach();

    Metrics metrics;
    HttpMetricsServer metricsServer(backendPool, metrics);
    std::thread metricsThread(&HttpMetricsServer::run, &metricsServer, 8081);
    metricsThread.detach();

    std::thread snapshotThread(metricsSnapshotLoop, std::ref(metrics), std::ref(db));
    snapshotThread.detach();

    RateLimiter rateLimiter(20, 10);

    try {
        TcpListener listener(8080);
        ThreadPool pool(4);

        Logger::info("NexusGate escutando na porta 8080 com 4 workers...");

        while (true) {
            auto clientPtr = std::make_shared<Socket>(listener.accept());
            pool.enqueue([clientPtr, &backendPool, &metrics, &db, &rateLimiter]() {
                handleClient(std::move(*clientPtr), backendPool, metrics, db, rateLimiter);
            });
        }
    } catch (const std::exception& e) {
        Logger::error(std::string("Erro fatal: ") + e.what());
        WSACleanup();
        return 1;
    }

    WSACleanup();
    return 0;
}