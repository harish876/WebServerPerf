import subprocess
import re
import argparse
import os
import matplotlib.pyplot as plt
import numpy as np

def get_mean_of_medians_and_rps(language, mode, num_runs, endpoint):
    result = subprocess.run(
        ['python3', 'collect_median.py', language, mode, str(num_runs), endpoint],
        capture_output=True,
        text=True
    )
    mean_of_medians = {}
    mean_rps = {}
    for line in result.stdout.splitlines():
        median_match = re.search(r'Mean of median times for concurrency level (\d+): ([\d.]+)', line)
        rps_match = re.search(r'Mean throughput for concurrency level (\d+): ([\d.]+) Requests/sec', line)
        if median_match:
            concurrency = int(median_match.group(1))
            mean = float(median_match.group(2))
            mean_of_medians[concurrency] = mean
        if rps_match:
            concurrency = int(rps_match.group(1))
            rps = float(rps_match.group(2))
            mean_rps[concurrency] = rps
    return mean_of_medians, mean_rps

def plot_comparison(rust_means, c_means, rust_rps, c_rps, mode, num_runs, endpoint):
    concurrency_levels = sorted(rust_means.keys())
    rust_values = [rust_means[concurrency] if concurrency in rust_means else 0 for concurrency in concurrency_levels]
    c_values = [c_means[concurrency] if concurrency in c_means else 0 for concurrency in concurrency_levels]
    rust_rps_values = [rust_rps[concurrency] if concurrency in rust_rps else 0 for concurrency in concurrency_levels]
    c_rps_values = [c_rps[concurrency] if concurrency in c_rps else 0 for concurrency in concurrency_levels]

    bar_width = 0.35
    index = np.arange(len(concurrency_levels))

    # Plot for Mean of Median Times
    fig, ax1 = plt.subplots()
    ax1.bar(index, rust_values, bar_width, label='Rust Median Time')
    ax1.bar(index + bar_width, c_values, bar_width, label='C Median Time')
    ax1.set_xlabel('Concurrency Level')
    ax1.set_ylabel('Mean of Median Times (ms)')
    ax1.set_title(f'Comparison of Mean of Median Times for Rust and C - endpoint {endpoint} \nMode: {mode}, Runs: {num_runs}')
    ax1.set_xticks(index + bar_width / 2)
    ax1.set_xticklabels(concurrency_levels)
    ax1.legend()
    ax1.grid(True)

    # Save the plot for Mean of Median Times
    output_dir = "../data/results"
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, f'comparison_median_{mode}_{num_runs}_{endpoint.replace("/", "_")}.png')
    plt.savefig(output_file)

    # Plot for Requests Per Second (RPS)
    fig, ax2 = plt.subplots()
    ax2.bar(index, rust_rps_values, bar_width, label='Rust RPS')
    ax2.bar(index + bar_width, c_rps_values, bar_width, label='C RPS')
    ax2.set_xlabel('Concurrency Level')
    ax2.set_ylabel('Requests Per Second (RPS)')
    ax2.set_title(f'Comparison of RPS for Rust and C - endpoint {endpoint} \nMode: {mode}, Runs: {num_runs}')
    ax2.set_xticks(index + bar_width / 2)
    ax2.set_xticklabels(concurrency_levels)
    ax2.legend()
    ax2.grid(True)

    # Save the plot for RPS
    output_file = os.path.join(output_dir, f'comparison_rps_{mode}_{num_runs}_{endpoint.replace("/", "_")}.png')
    plt.savefig(output_file)

    plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot comparison of mean of median times and RPS for Rust and C.')
    parser.add_argument('mode', type=str, help='The mode used.')
    parser.add_argument('num_runs', type=int, help='The number of runs.')
    parser.add_argument('endpoint', type=str, help='Endpoint which is tested. For example, /echo')

    args = parser.parse_args()

    rust_means, rust_rps = get_mean_of_medians_and_rps('rust', args.mode, args.num_runs, args.endpoint)
    c_means, c_rps = get_mean_of_medians_and_rps('c', args.mode, args.num_runs, args.endpoint)
    plot_comparison(rust_means, c_means, rust_rps, c_rps, args.mode, args.num_runs, args.endpoint)