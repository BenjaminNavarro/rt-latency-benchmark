#include <fmt/format.h>
#include <fmt/chrono.h>

#include <CLI/CLI.hpp>

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include "real_time.h"

int main(int argc, const char* argv[]) {
    CLI::App cli{"Realtime latency benchmarking application"};

    size_t loops{1000};
    cli.add_option("-l,--loops", loops,
                   fmt::format("Number of loops to execute ({})", loops));

    double period_ms{1};
    cli.add_option("-p,--period", period_ms,
                   fmt::format("Loop period in milliseconds ({})", period_ms));

    bool realtime{true};
    cli.add_option(
        "-r,--realtime", realtime,
        fmt::format(
            "Whether or not configuring the thread for realtime execution ({})",
            realtime));

    CLI11_PARSE(cli, argc, argv);

    fmt::print("Running with the following options:\n");
    fmt::print("    period: {}ms\n", period_ms);
    fmt::print("    loops: {}\n", loops);
    fmt::print("    realtime: {}\n", realtime);

    if (realtime) {
#if USE_PTHREAD_API
        make_thread_realtime(pthread_self());
#else
        make_thread_realtime(GetCurrentThread());
#endif
    }

    using clock = std::chrono::steady_clock;
    using duration = std::chrono::duration<double, std::ratio<1, 1000>>;

    std::vector<clock::time_point> timestamps{loops};
    const auto start = clock::now();
    for (size_t i = 0; i < loops; i++) {
        timestamps[i] = clock::now();
        std::this_thread::sleep_until(start + (i + 1) * duration{period_ms});
    }

    std::vector<duration> diffs;
    diffs.reserve(loops - 1);
    for (size_t i = 0; i < loops - 1; i++) {
        diffs.emplace_back(timestamps[i + 1] - timestamps[i]);
    }

    const auto [min_it, max_it] = std::minmax_element(begin(diffs), end(diffs));
    const auto min = *min_it;
    const auto max = *max_it;
    const auto sum = std::accumulate(begin(diffs), end(diffs), duration{0.0});
    const auto avg = sum / diffs.size();

    const auto sq_sum = std::inner_product(
        begin(diffs), end(diffs), begin(diffs), duration{0.0},
        [](duration a, duration b) { return a + b; },
        [](duration a, duration b) { return duration{a.count() * b.count()}; });
    const auto stddev =
        duration{std::sqrt(sq_sum.count() / static_cast<double>(diffs.size()) -
                           avg.count() * avg.count())};

    std::sort(begin(diffs), end(diffs));
    const auto median = diffs[diffs.size() / 2];

    fmt::print("\nLoop timings statistics:\n");
    fmt::print("    min: {}\n", min);
    fmt::print("    max: {}\n", max);
    fmt::print("    avg: {}, stddev: {} ({:.3}%)\n", avg, stddev,
               stddev.count() * 100. / period_ms);
    fmt::print("    median: {}\n", median);
}
