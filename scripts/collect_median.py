import os
import re
import argparse
import statistics

def extract_median_times_and_throughput(file_path):
    median_times = []
    throughputs = []
    failed_tries = 0
    with open(file_path, 'r') as file:
        content = file.read()
        runs = re.split(r'Run \d+:', content)
        for run in runs[1:]:  # Skip the first split part as it is before the first "Run X:"
            median_match = re.search(r'50%\s+(\d+)', run)
            throughput_match = re.search(r'Requests per second:\s+([\d.]+)', run)
            if median_match:
                median_time = int(median_match.group(1))
                median_times.append(median_time)
            else:
                failed_tries += 1
            if throughput_match:
                throughput = float(throughput_match.group(1))
                throughputs.append(throughput)
    return median_times, throughputs, failed_tries

def main(language, mode, num_runs, endpoint):
    output_dir = f"../data/tests/{endpoint}"
    if not os.path.exists(output_dir):
        print(f"Experiment directory does not exist")
        return
    
    concurrency_levels = [250, 500, 750]
    
    print(f"Language: {language}")
    print(f"Mode: {mode}")
    print(f"Number of runs: {num_runs}")
    print(f"Endpoint: {endpoint}")
    print("---" * 20)

    for concurrency in concurrency_levels:
        file_name = f"{language}_{mode}_{concurrency}_{num_runs}.txt"
        file_path = os.path.join(output_dir, file_name)
        if os.path.exists(file_path):
            median_times, throughputs, failed_tries = extract_median_times_and_throughput(file_path)
            if median_times:
                mean_of_medians = statistics.mean(median_times)
                mean_throughput = statistics.mean(throughputs) if throughputs else 0
                print(f"Median times for {file_name}: {median_times}")
                print(f"Mean of median times for concurrency level {concurrency}: {mean_of_medians}")
                print(f"Throughputs for {file_name}: {throughputs}")
                print(f"Mean throughput for concurrency level {concurrency}: {mean_throughput} Requests/sec")
            else:
                print(f"No median times found for {file_name}.")
            print(f"Number of failed tries for {file_name}: {failed_tries}")
        else:
            print(f"File {file_path} does not exist.")
            
        print("---" * 20)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Extract median times and throughput from benchmark files.')
    parser.add_argument('language', type=str, help='The programming language used.')
    parser.add_argument('mode', type=str, help='The mode used.')
    parser.add_argument('num_runs', type=int, help='The number of runs.')
    parser.add_argument('endpoint', type=str, help='The endpoint to be profiled.')

    args = parser.parse_args()
    
    main(args.language, args.mode, args.num_runs, args.endpoint)