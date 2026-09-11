#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t numThreads){
    for (
        size_t i = 0;
        i< numThreads; ++i
        ){
            workers_.emplace_back(&ThreadPool::workerLoop, this);
        }
}

ThreadPool::~ThreadPool(){
    stop_=true;
    condition_.notify_all();
    for (
        auto& worker : workers_
    ){
        if (
            worker.joinable()
        ) worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task){
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        tasks_.push(std::move(task));
    }
    condition_.notify_one();
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            //Fica em suspensão até um servico ser solicitado
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty()) return;

            task = std::move(tasks_.front());
            tasks_.pop();        
        }
        task(); //executa fora do lock
    }
}