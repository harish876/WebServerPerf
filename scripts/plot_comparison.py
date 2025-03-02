import subprocess
import re
import argparse
import os
import matplotlib.pyplot as plt
import numpy as np

def get_mean_of_medians(language, mode, num_runs, endpoint):
    result = subprocess.run(
        ['python3', 'collect_median.py', language, mode, str(num_runs), endpoint],
        capture_output=True,
        text=True
    )
    mean_of_medians = {}
    for line in result.stdout.splitlines():
        match = re.search(r'Mean of median times for concurrency level (\d+): ([\d.]+)', line)
        if match:
            concurrency = int(match.group(1))
            mean = float(match.group(2))
            mean_of_medians[concurrency] = mean
    return mean_of_medians

def plot_comparison(rust_means, c_means, mode, num_runs, endpoint):
    concurrency_levels = sorted(rust_means.keys())
    rust_values = [rust_means[concurrency] for concurrency in concurrency_levels]
    c_values = [c_means[concurrency] for concurrency in concurrency_levels]

    bar_width = 0.35
    index = np.arange(len(concurrency_levels))

    plt.bar(index, rust_values, bar_width, label='Rust')
    plt.bar(index + bar_width, c_values, bar_width, label='C')

    plt.xlabel('Concurrency Level')
    plt.ylabel('Mean of Median Times (ms)')
    plt.title(f'Comparison of Mean of Median Times for Rust and C - endpoint {endpoint} \nMode: {mode}, Runs: {num_runs}')
    plt.xticks(index + bar_width / 2, concurrency_levels)
    plt.legend()
    plt.grid(True)

    # Save the plot to a file
    output_dir = "../data/results"
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, f'comparison_{mode}_{num_runs}_{endpoint.replace("/", "_")}.png')
    plt.savefig(output_file)
    plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot comparison of mean of median times for Rust and C.')
    parser.add_argument('mode', type=str, help='The mode used.')
    parser.add_argument('num_runs', type=int, help='The number of runs.')
    parser.add_argument('endpoint', type=str, help='Endpoint which is tested. For example, /echo')

    args = parser.parse_args()

    rust_means = get_mean_of_medians('rust', args.mode, args.num_runs, args.endpoint)
    c_means = get_mean_of_medians('c', args.mode, args.num_runs, args.endpoint)
    plot_comparison(rust_means, c_means, args.mode, args.num_runs, args.endpoint)