#pragma once

#include <thread>
#include <system_error>
#include <cstring>

#if defined(__linux__) or defined(__APPLE__)
#define USE_PTHREAD_API 1

#include <pthread.h>

#if defined(__APPLE__)
// Implement the missing POSIX functions
// Source https://yyshen.github.io/2015/01/18/binding_threads_to_cores_osx.html
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach_init.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
namespace {
#define SYSCTL_CORE_COUNT "machdep.cpu.core_count"

typedef struct cpu_set {
    uint32_t count;
} cpu_set_t;

static inline void CPU_ZERO(cpu_set_t* cs) {
    cs->count = 0;
}

static inline void CPU_SET(int num, cpu_set_t* cs) {
    cs->count |= (1 << num);
}

static inline int CPU_ISSET(int num, cpu_set_t* cs) {
    return (cs->count & (1 << num));
}

int sched_getaffinity(pid_t pid, size_t cpu_size, cpu_set_t* cpu_set) {
    int32_t core_count = 0;
    size_t len = sizeof(core_count);
    int ret = sysctlbyname(SYSCTL_CORE_COUNT, &core_count, &len, 0, 0);
    if (ret) {
        printf("error while get core count %d\n", ret);
        return -1;
    }
    cpu_set->count = 0;
    for (int i = 0; i < core_count; i++) {
        cpu_set->count |= (1 << i);
    }

    return 0;
}

int pthread_setaffinity_np(pthread_t thread, size_t cpu_size,
                           cpu_set_t* cpu_set) {
    thread_port_t mach_thread;
    int core = 0;

    for (core = 0; core < 8 * cpu_size; core++) {
        if (CPU_ISSET(core, cpu_set))
            break;
    }
    printf("binding to core %d\n", core);
    thread_affinity_policy_data_t policy = {core};
    mach_thread = pthread_mach_thread_np(thread);
    thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                      (thread_policy_t)&policy, 1);
    return 0;
}
} // namespace
#endif

#elif defined(_WIN32)
#define USE_PTHREAD_API 0
#include <windows.h>
#include <processthreadsapi.h>
#include <winbase.h>
#include <aclapi.h>
#endif

void make_thread_realtime(std::thread::native_handle_type handle) {
    // Start with last core as the first one might be used for kernel stuff by
    // the OS (at least on Windows)
    static unsigned core = std::thread::hardware_concurrency();
#if USE_PTHREAD_API
    // Enable SCHED_FIFO RT scheduler and set maximum priority
    struct sched_param sched_params = {.sched_priority = 99};
    if (pthread_setschedparam(handle, SCHED_FIFO, &sched_params) != 0) {
        throw std::system_error(errno, std::system_category(),
                                std::strerror(errno));
    }

    // Pin the thread to a core
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core - 1, &cpuset);

    if (pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset) != 0) {
        throw std::system_error(errno, std::system_category(),
                                std::strerror(errno));
    }
#else
    HANDLE h_proces = GetCurrentProcess();
    if (SetSecurityInfo(h_proces, SE_KERNEL_OBJECT, PROCESS_SET_INFORMATION,
                        nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
        throw std::runtime_error{"Failed to set PROCESS_SET_INFORMATION "};
    }

    // Set the thread to be RT with maximum priority
    if (SetPriorityClass(h_proces, REALTIME_PRIORITY_CLASS) == 0) {
        throw std::runtime_error{"Failed to set REALTIME_PRIORITY_CLASS"};
    }
    if (SetThreadPriority(handle, THREAD_PRIORITY_TIME_CRITICAL) == 0) {
        throw std::runtime_error{"Failed to set THREAD_PRIORITY_TIME_CRITICAL"};
    }

    // Pin the thread to a core
    if (SetThreadAffinityMask(handle, 1 << (core - 1)) == 0) {
        throw std::runtime_error{"Failed to set thread affinity mask"};
    }
#endif

    // Next call to this function will pin the thread to the next core
    --core;
    if (core == 0) {
        core = std::thread::hardware_concurrency();
    }
}