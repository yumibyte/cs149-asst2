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
        // Check for shutdown and no more tasks
        if (is_main_thread_done.load() && tasks.load() <= 0) {
            break; 
        }

        IRunnable* task_runner = cur_runnable.load();
        if (task_runner == nullptr) {
            std::this_thread::yield();
            continue;
        }
        int next_task_id = tasks.fetch_sub(1) - 1;

        if (next_task_id < 0) {
            std::this_thread::yield(); 
            continue;
        }

        // printf("running next task %d\n", next_task_id);
        task_runner->runTask(next_task_id, total_tasks);

        int done = num_tasks_run.fetch_add(1) + 1;
        if (done == total_tasks) {
            are_workers_done.store(true);
            cur_runnable.store(nullptr);
            // printf("triggering done\n");
        }
    }
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).

    // initialize our threads, although inefficient, at the beginning
    // of our run function
    cur_runnable = nullptr;

    for (int i = 0; i < num_threads; ++i) {
        threads_.emplace_back(&TaskSystemParallelThreadPoolSpinning::spinningWork, this);
    }
 
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    // printf("setting is done to true\n");
    destructor_lock.lock();
    is_main_thread_done.store(true);
    // cur_runnable.store(nullptr);

    for (auto& thread : threads_) {

        if (thread.joinable()) {

            thread.join();
        }
    }
    destructor_lock.unlock();
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {

    // track the order of the tasks and add them into a queue to be used
    unique_lock<mutex> lock(task_lock);
    is_main_thread_done.store(false);
    are_workers_done.store(false);
    num_tasks_run.store(0);

    tasks.store(num_total_tasks);
    total_tasks = num_total_tasks;
    cur_runnable.store(runnable);

    while (!(are_workers_done.load(memory_order_acquire))) {
        this_thread::yield();
    }
    // printf("workers become done\n");
    cur_runnable.store(nullptr);
    // printf("workers are done\n");
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
            printf("getting ready to execute a task\n");

            cv_.wait(lock, [this] {
                printf("put a thread to sleep\n");
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
                printf("set next_task_id to be run for %d\n", next_task_id);
            }

        }
        if (task_runner != nullptr && next_task_id >= 0) {
            printf("running next task %d\n", next_task_id);
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
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    // might be useful... https://www.geeksforgeeks.org/cpp/thread-pool-in-cpp/
    // cur_runnable = nullptr;

    // for (int i = 0; i < num_threads; ++i) {
    //     threads_.emplace_back(&TaskSystemParallelThreadPoolSleeping::sleepingWork, this);
    //     printf("initialize thread %d\n", i);
    // }

}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    // {
    //     unique_lock<mutex> lock(queue_mutex);
    //     stop_ = true;
    //     // printf("main thread is done\n");
    // }

    // cv_.notify_all();

    // for (auto& thread: threads_) {
    //     thread.join();
    // }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
    // unique_lock<mutex> lock(task_lock);
    // printf("setting up run\n");

    // cur_runnable = runnable;
    // total_tasks = num_total_tasks;
    // stop_ = false;
    // num_tasks_run.store(0);

    // for (int i = 0; i < num_total_tasks; i++) {
    //     unique_lock<std::mutex> lock(queue_mutex);
    //     printf("enqueue task i: %d\n", i);
    //     tasks.push(i);
    // }
    // cv_.notify_all();


    // printf("finished setting in run\n");
    // {
    //     std::unique_lock<std::mutex> lock(queue_mutex);
    //     done_cv.wait(lock, [this] {
    //         return num_tasks_run.load() >= total_tasks;
    //     });
    // }

    // cur_runnable = nullptr;  // optional: clean up

}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
