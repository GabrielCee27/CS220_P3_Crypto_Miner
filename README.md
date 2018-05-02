# Project 3: Parallel Cryptocurrency Miner

See instructions here: https://www.cs.usfca.edu/~mmalensek/cs220/assignments/project-3.html

## Written Responses

Edit this readme.md file to answer the following questions:

### Busy-waiting vs. Condition Variables

Once you have condition variables working in your program, compare the performance of busy waiting with condition variables. To ensure a fair comparison, use a single thread.

Report for the original program as well as modified version:

Argument: ./mine 1 24 'Hello CS 220!!!'

Condition Variables:

1. Total run time: 1.85s
2. Hashes per second: 547563.49
3. CPU usage: 112.8%

Busy Waiting:

1. Total run time: 1.75s
2. Hashes per second: 576781.53
3. CPU usage: 199.7%

Which version performs better? Is this the result you expected? Why or why not?

In terms of speed, the busy waiting version performed better. I expected that to be the case due to the fact that busy waiting moves at a faster pace than conditional variables since conditional variables requires more communication. However, in terms of CPU usage conditional variables performed better than busy waiting. Again, I expected this to be the case since busy waiting inefficiently uses CPU to loop while waiting to continue as opposed to conditional variables where minimal CPU is used to wait to move on. 

### Nonces Per Task

Experiment with different values for `NONCES_PER_TASK`. What value yields the best performance in terms of hashes per second on your machine?

120 seems to be the best NONCES_PER_TASK for the best results.

### Performance I

When evaluating parallel programs, we use speedup and efficiency. Why are these metrics not as useful when measuring the performance of our parallel crytocurrency miner?


### Performance II

Using any of the `kudlick` machines (in our 220 classroom), what is the highest performance you were able to achieve in terms of hashes per second? What configuration did you use (`NONCES_PER_TASK`, number of threads, block data, compiler options)?
