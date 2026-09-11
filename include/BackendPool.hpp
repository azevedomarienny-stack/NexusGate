#pragma once
#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include <mutex>
#include <cstdint>


struct Backend {
    std::string host;
    uint16_t port;
    std::atomic<bool> healthy{true};
    std::atomic<uint64_t> requestCount{0};

    Backend(std::string h, uint16_t p) : host(std::move(h)), port(p) {}
};

class BackendPool {
public:
    void addBackend(const std::string& host, uint16_t port);
    Backend* getNextHealthy();
    std::vector<std::unique_ptr<Backend>>& all() { return backends_; }

private:
    std::vector<std::unique_ptr<Backend>> backends_;
    std::mutex structureMutex_;
    std::atomic<size_t> nextIndex_{0};
};