#ifndef OS_SYSCALLS_H
#define OS_SYSCALLS_H

#include "common.h"
#include <functional>
#include <map>

// Data structure to hold benchmark results for each system call
struct SyscallBenchResult {
    std::string category;
    std::string linux_call;
    std::string windows_call;
    std::string purpose;
    double avg_latency_ns = 0.0;
    double min_latency_ns = 0.0;
    double max_latency_ns = 0.0;
    double stddev_latency_ns = 0.0;
    double total_wall_ms = 0.0;
    double user_cpu_ms = 0.0;
    double sys_cpu_ms = 0.0;
    size_t peak_rss_kb = 0;
    long page_faults = 0;
    uint64_t operations_count = 0;
    bool success = true;
    std::string notes;
};

// System Call Interface & Demonstration Engine
class SyscallEngine {
public:
    static void print_table1_specification();
    
    // Individual category demonstrations with detailed logging
    static bool demo_file_io();
    static bool demo_process(const std::string& self_executable);
    static bool demo_memory();
    static bool demo_ipc_pipe();
    static bool demo_ipc_shared_memory();
    static bool demo_permissions();

    // Run all demonstrations sequentially
    static bool run_all_demos(const std::string& self_executable);

    // Microbenchmark suite for the 15 system calls
    static std::vector<SyscallBenchResult> run_benchmarks(int iterations = 10000, const std::string& self_exe = "");
    
    // Print and export benchmark report
    static void print_benchmark_table(const std::vector<SyscallBenchResult>& results);
    static bool export_benchmark_csv(const std::string& filepath, const std::vector<SyscallBenchResult>& results);
};

#endif // OS_SYSCALLS_H

