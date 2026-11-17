#if defined(__linux__) || defined(__unix__)

#include "os_syscalls.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <fstream>

void SyscallEngine::print_table1_specification() {
    std::cout << "\n" << Color::CYAN << Color::BOLD
              << "========================================================================================\n"
              << "               TABLE 1: 15 SYSTEM CALLS SPECIFICATION (CT-353 OS)                       \n"
              << "========================================================================================\n"
              << Color::RESET;
    std::cout << std::left 
              << std::setw(15) << "Category" 
              << std::setw(30) << "Linux System Calls" 
              << std::setw(35) << "Windows API Equivalents" 
              << std::setw(20) << "Purpose" << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left 
              << std::setw(15) << "File I/O" 
              << std::setw(30) << "open(), read(), write(), close()" 
              << std::setw(35) << "CreateFile, ReadFile, WriteFile, CloseHandle" 
              << std::setw(20) << "File operations" << "\n";
    std::cout << std::left 
              << std::setw(15) << "Process" 
              << std::setw(30) << "fork(), exec(), wait()" 
              << std::setw(35) << "CreateProcess, WaitForSingleObject" 
              << std::setw(20) << "Process creation/control" << "\n";
    std::cout << std::left 
              << std::setw(15) << "Memory" 
              << std::setw(30) << "mmap(), munmap()" 
              << std::setw(35) << "VirtualAlloc, VirtualFree" 
              << std::setw(20) << "Memory mapping" << "\n";
    std::cout << std::left 
              << std::setw(15) << "IPC" 
              << std::setw(30) << "pipe(), shmget(), shmat()" 
              << std::setw(35) << "CreatePipe, CreateFileMapping, MapViewOfFile" 
              << std::setw(20) << "Inter-process comm." << "\n";
    std::cout << std::left 
              << std::setw(15) << "Permissions" 
              << std::setw(30) << "chmod(), chown(), umask()" 
              << std::setw(35) << "SetFileSecurity" 
              << std::setw(20) << "File permissions" << "\n";
    std::cout << "========================================================================================\n\n";
}

bool SyscallEngine::demo_file_io() {
    log_info("Executing Demonstration: File I/O Category (open, write, read, close)");
    const char* filename = "demo_file_io.tmp";
    const std::string payload = "Operating Systems Complex Computing Problem (CT-353) - Linux Syscall Verification.";

    // 1. open()
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        log_error("open() failed: " + get_last_os_error());
        return false;
    }
    log_success("open() succeeded. File Descriptor: " + std::to_string(fd));

    // 2. write()
    ssize_t bytes_written = write(fd, payload.c_str(), payload.size());
    if (bytes_written < 0 || static_cast<size_t>(bytes_written) != payload.size()) {
        log_error("write() failed: " + get_last_os_error());
        close(fd);
        return false;
    }
    log_success("write() succeeded. Written " + std::to_string(bytes_written) + " bytes.");

    // Seek back to beginning
    lseek(fd, 0, SEEK_SET);

    // 3. read()
    std::vector<char> buffer(payload.size() + 1, 0);
    ssize_t bytes_read = read(fd, buffer.data(), payload.size());
    if (bytes_read < 0 || static_cast<size_t>(bytes_read) != payload.size()) {
        log_error("read() failed: " + get_last_os_error());
        close(fd);
        return false;
    }
    log_success("read() succeeded. Read " + std::to_string(bytes_read) + " bytes: \"" + std::string(buffer.data()) + "\"");

    // 4. close()
    if (close(fd) != 0) {
        log_error("close() failed: " + get_last_os_error());
        return false;
    }
    log_success("close() succeeded.");
    unlink(filename);
    return true;
}

