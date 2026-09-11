#pragma once
#include <string>
#include <mutex>
#include <cstdint>
#include "sqlite3.h"

//conexao com o SQLite protegida pelo mutex

class Database {
    public:
        explicit Database(const std::string& path);
        ~Database();

        Database(const Database&) = delete;
        Database& operator=(const Database&)= delete;

        void logBackendEvent(const std::string& host, uint16_t port, bool healthy);
        void saveMetricsSnapshot(uint64_t totalRequests, uint64_t totalErrors, uint64_t activeConnections, double avgLatencyMs);

    private:
        sqlite3* db_=nullptr;
        std::mutex dbMutex_;

        void execOrThrow(const std::string& sql);
        void createTables();
};