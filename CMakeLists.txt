cmake_minimum_required(VERSION 3.10)
project(ParallelSummation LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sum_experiment main.cpp)


# Enable testing
enable_testing()

add_test(NAME RunSumExperiment
         COMMAND $<TARGET_FILE:sum_experiment> --threads 1 --size 1000000 --method reduce --runs 1 --warmup 0 --dist rand)
