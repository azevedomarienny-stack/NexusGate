
#include "HttpMetricsServer.hpp"
#include "TcpListener.hpp"
#include "Logger.hpp"
#include <sstream>
#include <winsock2.h>


HttpMetricsServer::HttpMetricsServer(BackendPool& pool, Metrics& metrics)
        : pool_(pool), metrics_(metrics) {}

std::string HttpMetricsServer::buildMetricsJson() const{
    std::ostringstream json;
    json<< "{"
        << "\"totalRequests\":" << metrics_.totalRequests() << ","
        << "\"totalErrors\":" << metrics_.totalErrors() << ","
        << "\"activeConnections\":" << metrics_.activeConnections() << ","
        << "\"avgLatencyMs\":" << metrics_.averageLatencyMs() << ","
        << "\"backends\":[";

    bool first = true;
    for (auto& b : pool_.all()) {
        if (!first) json <<",";
        first = false;
        json << "{"
        << "\"host\":\"" << b->host << "\","
        << "\"port\":" << b->port << ","
        << "\"healthy\":" << (b->healthy.load() ? "true" : "false") << ","
        << "\"requests\":" << b->requestCount.load()
        << "}";
    }
    json << "]}";
    return json.str();
}

void HttpMetricsServer::handleRequest(Socket& client) {
    char buffer[2048];
    int received = recv(client.get(), buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return;
    buffer[received] = '\0';

    std::string request(buffer);

    // Responde ao preflight CORS do navegador antes de qualquer outra logica
    if (request.find("OPTIONS ") == 0) {
        std::ostringstream preflight;
        preflight << "HTTP/1.1 204 No Content\r\n"
                  << "Access-Control-Allow-Origin: *\r\n"
                  << "Access-Control-Allow-Headers: X-API-Key\r\n"
                  << "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
                  << "Content-Length: 0\r\n"
                  << "Connection: close\r\n\r\n";
        std::string preflightStr = preflight.str();
        send(client.get(), preflightStr.c_str(), static_cast<int>(preflightStr.size()), 0);
        return;
    }

    bool wantsMetrics = request.find("GET /metrics") == 0;

    const std::string expectedKey = "X-API-Key: nexusgate-dev-key";
    bool authorized = request.find(expectedKey) != std::string::npos;

    std::string status;
    std::string body;

    if (!wantsMetrics) {
        status = "404 Not Found";
        body = "{\"error\":\"not found\"}";
    } else if (!authorized) {
        status = "401 Unauthorized";
        body = "{\"error\":\"missing or invalid API key\"}";
    } else {
        status = "200 OK";
        body = buildMetricsJson();
    }

    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n"
             << "Content-Type: application/json\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Headers: X-API-Key\r\n"
             << "Cache-Control: no-store, no-cache, must-revalidate\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << body;

    std::string responseStr = response.str();
    send(client.get(), responseStr.c_str(), static_cast<int>(responseStr.size()), 0);
}
void HttpMetricsServer::run(uint16_t port) {
    try {
        TcpListener listener(port);
        Logger::info("Servidor de metricas escutando na porta " + std::to_string(port));

        while (true) {
            Socket client = listener.accept();
            handleRequest(client);
            // client fecha automaticamente ao sair de escopo
        }
    } catch (const std::exception& e) {
        Logger::error(std::string("Erro no HttpMetricsServer: ") + e.what());
    }
}