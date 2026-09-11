#include "../include/Logger.hpp"
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace {
    std::mutex logMutex;

    std::string timestamp(){
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &t);
        char buf[16];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
        return buf;
    }

    void log(const std::string& level, const std::string& msg){
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "[" << timestamp() << "] [" << level << "] " << msg << "\n";
    }
}

namespace Logger {
    void info(const std::string& msg) { log("INFO ", msg); }
    void warn(const std::string& msg) {log("WAN ", msg); }
    void error(const std::string& msg) {log("ERROR", msg); }
}