/**
 * Partition an array among multiple threads, each summing its portion. 
 * Use a designated atomic variable or a reduce-like operation to aggregate partial sums. 
 * Examine how the final summation can be performed with or without locks 
 * to highlight trade-offs in correctness, complexity, and speed.
 */

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>   
#include <numeric> //for std::accumulate 
#include <chrono>  //for std::chrono
#include <mutex> 
#include <algorithm>
#include <random> 
#include <cstdlib> // for rand()

#include "kaizen.h"

// Function to sum a portion of the array
void locked_sum(const std::vector<int>& arr, int start, int end, std::atomic<int>& total) {
    int sum = 0;
    for (int i = start; i < end; ++i) {
        sum += arr[i];
    }
    total += sum; // atomic addition
}

void unlocked_sum(const std::vector<int>& arr, int start, int end, int& total){
    int sum = 0;
    for (int i = start; i < end; ++i) {
        sum += arr[i];
    }
    total += sum; // non-atomic addition
}

int main(int argc, char* argv[]) {
    // Default values
    int num_threads = 4, array_size = 10000000;

    zen::cmd_args args(argv, argc);

    if (!args.is_present("--threads") || !args.is_present("--size")) {
        std::cerr << "Usage: " << argv[0] << " --threads <threads> --size <size>" << std::endl;
        return 1;
    }

    num_threads = std::stoi(args.get_options("--threads")[0]);
    array_size = std::stoi(args.get_options("--size")[0]);
    
    if (num_threads <= 0 || array_size <= 0) {
        std::cerr << "Invalid number of threads or size." << std::endl;
        return 1;
    }


    std::vector<int> arr(array_size);

    // fill array with random integers
    std::generate(arr.begin(), arr.end(), []() { return rand() % 100; });
    std::atomic<int> total_atomic(0);
    int total_unlocked = 0;
    std::vector<std::thread> threads;
    // Measure time for unlocked sum
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; ++i) {
        int start_index = i * (array_size / num_threads);
        int end_index = (i + 1) * (array_size / num_threads);
        threads.emplace_back(locked_sum, std::ref(arr), start_index, end_index, std::ref(total_atomic));
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Locked sum: " << total_atomic.load() << " in " << duration << " ms" << std::endl;
    threads.clear();
    start = std::chrono::high_resolution_clock::now();
    
    // Measure time for unlocked sum
    for (int i = 0; i < num_threads; ++i) {
        int start_index = i * (array_size / num_threads);
        int end_index = (i + 1) * (array_size / num_threads);
        threads.emplace_back(unlocked_sum, std::ref(arr), start_index, end_index, std::ref(total_unlocked));
    }
    for (auto& t : threads) {
        t.join();
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Unlocked sum: " << total_unlocked << " in " << duration << " ms" << std::endl;
}