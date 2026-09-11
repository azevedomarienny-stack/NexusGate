#include <iostream>
#include "BackendPool.hpp"
#include "RateLimiter.hpp"
#include "Metrics.hpp"

int testsRun = 0;
int testsFailed = 0;

#define CHECK(cond, msg) do { \
    testsRun++; \
    if (!(cond)){ \
        std::cerr << "[FALHOU]" << msg << " (linha " <<__LINE__<< ")\n"; \
        testsFailed++; \
    } else { \
        std::cout <<"[OK]" << msg << "\n"; \
    } \
}while (0)

void testBackendPoolRoundRobin() {
    BackendPool pool;
    pool.addBackend("127.0.0.1", 9001);
    pool.addBackend("127.0.0.1", 9002);

    Backend* first = pool.getNextHealthy();
    Backend* second = pool.getNextHealthy();
    Backend* third = pool.getNextHealthy();

    CHECK(first != nullptr && second !=nullptr, "round-robin retorna backends validos");
    CHECK(first->port != second->port, "round-robin alterna entre backends diferentes");
    CHECK(third->port == first->port, "round-robin volta ao primeiro backend apos ciclo completo");
}

void testBackendPoolSkipsUnhealthy(){
    BackendPool pool;
    pool.addBackend("127.0.0.1", 9001);
    pool.addBackend("127.0.0.1", 9002);
    pool.all()[0]->healthy.store(false);

    Backend* result = pool.getNextHealthy();
    CHECK(result != nullptr && result->port == 9002, "round-robin pula backend marcado como indisponivel");
}

void testBackendPoolAllUnhealthy() {
    BackendPool pool;
    pool.addBackend("127.0.0.1", 9001);
    pool.all()[0]->healthy.store(false);

    Backend* result = pool.getNextHealthy();
    CHECK(result == nullptr, "retorna nullptr quando nenhum backend esta saudavel");
}

void testRateLimiterAllowsWithinLimit() {
    RateLimiter limiter(3, 10);
    bool r1 = limiter.allow("1.2.3.4");
    bool r2 = limiter.allow("1.2.3.4");
    bool r3 = limiter.allow("1.2.3.4");
    CHECK(r1 && r2 && r3, "rate limiter permite requisicoes ate o limite configurado");
}

void testRateLimiterBlocksOverLimit() {
    RateLimiter limiter(2, 10);
    limiter.allow("5.6.7.8");
    limiter.allow("5.6.7.8");
    bool r3 = limiter.allow("5.6.7.8");
    CHECK(!r3, "rate limiter bloqueia apos exceder o limite");
}

void testRateLimiterIndependentPerIp() {
    RateLimiter limiter(1, 10);
    bool a = limiter.allow("9.9.9.9");
    bool b = limiter.allow("8.8.8.8");
    CHECK(a && b, "rate limiter conta cada IP de forma independente");
}

void testMetricsAverageLatency() {
    Metrics m;
    m.recordRequest(true, 10.0);
    m.recordRequest(true, 20.0);
    m.recordRequest(false, 30.0);

    CHECK(m.totalRequests() == 3, "metrics conta total de requisicoes corretamente");
    CHECK(m.totalErrors() == 1, "metrics conta erros corretamente");

    double avg = m.averageLatencyMs();
    CHECK(avg > 19.9 && avg < 20.1, "metrics calcula latencia media corretamente");
}

void testMetricsActiveConnections() {
    Metrics m;
    m.connectionOpened();
    m.connectionOpened();
    m.connectionClosed();
    CHECK(m.activeConnections() == 1, "metrics rastreia conexoes ativas corretamente");
}

int main() {
    std::cout << "=== Testes do NexusGate ===\n\n";

    testBackendPoolRoundRobin();
    testBackendPoolSkipsUnhealthy();
    testBackendPoolAllUnhealthy();
    testRateLimiterAllowsWithinLimit();
    testRateLimiterBlocksOverLimit();
    testRateLimiterIndependentPerIp();
    testMetricsAverageLatency();
    testMetricsActiveConnections();

    std::cout << "\n=== Resultado: " << (testsRun - testsFailed) << "/" << testsRun
            << " testes passaram ===\n";

    return testsFailed == 0 ? 0 : 1;
}
