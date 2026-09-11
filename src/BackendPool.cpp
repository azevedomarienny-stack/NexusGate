#include "BackendPool.hpp"

void BackendPool::addBackend(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(structureMutex_);
    backends_.push_back(std::make_unique<Backend>(host, port));
}

Backend* BackendPool::getNextHealthy() {
    size_t count = backends_.size();
    if (count == 0) return nullptr;

    for (size_t attempts = 0; attempts < count; ++attempts) {
        size_t idx = nextIndex_.fetch_add(1) % count;
        Backend* candidate = backends_[idx].get();
        if (candidate->healthy.load()) {
            return candidate;
        }
    }
    return nullptr;
}