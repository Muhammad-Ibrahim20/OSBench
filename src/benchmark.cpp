#include "benchmark.h"
#include "kalman_filter.h"
#include <iostream>
#include <thread>

void BenchmarkRunner::display_banner() {
    std::cout << Color::CYAN << Color::BOLD
              << "========================================================================================\n"
              << "   CT-353 OPERATING SYSTEMS - COMPLEX COMPUTING PROBLEM (CCP) PROJECT                   \n"
              << "   Cross-Platform System Call Analysis & Parallel Kalman Filter Computation             \n"
              << "========================================================================================\n"
              << Color::RESET;
}

void BenchmarkRunner::display_system_info() {
    std::cout << Color::WHITE << Color::BOLD << "[SYSTEM ENVIRONMENT INFORMATION]" << Color::RESET << "\n";
#if defined(OS_WINDOWS)
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    std::cout << "  * Operating System  : Microsoft Windows (NT Kernel)\n";
    std::cout << "  * CPU Architecture  : " << (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x86_64 (64-bit)" : "x86") << "\n";
    std::cout << "  * Logical Cores     : " << si.dwNumberOfProcessors << "\n";
    std::cout << "  * Memory Page Size  : " << si.dwPageSize << " bytes (4 KB)\n";
    std::cout << "  * Allocation Granularity: " << si.dwAllocationGranularity << " bytes (64 KB)\n";
#elif defined(OS_LINUX)
    std::cout << "  * Operating System  : Linux / POSIX Kernel\n";
    std::cout << "  * Logical Cores     : " << std::thread::hardware_concurrency() << "\n";
    std::cout << "  * Memory Page Size  : " << sysconf(_SC_PAGESIZE) << " bytes\n";
    std::cout << "  * Clock Ticks/Sec   : " << sysconf(_SC_CLK_TCK) << " Hz\n";
#endif
    std::cout << "  * Compiler          : " 
#if defined(__GNUC__)
              << "GCC/G++ " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__
#elif defined(_MSC_VER)
              << "MSVC " << _MSC_VER
#else
              << "Unknown C++ Compiler"
#endif
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n\n";
}

void BenchmarkRunner::display_menu() {
    std::cout << Color::YELLOW << Color::BOLD << "MAIN MENU / AVAILABLE ACTIONS:" << Color::RESET << "\n";
    std::cout << "  [1] Display Table 1 (15 System Calls Specification)\n";
    std::cout << "  [2] Demonstrate All 15 System Calls with Verification Logs\n";
    std::cout << "  [3] Run System Call Microbenchmarks & Latency Profiling\n";
    std::cout << "  [4] Generate Synthetic Sensor Trajectory Dataset\n";
    std::cout << "  [5] Run Single-Process System-Call-Based Kalman Filter\n";
    std::cout << "  [6] Run Multi-Process Parallel Kalman Filter (IPC + Shared Memory)\n";
    std::cout << "  [7] Run Full Comparative Evaluation (Single vs Multi-Process Speedup)\n";
    std::cout << "  [8] Run Complete Automated Test & Benchmark Pipeline\n";
    std::cout << "  [0] Exit\n";
    std::cout << Color::BOLD << "Enter your choice [0-8]: " << Color::RESET;
}

void BenchmarkRunner::run_full_suite(const std::string& self_exe, int iterations, int kalman_samples, int workers) {
    display_banner();
    display_system_info();

    log_info("Executing Stage 1: Demonstrating and validating 15 system calls...");
    SyscallEngine::run_all_demos(self_exe);

    log_info("Executing Stage 2: Microbenchmarking 15 system calls (" + std::to_string(iterations) + " iterations)...");
    auto results = SyscallEngine::run_benchmarks(iterations, self_exe);
    SyscallEngine::print_benchmark_table(results);
    SyscallEngine::export_benchmark_csv("benchmark_syscalls_report.csv", results);

    log_info("Executing Stage 3: Kalman Filter Pipeline Integration...");
    std::string dataset_file = "sensor_trajectory_data.csv";
    KalmanEngine::generate_sensor_dataset(dataset_file, kalman_samples, 0.01, 1.5);

    log_info("Executing Stage 4: Comparative Kalman Performance (Single vs " + std::to_string(workers) + " Workers)...");
    KalmanEngine::compare_performance(dataset_file, workers, self_exe);

    log_success("FULL BENCHMARK AND VALIDATION SUITE FINISHED SUCCESSFULLY!");
}
