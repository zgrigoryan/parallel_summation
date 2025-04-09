#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import sys

def main():
    try:
        df = pd.read_csv("results.csv")
    except Exception as e:
        print("Error reading results.csv:", e)
        sys.exit(1)

    # Compute average time per method and threads
    avg_df = df.groupby(["Method", "Threads"])["Time_ms"].mean().reset_index()
    
    # Pivot the table for plotting: rows=Threads, columns=Methods
    pivot = avg_df.pivot(index="Threads", columns="Method", values="Time_ms")
    pivot = pivot.sort_index()  # sort by Threads (0 is for parallel mode, treated as N/A)
    
    ax = pivot.plot(kind="bar", figsize=(10,6))
    ax.set_xlabel("Number of Threads (0 indicates Parallel Mode)")
    ax.set_ylabel("Average Time (ms)")
    ax.set_title("Benchmark: Average Time per Method vs. Thread Count")
    plt.tight_layout()
    plt.savefig("results.png")
    plt.show()

if __name__ == "__main__":
    main()
