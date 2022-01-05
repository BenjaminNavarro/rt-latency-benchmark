# rt-latency-benchmark

The goal of this tool is to measure latencies of OS schedulers for real-time applications.

The supported operating systems are:
 - Linux
 - macOS
 - Windows

The program runs a simple loop that does nothing but waiting for the next waking time.

At each iteration, the current timestamp is saved and once the loop has completed metrics are computed and displayed.

# Getting started

The project requires the following dependencies:
 - A C++17 compliant compiler
 - [fmt](https://github.com/fmtlib/fmt): tested with version 8.0.1 but older versions should work fine as well
 - [CLI11](https://github.com/CLIUtils/CLI11): tested with 2.1.1 but older versions should work fine as well

To build the project, you can install `fmt` and `CLI11` yourself or use the provided Conan configuration:
```
cd rt-latency-benchmark
mkdir build
cd build
conan install .. --build=missing -s build_type=Release
cmake -DCMAKE_MODULE_PATH=`pwd` -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

Note: On Windows you might need to pass also `-s compiler.cppstd=17 -s compiler.runtime=dynamic` (or similar) to the `conan install` command.

This will create an `rtbench` executable in the `build` folder

# Using rtbench

By default, `rtbench` will configure itself to execute as a real-time thread and then run a 1ms loop 1000 times.

These parameters can be tuned using command line parameters, e.g:
```
$ ./rtbench --realtime off --period 10 --loops 10000
```
This will run `rtbench` without real-time scheduling and perform 10000 iterations of a 10ms loop.

A typical output looks like:
```
$ ./rtbench
Running with the following options:
    period: 1ms
    loops: 1000
    realtime: true

Loop timings statistics:
    min: 0.975468ms
    max: 1.02564ms
    avg: 1.00001ms, stddev: 0.00572836ms (0.573%)
    median: 1.00003ms
```