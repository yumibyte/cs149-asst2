# Writeup

1. Task system implementation

For the sleeping thread pool implementation:

**Describe your task system implementation (1 page is fine). In additional to a general description of how it works, please make sure you address the following questions:**

- These threads are managed as:
    - A queue of tasks that we know we need to execute which will fill up and then the spinning threads will grab
    - We put the threads to sleep when there's no work to do and, if there is work that is added (by enqueuing the tasks), we wake up the threads
    - We track both when the work is complete after having queued a set of tasks and when the destructor activates (signifying the main thread needs the spinning threads to exit). This is done through flags updated with locks and condition variables to notify the workers to wake up or stop.
- When initializing the thread pool sleeping class, the threads immediately kick off and begin running in a function `sleepingWork`
    - For this sleeping work function, the threads continuously loop
    - Then, they wait based on a condition variable and once there are tasks that are available within a queue, they will stop waiting and will grab the next task ID and the proper runnable object that was set based on the run() function's runnable instance
    - Then, it will run the task and ensure that we haven't run out of tasks. In the case that we have, we need to notify the main thread that is waiting on the tasks 
    - The main thread that's waiting for the tasks to finish will then wait for all the tasks to complete and then join all the threads
- With regards to the asynchronous implementation:
    - if we run synchronously, simply execute the synchronous function with 0 dependencies and sync it immediately
    - otherwise, if we are running asynchronously, we will add any sets of tasks that have no dependencies to the ready queue in preparation for being run and the sync function will ensure that these tasks have finished by waiting for all tasks to complete
- My system assigns work to threads dynamically. This is simplest because we don't need to predetermine the work for each thread but instead have the work executed as when it becomes available. So, when we enqueue a new task to be executed, we simply notify the threads of this such that they can pick it up and execute it.
- I tracked dependencies in task B as so:
    - A struct to store the runnable pointer, number of tasks that runnable function should run, the dependencies this task has, the remaining dependencies that should be resolved, and the children who depend on this task. 
    - It is important we have the dependencies this task has such that we can determine when these tasks can be moved to the ready queue and the children are necessary to track when those respective tasks should be kicked off (in the case executing this task enables another task to run). We can do so by tracking the remaining dependencies for each set of tasks and then adding these sets of tasks to the ready queue once they have 0 remaining dependencies tracked 
    - With regards to speed, we can leverage a map of all of the task infos such that when we run a task we can access in O(1) time the task's information to either 1) run it 2) decrement the dependencies it has left to be resolved such that it can be moved to the ready queue later 3) or observe its dependencies. This allows us to avoid time-intensive loops to determine which task to run next or if running a provided task will allow any of its "children" to now be available to run in the ready queue

2. We can observe that from super_super_light that the performance of serial is very good. To break this down, we see that serial is much better for super_super_light than the other implementations, such as always spawn:
```
Results for: super_super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                10.49     10.264      1.02  (OK)
[Parallel + Always Spawn]               170.441   167.918     1.02  (OK)
[Parallel + Thread Pool + Spin]         24.905    24.765      1.01  (OK)
[Parallel + Thread Pool + Sleep]        12.952    12.955      1.00  (OK)
```

This is mainly due to the fact that in super_super_light we have 64 tasks and 400 bulk task launches. To elaborate, for Parallel + Always Spawn this means that each time we make a call to run() that we re-spin up all of the threads. This is incredibly time intesive since this requires us to repeat this for all 400 bulk task launches. This is especially clear given that the thread pool implementations perform much better, mainly because the threads remain running between each call to run().

Alternatively, we can see that serial begins to severely underperform in comparison to the thread pool implementations for tests such as mandelbrot_chunked.
```
Reference binary: ./runtasks_ref_linux_arm
Results for: mandelbrot_chunked
                                        STUDENT   REFERENCE   PERF?
[Serial]                                411.426   410.896     1.00  (OK)
[Parallel + Always Spawn]               26.121    26.125      1.00  (OK)
[Parallel + Thread Pool + Spin]         28.992    28.946      1.00  (OK)
[Parallel + Thread Pool + Sleep]        25.901    25.867      1.00  (OK)
```
This is mainly because this task is incredibly compute intensive and ideal for parallelizing across many threads. In this case, we can see all of the implementations with threads perform very well since, as we saw from assignment 1, mandelbrot is simple to split into chunks such that we can avoid processing each pixel sequentially. A key point to also observe is that always spawn and the other advanced methods of thread pooling are quite similar in performance. This is mainly because we only use a single bulk task launch so the loss in efficiency caused by needing to re-spinup the threads each time we need to launch tasks is not an issue in this test case.

3. The one test I implemented (running for both async and sync) executed a request that is possible either for asynchronous/synchronous testing but ran 0 tasks. I wanted to check that, even if we queue a set of tasks to get launched, especially asynchronously, that we don't error out improperly. This was to cover edge cases with regards to potentially breaking the thread pool with a 0 number of tasks.

This posed no additional issues to my code but was valuable for validating my implementation's consideration of edge cases.

The runtime is quick, as shown below for both running synchronously/asynchronously for part_b:
```
===================================================================================
Test name: your_test_async
===================================================================================
[Serial]:               [0.000] ms
[Parallel + Always Spawn]:              [0.000] ms
[Parallel + Thread Pool + Spin]:                [0.000] ms
[Parallel + Thread Pool + Sleep]:               [0.023] ms
===================================================================================
===================================================================================
Test name: your_test_sync
===================================================================================
[Serial]:               [0.000] ms
[Parallel + Always Spawn]:              [0.001] ms
[Parallel + Thread Pool + Spin]:                [0.001] ms
[Parallel + Thread Pool + Sleep]:               [0.121] ms
===================================================================================
```
