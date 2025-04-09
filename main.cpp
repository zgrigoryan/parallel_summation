#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>
#include <numeric>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <queue>
#include <future>
#include <functional>

// For C++17 parallel algorithm
#ifdef __cpp_lib_execution
#include <execution>
#endif

#include "kaizen.h"

// ------------------ Simple Thread Pool Implementation ---------------------
class ThreadPool {
public:
    ThreadPool(size_t num_threads)
        : stop(false)
    {
        for(size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this]() {
                for(;;) {
                    std::function<void()> task;
                    
                    {   // acquire lock
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this]() { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    
                    // execute task
                    task();
                }
            });
        }
    }
    
    template<class F, class... Args>
    auto submit(F&& f, Args&&... args)
      -> std::future<typename std::result_of<F(Args...)>::type>
    {
      using return_type = typename std::result_of<F(Args...)>::type;
      
      auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
      
      std::future<return_type> res = task->get_future();
      {
          std::unique_lock<std::mutex> lock(queue_mutex);
          tasks.emplace([task](){ (*task)(); });
      }
      condition.notify_one();
      return res;
    }
    
    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread &worker: workers)
            worker.join();
    }
    
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};
// ------------------ End Thread Pool ---------------------------------------

// Summation functions

// 1. Locked sum: each thread uses an atomic variable for safe addition.
void locked_sum(const std::vector<int>& arr, int start, int end, std::atomic<int>& total) {
    int sum = 0;
    for (int i = start; i < end; ++i) {
        sum += arr[i];
    }
    total += sum; // atomic addition is thread-safe
}

// 2. Unlocked sum: intentionally unsafe (data race) to illustrate issues.
void unlocked_sum(const std::vector<int>& arr, int start, int end, int& total) {
    int sum = 0;
    for (int i = start; i < end; ++i) {
        sum += arr[i];
    }
    total += sum; // unsafe addition, no synchronization
}

// 3. Reduce-like operation: each thread computes a partial sum.
void reduce_sum(const std::vector<int>& arr, int start, int end, int& partial_sum) {
    int sum = 0;
    for (int i = start; i < end; ++i) {
        sum += arr[i];
    }
    partial_sum = sum;
}

// 4. Parallel algorithm mode: uses C++17 parallel reduction.
int parallel_sum(const std::vector<int>& arr) {
#ifdef __cpp_lib_execution
    return std::reduce(std::execution::par, arr.begin(), arr.end(), 0);
#else
    // If not available, fall back to sequential accumulate.
    return std::accumulate(arr.begin(), arr.end(), 0);
#endif
}

// Utility to split a comma-separated string into integers.
std::vector<int> parseThreadCounts(const std::string& s) {
    std::vector<int> counts;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        try {
            counts.push_back(std::stoi(token));
        } catch (...) {
            std::cerr << "Invalid thread count value: " << token << std::endl;
        }
    }
    return counts;
}

// Utility to fill the array based on distribution type.
void fillArray(std::vector<int>& arr, const std::string& dist) {
    if (dist == "sorted") {
        for (size_t i = 0; i < arr.size(); ++i) {
            arr[i] = static_cast<int>(i);
        }
    }
    else if (dist == "reverse") {
        for (size_t i = 0; i < arr.size(); ++i) {
            arr[i] = static_cast<int>(arr.size() - i);
        }
    }
    else { // default "rand"
        std::generate(arr.begin(), arr.end(), []() { return rand() % 100; });
    }
}

