#pragma once
#include <atomic>
#include <cstdint>

//Coleta contadores com metodos concorrentes

class Metrics{
    public:
        void recordRequest(bool success, double latencyMs);
        void connectionOpened();
        void connectionClosed();

        uint64_t totalRequests() const { 
            return totalRequests_.load(); 
            }
        uint64_t totalErrors() const { 
            return totalErrors_.load(); 
            } 
        uint64_t activeConnections() const{ 
            return activeConnections_.load(); 
            }
        double averageLatencyMs() const;

    private:
        std::atomic<uint64_t>totalRequests_{0};
        std::atomic<uint64_t>totalErrors_{0};
        std::atomic<uint64_t>activeConnections_{0};
        std::atomic<uint64_t>latencySumMicros_{0};
};