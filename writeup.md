# Writeup

1. Task system implementation

For the sleeping thread pool implementation:
* Management of threads: 
- When initializing the thread pool sleeping class, the threads immediately kick off and begin running in a function `sleepingWork`
    - For this sleeping work function, the threads continuously loop
    - Then, they wait based on a condition variable and once there are tasks that are available within a queue, they will stop waiting and will grab the next task ID and the proper runnable object that was set based on the run() function's runnable instance
    - Then, it will run the task and ensure that we haven't run out of tasks. In the case we have, we need to notify the main thread that is waiting on the tasks (at least for the synchronous implementation)
    - The main thread that's waiting for the tasks to finish will then wait for all the tasks to complete and then join all the threads
- These threads are managed as:
    - A queue of tasks that we know we need to execute which will fill up and then the spinning threads will grab
    - We put the threads to sleep when there's no work to do and, if there is work that is added (by enqueuing the tasks), we wake up the threads
    - We track both when the work is complete after having queued a set of tasks and when the destructor activates (signifying the main thread needs the spinning threads to exit). This is done through flags updated with locks and condition variables to notify the workers to wake up and stop.
- With regards to the asynchronous implementation:
    - if we run synchronously, simply execute the asynchronous function with 0 dependencies and sync it immediately
        - a ready queue for tasks that have no dependencies that should be run
- My system assigns work to threads dynamically. This is simplest because we don't need to predetermine the work for each thread but instead have the work executed as when it becomes available. So, when we enqueue a new task to be executed, we simply notify the threads of this such that they can pick it up and execute it.
- I tracked dependencies in task B as so:
    - A struct to store the runnable pointer, number of tasks that runnable function should run, the dependencies this task has, and the children who depend on this task. 
    - It is important we have the dependencies this task has such that we can determine when these tasks can be moved to the ready queue and the children are necessary to track when those respective tasks should be kicked off. We can do so by tracking the remaining dependencies for each set of tasks and then adding these sets of tasks to the ready queue once they have 0 remaining dependencies tracked for that set of tasks
    - We can avoid looping through all tasks by tracking the details of every task, including their remaining dependencies in a map and leveraging the fact that we have both the dependencies for a set of tasks and the children that depend on it such that we can decrement the number of remaining dependencies per set of tasks appropriately

2. We can observe that, especially for super_super_light vs super_light
Results for: super_super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                9.011     9.124       0.99  (OK)
[Parallel + Always Spawn]               56.498    54.875      1.03  (OK)
[Parallel + Thread Pool + Spin]         15.22     13.124      1.16  (OK)
[Parallel + Thread Pool + Sleep]        10.157    10.197      1.00  (OK)
================================================================================
Executing test: super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                71.229    67.793      1.05  (OK)
[Parallel + Always Spawn]               62.419    62.448      1.00  (OK)
[Parallel + Thread Pool + Spin]         30.672    27.977      1.10  (OK)
[Parallel + Thread Pool + Sleep]        23.684    22.609      1.05  (OK)
================================================================================

The most important part about these results are that we can see the serial execution quickly begin to underperform between these
3. The one test I implemented executed a request that is possible either for asynchronous/synchronous testing but ran 0 tasks. I wanted to check that, even if we queue a set of tasks to get launched, especially asynchronously, that we don't error out improperly. This was to cover edge cases with regards to potentially breaking the thread pool with a 0 number of tasks.

This posed no additional issues to my code but was valuable for validating my implementation's consideration of edge cases.

The runtime is quick, as shown below for both running synchronously/asynchronously for part_b:
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