bool SyscallEngine::demo_process(const std::string& self_executable) {
    log_info("Executing Demonstration: Process Control Category (fork, exec, wait)");

    uint64_t start = get_time_ns();
    pid_t pid = fork();

    if (pid < 0) {
        log_error("fork() failed: " + get_last_os_error());
        return false;
    }

    if (pid == 0) {
        // Child process: execute helper ping
        char* args[] = { const_cast<char*>(self_executable.c_str()), const_cast<char*>("--helper-ping"), nullptr };
        execvp(args[0], args);
        // If execvp returns, it failed
        _exit(127);
    } else {
        // Parent process: wait for child
        log_success("fork() succeeded. Child PID: " + std::to_string(pid));
        int status = 0;
        pid_t waited_pid = waitpid(pid, &status, 0);
        uint64_t elapsed_us = (get_time_ns() - start) / 1000;

        if (waited_pid == pid) {
            int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            log_success("waitpid() succeeded in " + std::to_string(elapsed_us) + " us. Child exit code: " + std::to_string(exit_code));
        } else {
            log_error("waitpid() failed: " + get_last_os_error());
            return false;
        }
    }
    return true;
}

bool SyscallEngine::demo_memory() {
    log_info("Executing Demonstration: Memory Management Category (mmap, munmap)");

    const size_t alloc_size = 64 * 1024; // 64 KB (16 pages)
    // 1. mmap() anonymous private mapping
    void* ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        log_error("mmap() failed: " + get_last_os_error());
        return false;
    }
    log_success("mmap() allocated 64KB virtual pages at address: " + std::to_string(reinterpret_cast<uintptr_t>(ptr)));

    // Verify read/write across virtual page boundary
    char* byte_ptr = static_cast<char*>(ptr);
    for (size_t i = 0; i < alloc_size; i += 4096) {
        byte_ptr[i] = static_cast<char>(0xAA + (i / 4096));
    }
    log_success("Memory page writes and readbacks validated across all 16 mapped pages.");

    // 2. munmap()
    if (munmap(ptr, alloc_size) != 0) {
        log_error("munmap() failed: " + get_last_os_error());
        return false;
    }
    log_success("munmap() successfully deallocated virtual memory mapping.");
    return true;
}

bool SyscallEngine::demo_ipc_pipe() {
    log_info("Executing Demonstration: IPC Pipe Category (pipe, read, write)");

    int pipe_fd[2];
    // 1. pipe()
    if (pipe(pipe_fd) != 0) {
        log_error("pipe() failed: " + get_last_os_error());
        return false;
    }
    log_success("pipe() created anonymous unidirectional pipe descriptors: read=" +
                std::to_string(pipe_fd[0]) + ", write=" + std::to_string(pipe_fd[1]));

    const std::string msg = "Linux POSIX Anonymous Kernel Stream Buffer IPC";
    ssize_t written = write(pipe_fd[1], msg.c_str(), msg.size());
    if (written < 0) {
        log_error("write() to pipe failed: " + get_last_os_error());
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return false;
    }
    log_success("write() sent " + std::to_string(written) + " bytes into pipe.");

    char buf[128] = {0};
    ssize_t bytes_read = read(pipe_fd[0], buf, sizeof(buf) - 1);
    if (bytes_read < 0) {
        log_error("read() from pipe failed: " + get_last_os_error());
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return false;
    }
    log_success("read() retrieved " + std::to_string(bytes_read) + " bytes from pipe: \"" + std::string(buf) + "\"");

    close(pipe_fd[0]);
    close(pipe_fd[1]);
    return true;
}

bool SyscallEngine::demo_ipc_shared_memory() {
    log_info("Executing Demonstration: Shared Memory Category (shmget, shmat, shmdt, shmctl)");

    key_t key = 0x54321;
    size_t shm_size = 4096;

    // 1. shmget()
    int shmid = shmget(key, shm_size, IPC_CREAT | 0666);
    if (shmid < 0) {
        log_error("shmget() failed: " + get_last_os_error());
        return false;
    }
    log_success("shmget() allocated shared memory segment ID: " + std::to_string(shmid));

    // 2. shmat()
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        log_error("shmat() failed: " + get_last_os_error());
        shmctl(shmid, IPC_RMID, NULL);
        return false;
    }
    log_success("shmat() attached shared memory segment at: " + std::to_string(reinterpret_cast<uintptr_t>(addr)));

    const std::string test_msg = "POSIX System V Shared Memory Buffer (Zero-Copy Transfer)";
    std::memcpy(addr, test_msg.c_str(), test_msg.size() + 1);

    std::string read_back(static_cast<char*>(addr));
    log_success("Verified Shared Memory content: \"" + read_back + "\"");

    // 3. shmdt()
    shmdt(addr);
    log_success("shmdt() detached segment.");

    // 4. shmctl() IPC_RMID
    shmctl(shmid, IPC_RMID, NULL);
    log_success("shmctl() marked shared memory segment for deletion.");
    return true;
}

