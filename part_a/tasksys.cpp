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
    _num_threads = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::workToRun(IRunnable* runnable) {
    while (true) {
        lock_guard<mutex> lock(task_lock);
        int next_task_id = tasks.front();

        if (tasks.size() == 0) {
            break;
        }
        runnable -> runTask(next_task_id, total_tasks);
        tasks.pop();
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // track the order of the tasks and add them into a queue to be used
    for (int i = 0; i < num_total_tasks; i++) {
        tasks.push(i);
    }

    total_tasks.store(num_total_tasks);
    // printf("initializing with total_tasks: %d\n", num_total_tasks);
    // printf("total tasks: %d\n", total_tasks.load());

    // initialize our threads, although inefficient, at the beginning
    // of our run function
    vector<thread> threads;
    threads.reserve(_num_threads);

    for (int i = 0; i < _num_threads; ++i) {
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
    int nothing = 1;
    // while (!stop.load()) {

    //     if (tasks_remaining.load() > 0) {
    //         int index = next_task_index.fetch_sub(1);

    //         if (current_runnable == nullptr) {
    //             if (index >= 0) {
    //                 // run task
    //                 current_runnable->runTask(index, total_tasks);
    //                 tasks_remaining.fetch_sub(1);
    //             }
    //         }
    //     // no more tasks 
    //     } else {
    //         stop.store(true);
    //     }
    // }
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    int nothing = 1;
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    // stop.store(false);
    // for (int i = 0; i < num_threads; i++) {
    //     threads_.emplace_back(&TaskSystemParallelThreadPoolSpinning::spinningWork, this);
    // }

}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    // Initialize task counters
    // next_task_index.store(num_total_tasks - 1);
    // tasks_remaining.store(num_total_tasks);
    // total_tasks = num_total_tasks;

    // current_runnable = runnable;

    // // Wait until all tasks are done
    // while (tasks_remaining.load() > 0) {
    //     std::this_thread::yield(); 
    // }

    // stop.store(true);

    // for (auto& thread : threads_) {
    //     thread.join();
    // } 
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

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    // might be useful... https://www.geeksforgeeks.org/cpp/thread-pool-in-cpp/
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
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
