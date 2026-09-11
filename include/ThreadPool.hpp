#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

//consome tarefas com filas compartilhadas
//evita a construcao a cada conexao

class ThreadPool {
    public:
        explicit ThreadPool(size_t numThreands);
        ~ThreadPool();

        void enqueue(std::function<void()> task);

    private:
        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex queueMutex_;
        std::condition_variable condition_;
        std::atomic<bool> stop_{false};

        void workerLoop();
};