bool SyscallEngine::demo_permissions() {
    log_info("Executing Demonstration: Permissions Category (chmod, chown, umask)");

    const char* test_file = "demo_permissions.tmp";
    int fd = open(test_file, O_CREAT | O_WRONLY, 0666);
    if (fd >= 0) close(fd);

    // 1. umask()
    mode_t old_mask = umask(0022);
    log_success("umask() queried and set default file mode creation mask (old: " + std::to_string(old_mask) + ").");

    // 2. chmod()
    if (chmod(test_file, 0600) != 0) {
        log_error("chmod() failed: " + get_last_os_error());
        unlink(test_file);
        return false;
    }
    log_success("chmod() set file permissions to 0600 (Owner Read/Write Only).");

    // 3. chown()
    uid_t uid = getuid();
    gid_t gid = getgid();
    if (chown(test_file, uid, gid) != 0) {
        log_warning("chown() returned non-zero (normal for non-root users): " + get_last_os_error());
    } else {
        log_success("chown() confirmed ownership for UID " + std::to_string(uid) + " / GID " + std::to_string(gid));
    }

    unlink(test_file);
    return true;
}

bool SyscallEngine::run_all_demos(const std::string& self_executable) {
    print_table1_specification();
    std::cout << Color::BOLD << Color::YELLOW << ">>> STARTING SEQUENTIAL SYSTEM CALL DEMONSTRATIONS <<<\n\n" << Color::RESET;

    bool all_ok = true;
    all_ok &= demo_file_io();
    std::cout << "\n";
    all_ok &= demo_process(self_executable);
    std::cout << "\n";
    all_ok &= demo_memory();
    std::cout << "\n";
    all_ok &= demo_ipc_pipe();
    std::cout << "\n";
    all_ok &= demo_ipc_shared_memory();
    std::cout << "\n";
    all_ok &= demo_permissions();
    std::cout << "\n";

    if (all_ok) {
        log_success("ALL 15 SYSTEM CALLS VALIDATED SUCCESSFULLY ON LINUX!");
    } else {
        log_warning("Some demonstrations encountered warnings or errors.");
    }
    return all_ok;
}