int main(int argc, char* argv[]) {
    // Default parameters
    std::string method = "locked";       // locked, unlocked, reduce, parallel
    std::string thread_option = "4";       // single value or comma-separated list (e.g., "1,2,4,8")
    int array_size = 10000000;
    int runs = 5;       // number of timed benchmark runs (after warmup)
    int warmup = 2;     // number of warm-up runs (not recorded)
    std::string distribution = "rand"; // options: "rand", "sorted", "reverse"
    
    // Use kaizen library for cmd args (assumed available in "kaizen.h")
    zen::cmd_args args(argv, argc);
    if (!args.is_present("--size") || !args.is_present("--threads")) {
        std::cerr << "Usage: " << argv[0] 
                  << " --threads <thread_counts (comma-separated)> --size <array_size> [--method locked|unlocked|reduce|parallel] [--runs <n>] [--warmup <n>] [--dist rand|sorted|reverse]" << std::endl;
        return 1;
    }
    
    // Parse command-line parameters
    try {
        thread_option = args.get_options("--threads")[0];
        array_size = std::stoi(args.get_options("--size")[0]);
        if (args.is_present("--method"))
            method = args.get_options("--method")[0];
        if (args.is_present("--runs"))
            runs = std::stoi(args.get_options("--runs")[0]);
        if (args.is_present("--warmup"))
            warmup = std::stoi(args.get_options("--warmup")[0]);
        if (args.is_present("--dist"))
            distribution = args.get_options("--dist")[0];
    } catch (...) {
        std::cerr << "Error parsing command-line arguments." << std::endl;
        return 1;
    }
    
    // Parse thread counts (supporting comma-separated list)
    std::vector<int> thread_counts = parseThreadCounts(thread_option);
    if (thread_counts.empty()) {
        std::cerr << "No valid thread counts provided." << std::endl;
        return 1;
    }
    
    // Prepare the array
    std::vector<int> arr(array_size);
    fillArray(arr, distribution);
    std::cout << "Array of size " << array_size << " filled using distribution: " << distribution << std::endl;
    
    // Open CSV for output
    std::ofstream csv_file("results.csv");
    if (!csv_file.is_open()) {
        std::cerr << "Failed to open results.csv for writing." << std::endl;
        return 1;
    }
    csv_file << "Method,Threads,ArraySize,Run,Sum,Time_ms\n";
    
    // Loop through each specified thread count (for scalability experiments).
    for (int n_threads : thread_counts) {
        std::cout << "\n--- Running with " << n_threads << " thread(s) using method: " << method << " ---" << std::endl;
        // For methods other than parallel, we use our thread pool.
        // For each configuration, perform warm-up runs first.
        for (int i = 0; i < warmup; ++i) {
            if (method == "parallel") {
                volatile int sum = parallel_sum(arr);
                (void)sum;
            } else if (method == "locked") {
                std::atomic<int> total_atomic(0);
                ThreadPool pool(n_threads);
                int block = array_size / n_threads;
                std::vector<std::future<void>> futures;
                for (int t = 0; t < n_threads; ++t) {
                    int start_index = t * block;
                    int end_index = (t == n_threads - 1) ? array_size : (t + 1) * block;
                    futures.push_back(pool.submit(locked_sum, std::ref(arr), start_index, end_index, std::ref(total_atomic)));
                }
                for(auto &f: futures) { f.get(); }
            } else if (method == "unlocked") {
                int total_unlocked = 0;
                ThreadPool pool(n_threads);
                int block = array_size / n_threads;
                std::vector<std::future<void>> futures;
                for (int t = 0; t < n_threads; ++t) {
                    int start_index = t * block;
                    int end_index = (t == n_threads - 1) ? array_size : (t + 1) * block;
                    futures.push_back(pool.submit(unlocked_sum, std::ref(arr), start_index, end_index, std::ref(total_unlocked)));
                }
                for(auto &f: futures) { f.get(); }
            } else if (method == "reduce") {
                std::vector<int> partial_sums(n_threads, 0);
                ThreadPool pool(n_threads);
                int block = array_size / n_threads;
                std::vector<std::future<void>> futures;
                for (int t = 0; t < n_threads; ++t) {
                    int start_index = t * block;
                    int end_index = (t == n_threads - 1) ? array_size : (t + 1) * block;
                    futures.push_back(pool.submit(reduce_sum, std::ref(arr), start_index, end_index, std::ref(partial_sums[t])));
                }
                for(auto &f: futures) { f.get(); }
                volatile int s = std::accumulate(partial_sums.begin(), partial_sums.end(), 0);
                (void)s;
            }
        }
        
        // Now perform the timed runs.
        for (int run = 0; run < runs; ++run) {
            int sum_result = 0;
            auto start_time = std::chrono::high_resolution_clock::now();
            if (method == "parallel") {
                // Note: thread count is not used in parallel mode.
                sum_result = parallel_sum(arr);
            }
            else if (method == "locked") {
                std::atomic<int> total_atomic(0);
                ThreadPool pool(n_threads);
                int block = array_size / n_threads;
                std::vector<std::future<void>> futures;
                for (int t = 0; t < n_threads; ++t) {
                    int start_index = t * block;
                    int end_index = (t == n_threads - 1) ? array_size : (t + 1) * block;
                    futures.push_back(pool.submit(locked_sum, std::ref(arr), start_index, end_index, std::ref(total_atomic)));
                }
                for(auto &f: futures) { f.get(); }
                sum_result = total_atomic.load();
            }
            else if (method == "unlocked") {
                int total_unlocked = 0;
                ThreadPool pool(n_threads);
                int block = array_size / n_threads;
                std::vector<std::future<void>> futures;
                for (int t = 0; t < n_threads; ++t) {
                    int start_index = t * block;
                    int end_index = (t == n_threads - 1) ? array_size : (t + 1) * block;
                    futures.push_back(pool.submit(unlocked_sum, std::ref(arr), start_index, end_index, std::ref(total_unlocked)));
                }
                for(auto &f: futures) { f.get(); }
                sum_result = total_unlocked;
            }
            else if (method == "reduce") {
                std::vector<int> partial_sums(n_threads, 0);
                ThreadPool pool(n_threads);
                int block = array_size / n_threads;
                std::vector<std::future<void>> futures;
                for (int t = 0; t < n_threads; ++t) {
                    int start_index = t * block;
                    int end_index = (t == n_threads - 1) ? array_size : (t + 1) * block;
                    futures.push_back(pool.submit(reduce_sum, std::ref(arr), start_index, end_index, std::ref(partial_sums[t])));
                }
                for(auto &f: futures) { f.get(); }
                sum_result = std::accumulate(partial_sums.begin(), partial_sums.end(), 0);
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            std::cout << "Run " << run + 1 << " - Sum: " << sum_result << ", Time: " << elapsed << " ms" << std::endl;
            // For the parallel method, record thread count as 0 (or "N/A")
            csv_file << method << "," << ((method=="parallel") ? 0 : n_threads) << "," 
                     << array_size << "," << run + 1 << "," << sum_result << "," << elapsed << "\n";
        }
    }
    
    csv_file.close();
    std::cout << "\nResults written to results.csv" << std::endl;
    return 0;
}
