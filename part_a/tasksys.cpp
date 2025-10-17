#include "tasksys.h"
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <cstdio>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

using namespace std;

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}


TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    num_threads_ = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::workToRun(IRunnable* runnable) {
    while (true) {
        int next_task_id = tasks.fetch_sub(1) - 1;

        if (next_task_id < 0) {
            break;
        }
        runnable -> runTask(next_task_id, total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // track the order of the tasks and add them into a queue to be used
    tasks.store(num_total_tasks);
    total_tasks.store(num_total_tasks);

    // initialize our threads, although inefficient, at the beginning
    // of our run function
    vector<thread> threads;
    threads.reserve(num_threads_);

    for (int i = 0; i < num_threads_; ++i) {
        threads.emplace_back(&TaskSystemParallelSpawn::workToRun, this, runnable);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    total_tasks.store(0);
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

void TaskSystemParallelThreadPoolSpinning::spinningWork() {
    while (true) {
        IRunnable* task_runner = nullptr;
        int task_id;
        {
            lock_guard<mutex> lock(task_lock);

            if (is_main_thread_done) return;

            if (tasks_remaining > 0 && current_runnable != nullptr) {
                task_id = --tasks_remaining;
                task_runner = current_runnable;
            }
        }
        if (task_runner && task_id >= 0) {
            task_runner->runTask(task_id, total_tasks);

            lock_guard<mutex> lock(task_lock);
            num_tasks_completed++;

            if (num_tasks_completed == total_tasks) {
                are_workers_done = true;
                current_runnable = nullptr;
            }
        } 
    }
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // initialize our threads, although inefficient, at the beginning
    // of our run function
    current_runnable = nullptr;

    for (int i = 0; i < num_threads; ++i) {
        threads_.emplace_back(&TaskSystemParallelThreadPoolSpinning::spinningWork, this);
    }
 
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    {
        lock_guard<mutex> lock(task_lock);
        is_main_thread_done = true;
    }
    for (auto& thread : threads_) {

        if (thread.joinable()) {
            thread.join();
        }
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {

    // track the order of the tasks and add them into a queue to be used
    {
        lock_guard<mutex> lock(task_lock);
        current_runnable = runnable;
        total_tasks = num_total_tasks;
        tasks_remaining = num_total_tasks;
        num_tasks_completed = 0;
        are_workers_done = false;
    }

    while (true) {
        {
            lock_guard<mutex> lock(task_lock);
            if (are_workers_done) break;
        }
    }
    
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

void TaskSystemParallelThreadPoolSleeping::sleepingWork() {
    while (true) {

        int next_task_id;
        IRunnable* task_runner = nullptr;

        {
            unique_lock<mutex> lock(queue_mutex);

            cv_.wait(lock, [this] {
                return (!tasks.empty() && cur_runnable != nullptr) || stop_;
            });

            // exit the pool once we should stop and there's no more tasks
            if (stop_ && tasks.empty()) {
                return;
            }

            if (!tasks.empty() && cur_runnable != nullptr) {
                next_task_id = tasks.front();
                tasks.pop();
                task_runner = cur_runnable;  
            }

        }
        if (task_runner != nullptr && next_task_id >= 0) {
            task_runner->runTask(next_task_id, total_tasks);

            int done = num_tasks_run.fetch_add(1) + 1;
            if (done == total_tasks) {
                lock_guard<mutex> lock(queue_mutex);
                done_cv.notify_one();  // notify run() that all tasks are done
            }
        }
    }
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    cur_runnable = nullptr;

    for (int i = 0; i < num_threads; ++i) {
        threads_.emplace_back(&TaskSystemParallelThreadPoolSleeping::sleepingWork, this);
    }

}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    {
        unique_lock<mutex> lock(queue_mutex);
        stop_ = true;
    }

    cv_.notify_all();

    for (auto& thread: threads_) {
        thread.join();
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    unique_lock<mutex> lock(task_lock);

    cur_runnable = runnable;
    total_tasks = num_total_tasks;
    stop_ = false;
    num_tasks_run.store(0);

    for (int i = 0; i < num_total_tasks; i++) {
        unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(i);
    }
    cv_.notify_all();

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        done_cv.wait(lock, [this] {
            return num_tasks_run.load() >= total_tasks;
        });
    }

    cur_runnable = nullptr;  

}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {
    
    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {
    return;
}
