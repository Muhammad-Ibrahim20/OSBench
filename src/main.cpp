#include "common.h"
#include "os_syscalls.h"
#include "kalman_filter.h"
#include "benchmark.h"
#include <iostream>
#include <string>

void print_help(const char* prog) {
    BenchmarkRunner::display_banner();
    std::cout << Color::WHITE << Color::BOLD << "USAGE:" << Color::RESET << "\n"
              << "  " << prog << " [options]\n\n"
              << Color::WHITE << Color::BOLD << "COMMAND-LINE OPTIONS:" << Color::RESET << "\n"
              << "  --help, -h                  Display this help menu\n"
              << "  --table1                    Display Table 1 (15 System Calls Matrix)\n"
              << "  --demo-all                  Run demonstrations for all 15 system calls\n"
              << "  --benchmark-syscalls [N]    Microbenchmark 15 system calls (default N=10000)\n"
              << "  --generate-data [N] [file]  Generate N synthetic sensor readings (default 50000)\n"
              << "  --kalman-single [in] [out]  Execute single-process system-call Kalman filter\n"
              << "  --kalman-parallel [in] [out] [W] Execute parallel Kalman filter (W workers, default 4)\n"
              << "  --kalman-compare [in] [W]   Compare single vs multi-process Kalman performance\n"
              << "  --run-all                   Execute complete automated benchmark pipeline\n"
              << "  (no arguments)              Launch interactive CLI dashboard\n\n";
}

int main(int argc, char* argv[]) {
    std::string self_exe = argv[0];

    // Special worker flag for child processes spawned via CreateProcess / exec
    if (argc >= 2 && std::string(argv[1]) == "--helper-ping") {
        return 0; // successfully responded
    }

    if (argc >= 5 && std::string(argv[1]) == "--worker") {
        int r_fd = std::stoi(argv[2]);
        int w_fd = std::stoi(argv[3]);
        std::string shm_name = argv[4];
        return KalmanEngine::run_worker_process(r_fd, w_fd, shm_name);
    }

    // CLI Flag Parsing
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "--table1") {
            SyscallEngine::print_table1_specification();
            return 0;
        } else if (arg == "--demo-all") {
            BenchmarkRunner::display_banner();
            SyscallEngine::run_all_demos(self_exe);
            return 0;
        } else if (arg == "--benchmark-syscalls") {
            int iters = (argc > 2) ? std::stoi(argv[2]) : 5000;
            BenchmarkRunner::display_banner();
            auto res = SyscallEngine::run_benchmarks(iters, self_exe);
            SyscallEngine::print_benchmark_table(res);
            SyscallEngine::export_benchmark_csv("benchmark_syscalls_report.csv", res);
            return 0;
        } else if (arg == "--generate-data") {
            size_t count = (argc > 2) ? std::stoull(argv[2]) : 50000;
            std::string path = (argc > 3) ? argv[3] : "sensor_trajectory_data.csv";
            KalmanEngine::generate_sensor_dataset(path, count);
            return 0;
        } else if (arg == "--kalman-single") {
            std::string in_path = (argc > 2) ? argv[2] : "sensor_trajectory_data.csv";
            std::string out_path = (argc > 3) ? argv[3] : "kalman_single_out.csv";
            KalmanStageMetrics m;
            size_t count = 0;
            KalmanEngine::run_single_process(in_path, out_path, m, count);
            return 0;
        } else if (arg == "--kalman-parallel") {
            std::string in_path = (argc > 2) ? argv[2] : "sensor_trajectory_data.csv";
            std::string out_path = (argc > 3) ? argv[3] : "kalman_multi_out.csv";
            int workers = (argc > 4) ? std::stoi(argv[4]) : 4;
            KalmanStageMetrics m;
            size_t count = 0;
            KalmanEngine::run_multi_process(in_path, out_path, workers, self_exe, m, count);
            return 0;
        } else if (arg == "--kalman-compare") {
            std::string in_path = (argc > 2) ? argv[2] : "sensor_trajectory_data.csv";
            int workers = (argc > 3) ? std::stoi(argv[3]) : 4;
            KalmanEngine::compare_performance(in_path, workers, self_exe);
            return 0;
        } else if (arg == "--run-all") {
            BenchmarkRunner::run_full_suite(self_exe, 5000, 50000, 4);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_help(argv[0]);
            return 1;
        }
    }

    // Interactive CLI Mode
    BenchmarkRunner::display_banner();
    BenchmarkRunner::display_system_info();

    while (true) {
        BenchmarkRunner::display_menu();
        int choice = -1;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::string trash;
            std::getline(std::cin, trash);
            std::cout << "\n";
            continue;
        }

        std::cout << "\n";
        switch (choice) {
            case 1:
                SyscallEngine::print_table1_specification();
                break;
            case 2:
                SyscallEngine::run_all_demos(self_exe);
                break;
            case 3: {
                std::cout << "Enter number of iterations per syscall [e.g. 5000]: ";
                int iters = 5000;
                std::cin >> iters;
                auto res = SyscallEngine::run_benchmarks(iters, self_exe);
                SyscallEngine::print_benchmark_table(res);
                SyscallEngine::export_benchmark_csv("benchmark_syscalls_report.csv", res);
                break;
            }
            case 4: {
                std::cout << "Enter sample count [e.g. 50000]: ";
                size_t samples = 50000;
                std::cin >> samples;
                KalmanEngine::generate_sensor_dataset("sensor_trajectory_data.csv", samples);
                break;
            }
            case 5: {
                KalmanStageMetrics m;
                size_t count = 0;
                KalmanEngine::run_single_process("sensor_trajectory_data.csv", "kalman_single_out.csv", m, count);
                break;
            }
            case 6: {
                std::cout << "Enter number of worker processes [e.g. 4]: ";
                int workers = 4;
                std::cin >> workers;
                KalmanStageMetrics m;
                size_t count = 0;
                KalmanEngine::run_multi_process("sensor_trajectory_data.csv", "kalman_multi_out.csv", workers, self_exe, m, count);
                break;
            }
            case 7: {
                std::cout << "Enter number of worker processes for comparison [e.g. 4]: ";
                int workers = 4;
                std::cin >> workers;
                KalmanEngine::compare_performance("sensor_trajectory_data.csv", workers, self_exe);
                break;
            }
            case 8:
                BenchmarkRunner::run_full_suite(self_exe, 5000, 50000, 4);
                break;
            case 0:
                std::cout << Color::GREEN << "Exiting OS Project CLI. Goodbye!\n" << Color::RESET;
                return 0;
            default:
                std::cout << Color::RED << "Invalid option. Please choose between 0 and 8.\n" << Color::RESET;
                break;
        }
        std::cout << "\n";
    }

    return 0;
}
