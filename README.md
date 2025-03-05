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
1. Done: Spent some time getting the test client and testing methodology set up.
2. Done: JSON endpoint to measure serialisation/deserialisation cost
3. TODO: Next step is implementing `zlib` and JSON parsing, along with an endpoint that performs heavy CPU computation.


## Challenges
1. naive implementation stalls for post request using epoll. works with postman but i dont understand why it is stalling.need to look into it more.
2. so used standard low level libraries for both c and rust which both rely on epoll. used `tokio` + `hyper` for rust and `libmicrohttpd` GNU extension for C. these arent apples to apples comparision but since these are used by high level libraries -  using these and benchmarking is not totally off. both use epoll for async I/O configured with to a single thread.
3. Use perf and bcc ebpf profiler proved to be difficult to use to generate flamegraphs. There should be a way its just that even documentation examples did not run properly i.e flamegraph gen was not happening nothing wrong with perf i guess. i would like some a second set of eyes here
4. used `kout/not-perf` github library for flamegraph. made some changes to the source and got it to work

### Findings for JSON endpoint

1. Rust outperforms C for the `json` endpoint. Reason is that `epoll_wait` syscall is on CPU in rust for 2.66% vs in C it is 12.51%.The C http implementation wakes up more often to check on connections than tokio which is more efficient when it comes to listening on active connections. (Plase open the flamegraphs in browser to zoom into the call stack and search for functions.) 

2. ![Latency for `json` endpoint](/data/results/comparison_median_3p_5_json.png)
   *Figure 1: Latency comparison for `json` endpoint between Rust and C implementations.*

3. ![Throughput for `json` endpoint](/data/results/comparison_rps_3p_5_json.png)
   *Figure 2: Throughput comparison for `json` endpoint between Rust and C implementations.*

4. ![Rust Flamegraph](/data/profiles/flamegraph_rust-tokio_passing-duck5.svg)
   *Figure 3: Flamegraph for Rust implementation using Tokio.*

5. ![C Flamegraph](/data/profiles/flamegraph_c-httpd_passing-duck5.svg)
   *Figure 4: Flamegraph for C implementation using libmicrohttpd.*

### TODO
1. Structure results neatly.
2. Debug issues in native implementation.