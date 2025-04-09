# COOOOL Multi-Threaded Summation Benchmark

This project is an advanced multi-threaded experiment that benchmarks several methods to sum the values of an array. The experiment illustrates trade-offs in correctness, complexity, and speed by comparing different strategies: using locks (atomics), intentionally unsafe (unlocked) operations, a reduce-like approach, and C++17 parallel algorithms.

The project also includes a CSV output of run results and a Python script to visualize the average run times per method and thread count. This makes the project an excellent demo of modern C++ concurrency techniques along with data visualization for performance analysis.

---

## Features

1. **Multiple Summation Methods**
   - **Locked:** Uses `std::atomic<int>` for safe, lock-free summation.
   - **Unlocked:** Intentionally unsynchronized summation (to illustrate race conditions).
   - **Reduce-like:** Each thread computes a partial sum which is then aggregated using `std::accumulate()`.
   - **Parallel:** Leverages C++17’s `std::reduce` with parallel execution policies.

2. **Flexible Command-Line Configuration**
   - Select the summing method using `--method` (options: `locked`, `unlocked`, `reduce`, `parallel`).
   - Specify a single thread count or a comma-separated list of thread counts via `--threads` to test scalability.
   - Set the size of the array with `--size`.
   - Define the number of warm-up and benchmark runs with `--warmup` and `--runs`.

3. **Benchmarking Techniques**
   - **Warm-Up Runs:** Execute a few preliminary runs to allow caching and thread pool stabilization.
   - **Multiple Runs:** Perform several timed runs for more reliable averaged results.
   - **Detailed CSV Logging:** Outputs benchmarking data (`Method, Threads, ArraySize, Run, Sum, Time_ms`) into a `results.csv` file.

4. **Array Distribution Options**
   - **rand:** Randomly initialized array.
   - **sorted:** Elements in ascending order.
   - **reverse:** Elements in descending order.
   - Specify the distribution with `--dist`.

5. **Thread Pool Integration**
   - Uses a simple thread pool to manage workload for the locked, unlocked, and reduce methods—demonstrating scalable task scheduling.

6. **C++17 Parallel Algorithms**
   - An alternative summation mode using the modern parallel capabilities of C++ (if supported by your compiler).

7. **Python Visualization Script**
   - A script (`plot_results.py`) to read the CSV results and generate a bar chart of the average execution times for each method across different thread counts. The resulting chart (`results.png`) can be directly included in your README or project documentation.

8. **Robustness & Logging**
   - Error handling and parameter validation are built in to make sure that invalid configurations are caught early and logged.

---

## File Overview

- **`main.cpp`**  
  Contains the full C++ implementation of the multi-threaded summation benchmark. It includes:
  - The simple thread pool implementation.
  - Four summation methods (locked, unlocked, reduce-like, and parallel).
  - Command-line parsing (via a presumed `kaizen.h` header).
  - Benchmarking logic (warm-up, multiple runs, timing).
  - Writing of benchmarking results to `results.csv`.

- **`kaizen.h`**  
  A header file assumed to provide a lightweight command-line argument parser.  

- **`plot_results.py`**  
  A Python script using `pandas` and `matplotlib` that reads `results.csv` and produces a bar chart of average run times.  

- **`results.csv`**  
  Automatically generated when you run the benchmark. Contains detailed run data.

- **`results.png`**  
  The generated visualization output from `plot_results.py`.

---

## Prerequisites

- **C++ Compiler:** A C++17 compliant compiler (e.g., g++ 7+, clang 6+).
- **POSIX Threads:** Ensure pthread support for multi-threading.
- **Python 3:** To run the visualization script.
- **Python Packages:** Install via pip if not already present:
  ```bash
  pip install pandas matplotlib
  ```

---

## Building the Project

Compile the C++ source using a command similar to:

```bash
g++ -std=c++17 -pthread main.cpp -o sum_experiment
```

This command builds the executable named `sum_experiment`.

---

## Running the Benchmark

The executable accepts several command-line options. For example:

