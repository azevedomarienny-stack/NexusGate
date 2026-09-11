#include "Database.hpp"
#include <stdexcept>

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "desconhecido";
        throw std::runtime_error("Falha ao abrir banco SQLite: " + msg);
    }
    createTables();
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

void Database::execOrThrow(const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "erro desconhecido";
        sqlite3_free(errMsg);
        throw std::runtime_error("Erro SQL: " + msg);
    }
}

void Database::createTables() {
    execOrThrow(R"(
        CREATE TABLE IF NOT EXISTS backend_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            host TEXT NOT NULL,
            port INTEGER NOT NULL,
            healthy INTEGER NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )");

    execOrThrow(R"(
        CREATE TABLE IF NOT EXISTS metrics_snapshots (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            total_requests INTEGER NOT NULL,
            total_errors INTEGER NOT NULL,
            active_connections INTEGER NOT NULL,
            avg_latency_ms REAL NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )");
}

void Database::logBackendEvent(const std::string& host, uint16_t port, bool healthy) {
    std::lock_guard<std::mutex> lock(dbMutex_);

    const char* sql = "INSERT INTO backend_events (host, port, healthy) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, host.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, port);
    sqlite3_bind_int(stmt, 3, healthy ? 1 : 0);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::saveMetricsSnapshot(uint64_t totalRequests, uint64_t totalErrors,
                                    uint64_t activeConnections, double avgLatencyMs) {
    std::lock_guard<std::mutex> lock(dbMutex_);

    const char* sql =
        "INSERT INTO metrics_snapshots (total_requests, total_errors, active_connections, avg_latency_ms) "
        "VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(totalRequests));
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(totalErrors));
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(activeConnections));
    sqlite3_bind_double(stmt, 4, avgLatencyMs);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}