#include "RateLimiter.hpp"

RateLimiter::RateLimiter(int maxRequests, int windowSeconds)
    : maxRequests_(maxRequests), windowSeconds_(windowSeconds) {}

bool RateLimiter::allow(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();

    auto it = entries_.find(ip);
    if (it == entries_.end()) {
        entries_[ip] = { 1, now };
        return true;
    }

    Entry& entry = it->second;
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - entry.windowStart).count();

    if (elapsed >= windowSeconds_) {
        // janela expirou, reseta a contagem
        entry.count = 1;
        entry.windowStart = now;
        return true;
    }

    entry.count++;
    return entry.count <= maxRequests_;
}