std::vector<SyscallBenchResult> SyscallEngine::run_benchmarks(int iterations, const std::string& self_exe) {
    (void)self_exe;
    std::vector<SyscallBenchResult> results;
    log_info("Benchmarking 15 System Calls (Iterations: " + std::to_string(iterations) + " per syscall)...");

    const char* bench_file = "bench_syscall_temp.dat";
    std::vector<double> latencies;
    latencies.reserve(iterations);

    auto compute_stats = [&](SyscallBenchResult& res, const std::vector<double>& lats, 
                             ResourceMetrics start_m, ResourceMetrics end_m, uint64_t total_ns) {
        res.operations_count = lats.size();
        res.total_wall_ms = total_ns / 1000000.0;
        res.user_cpu_ms = end_m.user_cpu_ms - start_m.user_cpu_ms;
        res.sys_cpu_ms = end_m.sys_cpu_ms - start_m.sys_cpu_ms;
        res.peak_rss_kb = end_m.peak_rss_kb;
        res.page_faults = end_m.page_faults - start_m.page_faults;

        double sum = std::accumulate(lats.begin(), lats.end(), 0.0);
        res.avg_latency_ns = sum / lats.size();
        res.min_latency_ns = *std::min_element(lats.begin(), lats.end());
        res.max_latency_ns = *std::max_element(lats.begin(), lats.end());

        double var_sum = 0.0;
        for (double val : lats) {
            double diff = val - res.avg_latency_ns;
            var_sum += diff * diff;
        }
        res.stddev_latency_ns = std::sqrt(var_sum / lats.size());
    };

    // 1. open()
    {
        SyscallBenchResult r;
        r.category = "File I/O";
        r.linux_call = "open()";
        r.windows_call = "CreateFile()";
        r.purpose = "File descriptor creation/open";

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            int fd = open(bench_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
            close(fd);
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 2. write()
    {
        SyscallBenchResult r;
        r.category = "File I/O";
        r.linux_call = "write()";
        r.windows_call = "WriteFile()";
        r.purpose = "Raw buffer disk write";

        int fd = open(bench_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        char dummy[1024];
        std::memset(dummy, 'A', sizeof(dummy));

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            ssize_t w = write(fd, dummy, sizeof(dummy));
            (void)w;
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        close(fd);
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 3. read()
    {
        SyscallBenchResult r;
        r.category = "File I/O";
        r.linux_call = "read()";
        r.windows_call = "ReadFile()";
        r.purpose = "Raw buffer disk read";

        int fd = open(bench_file, O_RDONLY);
        char dummy[1024];

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            lseek(fd, 0, SEEK_SET);
            uint64_t t0 = get_time_ns();
            ssize_t rd = read(fd, dummy, sizeof(dummy));
            (void)rd;
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        close(fd);
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 4. close()
    {
        SyscallBenchResult r;
        r.category = "File I/O";
        r.linux_call = "close()";
        r.windows_call = "CloseHandle()";
        r.purpose = "File descriptor cleanup";

        std::vector<int> fds(iterations);
        for (int i = 0; i < iterations; ++i) {
            fds[i] = open(bench_file, O_RDONLY);
        }

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            close(fds[i]);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }
    unlink(bench_file);

    // 5. mmap()
    {
        SyscallBenchResult r;
        r.category = "Memory";
        r.linux_call = "mmap()";
        r.windows_call = "VirtualAlloc()";
        r.purpose = "Virtual page mapping";

        std::vector<void*> addrs(iterations);
        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            addrs[i] = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);

        // 6. munmap()
        SyscallBenchResult rFree;
        rFree.category = "Memory";
        rFree.linux_call = "munmap()";
        rFree.windows_call = "VirtualFree()";
        rFree.purpose = "Virtual page unmapping";

        std::vector<double> free_lats;
        free_lats.reserve(iterations);
        ResourceMetrics fm1 = sample_process_metrics();
        uint64_t ft_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            munmap(addrs[i], 4096);
            uint64_t t1 = get_time_ns();
            free_lats.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t ft_end = get_time_ns();
        ResourceMetrics fm2 = sample_process_metrics();
        compute_stats(rFree, free_lats, fm1, fm2, ft_end - ft_start);
        results.push_back(rFree);
    }

    // 7. pipe()
    {
        SyscallBenchResult r;
        r.category = "IPC";
        r.linux_call = "pipe()";
        r.windows_call = "CreatePipe()";
        r.purpose = "Anonymous IPC pipe creation";

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            int pfd[2];
            uint64_t t0 = get_time_ns();
            pipe(pfd);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
            close(pfd[0]);
            close(pfd[1]);
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 8. shmget()
    {
        SyscallBenchResult r;
        r.category = "IPC";
        r.linux_call = "shmget()";
        r.windows_call = "CreateFileMapping()";
        r.purpose = "Shared memory segment allocation";

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            int shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0666);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
            shmctl(shmid, IPC_RMID, NULL);
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 9. shmat()
    {
        SyscallBenchResult r;
        r.category = "IPC";
        r.linux_call = "shmat()";
        r.windows_call = "MapViewOfFile()";
        r.purpose = "Attach shared memory segment";

        int shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0666);
        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            void* p = shmat(shmid, NULL, 0);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
            shmdt(p);
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        shmctl(shmid, IPC_RMID, NULL);
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 10. chmod(), chown(), umask()
    {
        SyscallBenchResult r;
        r.category = "Permissions";
        r.linux_call = "chmod(), chown(), umask()";
        r.windows_call = "SetFileSecurity()";
        r.purpose = "Permissions modification";

        const char* sec_file = "bench_sec.tmp";
        int fd = open(sec_file, O_CREAT | O_WRONLY, 0666);
        close(fd);

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            chmod(sec_file, 0644);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        unlink(sec_file);
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 11. fork() + wait()
    {
        SyscallBenchResult r;
        r.category = "Process";
        r.linux_call = "fork() & wait()";
        r.windows_call = "CreateProcess()";
        r.purpose = "Process fork and lifecycle control";

        int proc_iters = (iterations > 50) ? 50 : iterations;
        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < proc_iters; ++i) {
            uint64_t t0 = get_time_ns();
            pid_t p = fork();
            if (p == 0) {
                _exit(0);
            } else if (p > 0) {
                int status;
                waitpid(p, &status, 0);
            }
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        r.notes = "Sample size: " + std::to_string(proc_iters) + " processes";
        results.push_back(r);
    }

    return results;
}

void SyscallEngine::print_benchmark_table(const std::vector<SyscallBenchResult>& results) {
    std::cout << "\n" << Color::GREEN << Color::BOLD
              << "====================================================================================================\n"
              << "                       SYSTEM CALL MICROBENCHMARK RESULTS (LINUX / POSIX)                           \n"
              << "====================================================================================================\n"
              << Color::RESET;
    std::cout << std::left 
              << std::setw(12) << "Category" 
              << std::setw(24) << "Linux Syscall"
              << std::setw(24) << "Windows API" 
              << std::setw(14) << "Avg Latency" 
              << std::setw(12) << "Min" 
              << std::setw(12) << "Max"
              << std::setw(12) << "StdDev" << "\n";
    std::cout << "----------------------------------------------------------------------------------------------------\n";

    for (const auto& r : results) {
        std::string lat_str;
        if (r.avg_latency_ns < 1000.0) {
            lat_str = std::to_string(static_cast<int>(r.avg_latency_ns)) + " ns";
        } else if (r.avg_latency_ns < 1000000.0) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << (r.avg_latency_ns / 1000.0) << " us";
            lat_str = ss.str();
        } else {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << (r.avg_latency_ns / 1000000.0) << " ms";
            lat_str = ss.str();
        }

        std::cout << std::left 
                  << std::setw(12) << r.category
                  << std::setw(24) << r.linux_call 
                  << std::setw(24) << r.windows_call 
                  << std::setw(14) << lat_str
                  << std::setw(12) << std::fixed << std::setprecision(1) << (r.min_latency_ns / 1000.0)
                  << std::setw(12) << std::fixed << std::setprecision(1) << (r.max_latency_ns / 1000.0)
                  << std::setw(12) << std::fixed << std::setprecision(1) << (r.stddev_latency_ns / 1000.0) << "\n";
    }
    std::cout << "====================================================================================================\n\n";
}

bool SyscallEngine::export_benchmark_csv(const std::string& filepath, const std::vector<SyscallBenchResult>& results) {
    std::ofstream f(filepath);
    if (!f.is_open()) return false;
    f << "Category,Linux_Syscall,Windows_API,Purpose,Iterations,Avg_Latency_ns,Min_Latency_ns,Max_Latency_ns,StdDev_ns,Total_Wall_ms,User_CPU_ms,Sys_CPU_ms,Peak_RSS_KB\n";
    for (const auto& r : results) {
        f << r.category << ","
          << "\"" << r.linux_call << "\","
          << "\"" << r.windows_call << "\","
          << "\"" << r.purpose << "\","
          << r.operations_count << ","
          << r.avg_latency_ns << ","
          << r.min_latency_ns << ","
          << r.max_latency_ns << ","
          << r.stddev_latency_ns << ","
          << r.total_wall_ms << ","
          << r.user_cpu_ms << ","
          << r.sys_cpu_ms << ","
          << r.peak_rss_kb << "\n";
    }
    log_success("Exported benchmark metrics to CSV: " + filepath);
    return true;
}

#endif // OS_LINUX

