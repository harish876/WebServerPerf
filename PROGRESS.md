## Project Update

### Methodology
1. For a simple echo endpoint:
   1. Used `taskset` to pin process to one CPU core.
   2. Used `epoll`, i.e., async I/O, as the mode for benchmarking C and Rust.
   3. Used `htop` on a corresponding tab to check CPU usage.
   4. Ran `perf` recording and recorded about 38K samples.
   5. On the client side, used 3 runs of 10K requests spaced by 20 seconds for concurrent connections in (250, 500, 750).
   6. Used a Python script to extract and calculate the mean of medians of the 3 runs and compare it over 3 concurrency levels.

### Progress
1. Spent some time getting the test client and testing methodology set up.
2. Next step is implementing `zlib` and JSON parsing, along with an endpoint that performs heavy CPU computation.

### Findings
1. Rust outperforms C when concurrent connections increase to 750. However, this appears to be within a margin of error, so additional runs are required. Each run takes about 2 minutes.
2. From a server standpoint, Rust demonstrates lower application-side overhead (0.43% of the time) compared to C (1.58%).

3. ![Concurrency Compare for `/echo` endpoint](/data/results/comparison_epoll_3_echo.png)

4. ![Rust Perf Data](/data/results/rust_perf_report.png)

5. ![C Perf Data](/data/results/c_perf_report.png)
