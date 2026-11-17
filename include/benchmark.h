#ifndef OS_BENCHMARK_H
#define OS_BENCHMARK_H

#include "common.h"
#include "os_syscalls.h"

class BenchmarkRunner {
public:
    static void display_banner();
    static void display_system_info();
    static void display_menu();
    static void run_full_suite(const std::string& self_exe, int iterations, int kalman_samples, int workers);
};

#endif // OS_BENCHMARK_H