```bash
./sum_experiment --threads 1,2,4,8 --size 10000000 --method reduce --runs 5 --warmup 2 --dist rand
```

**Parameter Descriptions:**

- `--threads`: Specifies the thread counts; can be a single value or a comma-separated list (e.g., `"1,2,4,8"`). For `parallel` mode, thread count is not used.
- `--size`: Size of the array to sum.
- `--method`: Summation method. Options:
  - `locked` — atomic-based (safe).
  - `unlocked` — intentionally unsynchronized (unsafe).
  - `reduce` — compute per-thread partial sums and then aggregate.
  - `parallel` — use C++17 parallel reduction.
- `--runs`: Number of timed benchmark runs (recorded in CSV).
- `--warmup`: Number of warm-up iterations before timing starts.
- `--dist`: Distribution for array initialization (`rand`, `sorted`, or `reverse`).

---
Output Example
After running the benchmark with the sample command, you might see the following output in the console:
```
Array of size 10000000 filled using distribution: rand

--- Running with 1 thread(s) using method: reduce ---
Run 1 - Sum: 495026639, Time: 22 ms
Run 2 - Sum: 495026639, Time: 23 ms
Run 3 - Sum: 495026639, Time: 24 ms
Run 4 - Sum: 495026639, Time: 84 ms
Run 5 - Sum: 495026639, Time: 26 ms

--- Running with 2 thread(s) using method: reduce ---
Run 1 - Sum: 495026639, Time: 18 ms
Run 2 - Sum: 495026639, Time: 16 ms
Run 3 - Sum: 495026639, Time: 22 ms
Run 4 - Sum: 495026639, Time: 16 ms
Run 5 - Sum: 495026639, Time: 15 ms

--- Running with 4 thread(s) using method: reduce ---
Run 1 - Sum: 495026639, Time: 5 ms
Run 2 - Sum: 495026639, Time: 6 ms
Run 3 - Sum: 495026639, Time: 6 ms
Run 4 - Sum: 495026639, Time: 6 ms
Run 5 - Sum: 495026639, Time: 7 ms

--- Running with 8 thread(s) using method: reduce ---
Run 1 - Sum: 495026639, Time: 5 ms
Run 2 - Sum: 495026639, Time: 5 ms
Run 3 - Sum: 495026639, Time: 6 ms
Run 4 - Sum: 495026639, Time: 5 ms
Run 5 - Sum: 495026639, Time: 6 ms

Results written to results.csv
```

Additionally, the Python script processes this CSV data and generates a visualization. Below is an example of what the visualized plot might look like:


The above chart [results.png](results.png) is a bar graph that shows the average execution time (in ms) for each summation method and thread count, providing a clear comparison of performance scalability.


## Visualizing the Results

Once you run the benchmark, a `results.csv` file is generated. To visualize the performance data:

1. Ensure you have Python 3 installed along with the `pandas` and `matplotlib` libraries.
2. Run the plotting script:
   ```bash
   python3 plot_results.py
   ```
3. The script will generate and display a bar chart (and save it as `results.png`) that compares the average execution times per method and thread count.

---

## Advanced Experimentation Ideas

- **Method Selection:** Easily switch between summation methods to observe differences in performance and correctness.
- **Scalability Testing:** Experiment with different thread counts to analyze the scalability of each approach.
- **Varied Array Distributions:** Use sorted or reverse distributions to examine the impact of data organization on performance.
- **Warm-Up and Multiple Runs:** Stabilize performance measurements with warm-up runs and report average results over multiple runs.
- **Performance Metrics:** Extend the CSV output or logging to include additional metrics such as CPU usage or memory footprint.
- **Error Handling & Logging:** Use the built-in logging to track anomalies or synchronization issues.
- **Integration with Other Tools:** Plug in additional analysis or visualization tools as needed.

---

## License

This project is released under the MIT License.

---

## Acknowledgments

Enjoy exploring advanced multi-threading and data visualization techniques!

---

Happy coding, benchmarking, and visualizing!
