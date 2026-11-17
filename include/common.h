#ifndef OS_COMMON_H
#define OS_COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <cstdint>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
    #define OS_WINDOWS 1
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <psapi.h>
    #include <aclapi.h>
    #include <sddl.h>
#elif defined(__linux__) || defined(__unix__)
    #define OS_LINUX 1
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/wait.h>
    #include <sys/mman.h>
    #include <sys/ipc.h>
    #include <sys/shm.h>
    #include <sys/resource.h>
    #include <sys/time.h>
    #include <errno.h>
    #include <pwd.h>
    #include <grp.h>
#else
    #error "Unsupported Operating System. Supported OS: Windows and Linux."
#endif

// ANSI color codes for rich CLI presentation
namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string RED     = "\033[1;31m";
    const std::string GREEN   = "\033[1;32m";
    const std::string YELLOW  = "\033[1;33m";
    const std::string BLUE    = "\033[1;34m";
    const std::string MAGENTA = "\033[1;35m";
    const std::string CYAN    = "\033[1;36m";
    const std::string WHITE   = "\033[1;37m";
    const std::string BOLD    = "\033[1m";
}

// Telemetry & Resource Usage Metrics
struct ResourceMetrics {
    double wall_time_ms = 0.0;
    double user_cpu_ms  = 0.0;
    double sys_cpu_ms   = 0.0;
    size_t peak_rss_kb  = 0;
    long page_faults    = 0;
};

// High-resolution clock query
inline uint64_t get_time_ns() {
#if defined(OS_WINDOWS)
    static LARGE_INTEGER freq = {};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<uint64_t>((counter.QuadPart * 1000000000ULL) / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<uint64_t>(ts.tv_nsec);
#endif
}

// Format OS-specific error messages
inline std::string get_last_os_error() {
#if defined(OS_WINDOWS)
    DWORD err = GetLastError();
    if (err == 0) return "No error";
    LPSTR msgBuf = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msgBuf, 0, NULL);
    std::string message(msgBuf, size);
    LocalFree(msgBuf);
    while (!message.empty() && (message.back() == '\r' || message.back() == '\n' || message.back() == ' ')) {
        message.pop_back();
    }
    return "Error " + std::to_string(err) + ": " + message;
#else
    return "Errno " + std::to_string(errno) + ": " + std::string(strerror(errno));
#endif
}

// Loggers
inline void log_info(const std::string& msg) {
    std::cout << Color::CYAN << "[INFO] " << Color::RESET << msg << std::endl;
}

inline void log_success(const std::string& msg) {
    std::cout << Color::GREEN << "[SUCCESS] " << Color::RESET << msg << std::endl;
}

inline void log_warning(const std::string& msg) {
    std::cout << Color::YELLOW << "[WARNING] " << Color::RESET << msg << std::endl;
}

inline void log_error(const std::string& msg) {
    std::cerr << Color::RED << "[ERROR] " << Color::RESET << msg << std::endl;
}

// Query process resource usage snapshot
inline ResourceMetrics sample_process_metrics() {
    ResourceMetrics m;
#if defined(OS_WINDOWS)
    FILETIME creationTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER kt, ut;
        kt.LowPart = kernelTime.dwLowDateTime;
        kt.HighPart = kernelTime.dwHighDateTime;
        ut.LowPart = userTime.dwLowDateTime;
        ut.HighPart = userTime.dwHighDateTime;
        m.sys_cpu_ms = kt.QuadPart / 10000.0;  // 100ns units to ms
        m.user_cpu_ms = ut.QuadPart / 10000.0;
    }
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        m.peak_rss_kb = pmc.PeakWorkingSetSize / 1024;
        m.page_faults = pmc.PageFaultCount;
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        m.user_cpu_ms = usage.ru_utime.tv_sec * 1000.0 + usage.ru_utime.tv_usec / 1000.0;
        m.sys_cpu_ms  = usage.ru_stime.tv_sec * 1000.0 + usage.ru_stime.tv_usec / 1000.0;
        m.peak_rss_kb = usage.ru_maxrss; // in Linux, ru_maxrss is in KB
        m.page_faults = usage.ru_majflt + usage.ru_minflt;
    }
#endif
    return m;
}

#endif // OS_COMMON_H
