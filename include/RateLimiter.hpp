#pragma once 
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>

//janela fixa:; limita as requesições por IP

class RateLimiter {
    public: 
        RateLimiter(int maxRequests, int windowSeconds);
        bool allow(const std::string& ip);

    private: 
        struct Entry{
            int count;
            std::chrono::steady_clock::time_point windowStart;
        };

        std::unordered_map<std::string, Entry> entries_;
        std::mutex mutex_;
        int maxRequests_;
        int windowSeconds_;
};