cmake_minimum_required(VERSION 3.8.2)

project(
    rt-latency-benchmark
    LANGUAGES CXX
    VERSION 0.1.0
)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(CLI11 REQUIRED)

add_executable(
    rtbench
    rtbench/main.cpp
)

target_link_libraries(
    rtbench
    PRIVATE
        Threads::Threads
        fmt::fmt
        CLI11::CLI11
)

target_compile_features(
    rtbench
    PRIVATE
        cxx_std_17
)