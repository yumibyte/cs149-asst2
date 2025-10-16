#include "tasksys.h"
#include <vector>
#include <cstdio>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

using namespace std;
/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

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
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
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
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
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
            // printf("getting ready to execute a task\n");

            cv_.wait(lock, [this] {
                // printf("put a thread to sleep\n");
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
                // printf("set next_task_id to be run for %d\n", next_task_id);
            }

        }
        if (task_runner != nullptr && next_task_id >= 0) {
            // printf("running next task %d\n", next_task_id);
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
    cur_runnable = nullptr;

    for (int i = 0; i < num_threads; ++i) {
        threads_.emplace_back(&TaskSystemParallelThreadPoolSleeping::sleepingWork, this);
        // printf("initialize thread %d\n", i);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    {
        unique_lock<mutex> lock(queue_mutex);
        stop_ = true;
        // printf("main thread is done\n");
    }

    cv_.notify_all();

    for (auto& thread: threads_) {
        thread.join();
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    vector<TaskID> no_deps;
    printf("adding async task with %d total tasks\n", num_total_tasks);
    runAsyncWithDeps(runnable, num_total_tasks, no_deps);
    sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    unique_lock<mutex> lock(graph_mutex);

    TaskID id = next_task_id++;

    TaskInfo info;
    info.runnable = runnable;
    info.num_total_tasks = num_total_tasks;
    info.deps = deps;
    info.remaining_deps = deps.size();

    launches[id] = move(info);
    unfinished_launches++;

    // Record reverse dependencies
    for (TaskID dep : deps) {
        printf("adding the deps for id %d", id);
        launches[dep].children.push_back(id);
    }

    // If no dependencies, mark it ready
    if (deps.empty()) {
        printf("adding id %d to ready queue\n", id);
        ready_queue.push(id);
    }

    return id;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    // Process launches in dependency order
    while (true) {
        printf("running sync\n");
        TaskID next_launch = -1;

        {
            unique_lock<mutex> lock(graph_mutex);

            if (ready_queue.empty()) {
                if (unfinished_launches == 0)
                    break; 

                // If nothing ready yet, wait for dependencies
                sync_cv.wait(lock);
                continue;
            }

            next_launch = ready_queue.front();
            ready_queue.pop();
        }

        // Execute this launch using existing run() system
        task_lock.lock();
        TaskInfo &info = launches[next_launch];
        cur_runnable = info.runnable;
        total_tasks = info.num_total_tasks;
        stop_ = false;
        num_tasks_run.store(0);

        for (int i = 0; i < info.num_total_tasks; i++) {
            printf("added task to tasks for %d\n", i);
            tasks.push(i);
        }

        cv_.notify_all();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            done_cv.wait(lock, [this] {
                return num_tasks_run.load() >= total_tasks;
            });
        }
        task_lock.unlock();


        // Mark launch complete and update dependents
        {
            unique_lock<mutex> lock(graph_mutex);
            unfinished_launches--;

            for (TaskID child : info.children) {
                TaskInfo &child_info = launches[child];
                child_info.remaining_deps--;

                if (child_info.remaining_deps == 0) {
                    ready_queue.push(child);
                }
            }

            if (unfinished_launches == 0) {
                sync_cv.notify_all();

            } else {
                sync_cv.notify_all(); // wake up waiting sync()
            }
        }
    }


    return;
}
