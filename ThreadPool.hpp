#pragma once

#include <queue>

struct TaskQueue{
    std::queue<std::function<void()>> tasks;
    std::mutex mutex;
    std::atomic<uint32_t> tasks_remaining{0};

    template<typename CallBack>
    void addTask(CallBack&& cb){
        std::lock_guard<std::mutex> lock_guard{mutex};
        tasks.push(std::forward<CallBack>(cb));
        tasks_remaining++;
    }

    void getTask(std::function<void()>& cb){
        std::lock_guard<std::mutex> lock_guard{mutex};
        if(tasks.empty()) return;
        cb = std::move(tasks.front());
        tasks.pop();
    }

    static void wait(){
        std::this_thread::yield();
    }

    void waitForCompletion() const {
        while(tasks_remaining > 0) wait();
    }

    void workDone(){
        tasks_remaining--;
    }
};

struct Single{
    uint32_t id;
    std::thread thread;
    std::function<void()> task = nullptr;
    bool running = true;
    TaskQueue* queue = nullptr;

    Single() = default;

    Single(TaskQueue& queue_, uint32_t id_):
    queue{&queue_},
    id{id_} {
        thread = std::thread([this](){
            run();
        });
    }

    void run(){
        while(running){
            queue->getTask(task);
            if(task == nullptr){
                TaskQueue::wait();
            }else{
                task();
                queue->workDone();
                task = nullptr;
            }
        }
    }

    void stop(){
        running = false;
        thread.join();
    }

};

struct ThreadPool{
    uint32_t thread_count = 0;
    TaskQueue queue;
    std::vector<Single> threads;

    explicit ThreadPool(uint32_t thread_count_):
    thread_count{thread_count_}{
        threads.reserve(thread_count);
        for(uint32_t i{thread_count}; i--;){
            threads.emplace_back(queue, static_cast<uint32_t>(threads.size()));
        }
    }

    virtual ~ThreadPool(){
        for(Single& thread : threads){
            thread.stop();
        }
    }

    template<typename CallBack>
    void addTask(CallBack&& cb){
        queue.addTask(std::forward<CallBack>(cb));
    }

    void waitForCompletion() const{
        queue.waitForCompletion();
    }

    template<typename CallBack>
    void dispatch(uint32_t element_count, CallBack&& cb){
        const uint32_t batch_size = element_count / thread_count;
        for(uint32_t i{0}; i < thread_count; i++){
            const uint32_t start = batch_size*i;
            const uint32_t end = batch_size + start;
            addTask([start, end, &cb] {cb(start, end);});
        }
        if(batch_size*thread_count < element_count){
            const uint32_t start = batch_size * thread_count;
            cb(start, element_count);
        }

        waitForCompletion();
    }
};
