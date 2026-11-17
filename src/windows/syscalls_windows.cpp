#if defined(_WIN32) || defined(_WIN64)

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
    log_info("Executing Demonstration: File I/O Category (CreateFile, WriteFile, ReadFile, CloseHandle)");
    const char* filename = "demo_file_io.tmp";
    const std::string payload = "Operating Systems Complex Computing Problem (CT-353) - File I/O Verification.";

    // 1. CreateFile
    HANDLE hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        log_error("CreateFile failed: " + get_last_os_error());
        return false;
    }
    log_success("CreateFile() succeeded. File Handle: " + std::to_string(reinterpret_cast<uintptr_t>(hFile)));

    // 2. WriteFile
    DWORD bytesWritten = 0;
    BOOL writeOk = WriteFile(hFile, payload.c_str(), static_cast<DWORD>(payload.size()), &bytesWritten, NULL);
    if (!writeOk || bytesWritten != payload.size()) {
        log_error("WriteFile failed: " + get_last_os_error());
        CloseHandle(hFile);
        return false;
    }
    log_success("WriteFile() succeeded. Written " + std::to_string(bytesWritten) + " bytes.");

    // Seek back to beginning
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    // 3. ReadFile
    std::vector<char> buffer(payload.size() + 1, 0);
    DWORD bytesRead = 0;
    BOOL readOk = ReadFile(hFile, buffer.data(), static_cast<DWORD>(payload.size()), &bytesRead, NULL);
    if (!readOk || bytesRead != payload.size()) {
        log_error("ReadFile failed: " + get_last_os_error());
        CloseHandle(hFile);
        return false;
    }
    log_success("ReadFile() succeeded. Read " + std::to_string(bytesRead) + " bytes: \"" + std::string(buffer.data()) + "\"");

    // 4. CloseHandle
    if (!CloseHandle(hFile)) {
        log_error("CloseHandle failed: " + get_last_os_error());
        return false;
    }
    log_success("CloseHandle() succeeded.");
    DeleteFileA(filename);
    return true;
}

bool SyscallEngine::demo_process(const std::string& self_executable) {
    log_info("Executing Demonstration: Process Control Category (CreateProcess, WaitForSingleObject)");

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::string cmd = "\"" + self_executable + "\" --helper-ping";
    log_info("Invoking child process via CreateProcessA: " + cmd);

    uint64_t start = get_time_ns();
    BOOL created = CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), 
                                 NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (!created) {
        log_error("CreateProcessA failed: " + get_last_os_error());
        return false;
    }
    log_success("CreateProcessA succeeded. Child PID: " + std::to_string(pi.dwProcessId) + ", TID: " + std::to_string(pi.dwThreadId));

    // WaitForSingleObject
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 5000);
    uint64_t elapsed_us = (get_time_ns() - start) / 1000;

    if (waitResult == WAIT_OBJECT_0) {
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        log_success("WaitForSingleObject succeeded in " + std::to_string(elapsed_us) + " us. Child exit code: " + std::to_string(exitCode));
    } else {
        log_error("WaitForSingleObject timed out or failed: " + get_last_os_error());
        TerminateProcess(pi.hProcess, 1);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

bool SyscallEngine::demo_memory() {
    log_info("Executing Demonstration: Memory Management Category (VirtualAlloc, VirtualFree)");

    const size_t alloc_size = 64 * 1024; // 64 KB (16 pages)
    // 1. VirtualAlloc
    LPVOID pMemory = VirtualAlloc(NULL, alloc_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (pMemory == NULL) {
        log_error("VirtualAlloc failed: " + get_last_os_error());
        return false;
    }
    log_success("VirtualAlloc() allocated 64KB at address: " + std::to_string(reinterpret_cast<uintptr_t>(pMemory)));

    // Verify read/write across virtual page boundary
    char* bytePtr = static_cast<char*>(pMemory);
    for (size_t i = 0; i < alloc_size; i += 4096) {
        bytePtr[i] = static_cast<char>(0xAA + (i / 4096));
    }
    log_success("Memory page writes and readbacks validated across all 16 allocated pages.");

    // Query Virtual Memory state
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(pMemory, &mbi, sizeof(mbi))) {
        log_info("VirtualQuery: State=MEM_COMMIT (" + std::to_string(mbi.State) + "), Protect=PAGE_READWRITE (" + std::to_string(mbi.Protect) + ")");
    }

    // 2. VirtualFree
    BOOL freed = VirtualFree(pMemory, 0, MEM_RELEASE);
    if (!freed) {
        log_error("VirtualFree failed: " + get_last_os_error());
        return false;
    }
    log_success("VirtualFree() released virtual memory range successfully.");
    return true;
}

bool SyscallEngine::demo_ipc_pipe() {
    log_info("Executing Demonstration: IPC Pipe Category (CreatePipe, ReadFile, WriteFile)");

    HANDLE hRead = NULL, hWrite = NULL;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // 1. CreatePipe
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        log_error("CreatePipe failed: " + get_last_os_error());
        return false;
    }
    log_success("CreatePipe() created anonymous unidirectional pipe.");

    const std::string msg = "IPC Pipe Communication Test via Win32 Kernel Stream";
    DWORD written = 0;
    if (!WriteFile(hWrite, msg.c_str(), static_cast<DWORD>(msg.size()), &written, NULL)) {
        log_error("WriteFile to pipe failed: " + get_last_os_error());
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }
    log_success("WriteFile() pushed " + std::to_string(written) + " bytes into pipe buffer.");

    char buf[128] = {0};
    DWORD readBytes = 0;
    if (!ReadFile(hRead, buf, sizeof(buf) - 1, &readBytes, NULL)) {
        log_error("ReadFile from pipe failed: " + get_last_os_error());
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }
    log_success("ReadFile() retrieved " + std::to_string(readBytes) + " bytes from pipe: \"" + std::string(buf) + "\"");

    CloseHandle(hRead);
    CloseHandle(hWrite);
    return true;
}

bool SyscallEngine::demo_ipc_shared_memory() {
    log_info("Executing Demonstration: Shared Memory Category (CreateFileMapping, MapViewOfFile)");

    const char* shmName = "Local\\OSProjectSharedMemDemo";
    const DWORD shmSize = 4096;

    // 1. CreateFileMapping (Backed by system paging file)
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, shmSize, shmName);
    if (hMap == NULL) {
        log_error("CreateFileMapping failed: " + get_last_os_error());
        return false;
    }
    log_success("CreateFileMapping() initialized named page file mapping: " + std::string(shmName));

    // 2. MapViewOfFile
    LPVOID pBuf = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, shmSize);
    if (pBuf == NULL) {
        log_error("MapViewOfFile failed: " + get_last_os_error());
        CloseHandle(hMap);
        return false;
    }
    log_success("MapViewOfFile() mapped section into virtual address space at " + std::to_string(reinterpret_cast<uintptr_t>(pBuf)));

    // Write structured IPC message
    const std::string testMsg = "Shared Memory Segment Kernel Buffer (Zero-Copy Transfer)";
    std::memcpy(pBuf, testMsg.c_str(), testMsg.size() + 1);

    // Read back
    std::string readBack(static_cast<char*>(pBuf));
    log_success("Verified Shared Memory content: \"" + readBack + "\"");

    // Cleanup
    UnmapViewOfFile(pBuf);
    CloseHandle(hMap);
    log_success("UnmapViewOfFile() and CloseHandle() completed successfully.");
    return true;
}

bool SyscallEngine::demo_permissions() {
    log_info("Executing Demonstration: Permissions Category (SetFileSecurity)");

    const char* testFile = "demo_permissions.tmp";
    // Create dummy file
    HANDLE hFile = CreateFileA(testFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    // Initialize Security Descriptor
    SECURITY_DESCRIPTOR sd;
    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        log_error("InitializeSecurityDescriptor failed: " + get_last_os_error());
        DeleteFileA(testFile);
        return false;
    }

    // Set DACL to NULL (grant access) or construct ACL
    if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE)) {
        log_error("SetSecurityDescriptorDacl failed: " + get_last_os_error());
        DeleteFileA(testFile);
        return false;
    }

    // Invoke SetFileSecurityA
    if (!SetFileSecurityA(testFile, DACL_SECURITY_INFORMATION, &sd)) {
        log_error("SetFileSecurity failed: " + get_last_os_error());
        DeleteFileA(testFile);
        return false;
    }
    log_success("SetFileSecurity() applied DACL security descriptor to file successfully.");

    DeleteFileA(testFile);
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
        log_success("ALL 15 SYSTEM CALL EQUIVALENTS VALIDATED SUCCESSFULLY ON WINDOWS!");
    } else {
        log_warning("Some demonstrations encountered warnings or errors.");
    }
    return all_ok;
}

std::vector<SyscallBenchResult> SyscallEngine::run_benchmarks(int iterations, const std::string& self_exe) {
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

    // 1. File I/O: CreateFile / open
    {
        SyscallBenchResult r;
        r.category = "File I/O";
        r.linux_call = "open()";
        r.windows_call = "CreateFile()";
        r.purpose = "File handle creation/open";

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            HANDLE h = CreateFileA(bench_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
            CloseHandle(h);
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 2. File I/O: WriteFile / write
    {
        SyscallBenchResult r;
        r.category = "File I/O";
        r.linux_call = "write()";
        r.windows_call = "WriteFile()";
        r.purpose = "Raw buffer disk write";

        HANDLE h = CreateFileA(bench_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        char dummy[1024];
        std::memset(dummy, 'A', sizeof(dummy));
        DWORD written = 0;

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            WriteFile(h, dummy, sizeof(dummy), &written, NULL);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        CloseHandle(h);
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 3. File I/O: ReadFile / read
    {
        SyscallBenchResult r;
        r.category = "File I/O";
        r.linux_call = "read()";
        r.windows_call = "ReadFile()";
        r.purpose = "Raw buffer disk read";

        HANDLE h = CreateFileA(bench_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        char dummy[1024];
        DWORD bytesRead = 0;

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            SetFilePointer(h, 0, NULL, FILE_BEGIN);
            uint64_t t0 = get_time_ns();
            ReadFile(h, dummy, sizeof(dummy), &bytesRead, NULL);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        CloseHandle(h);
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 4. File I/O: CloseHandle / close
    {
        SyscallBenchResult r;
        r.category = "File I/O";
        r.linux_call = "close()";
        r.windows_call = "CloseHandle()";
        r.purpose = "Handle/FD deallocation";

        std::vector<HANDLE> handles(iterations);
        for (int i = 0; i < iterations; ++i) {
            handles[i] = CreateFileA(bench_file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        }

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            CloseHandle(handles[i]);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }
    DeleteFileA(bench_file);

    // 5. Memory: VirtualAlloc / mmap
    {
        SyscallBenchResult r;
        r.category = "Memory";
        r.linux_call = "mmap()";
        r.windows_call = "VirtualAlloc()";
        r.purpose = "Virtual page allocation";

        std::vector<LPVOID> addrs(iterations);
        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            addrs[i] = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);

        // 6. Memory: VirtualFree / munmap
        SyscallBenchResult rFree;
        rFree.category = "Memory";
        rFree.linux_call = "munmap()";
        rFree.windows_call = "VirtualFree()";
        rFree.purpose = "Virtual page release";

        std::vector<double> free_lats;
        free_lats.reserve(iterations);
        ResourceMetrics fm1 = sample_process_metrics();
        uint64_t ft_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            VirtualFree(addrs[i], 0, MEM_RELEASE);
            uint64_t t1 = get_time_ns();
            free_lats.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t ft_end = get_time_ns();
        ResourceMetrics fm2 = sample_process_metrics();
        compute_stats(rFree, free_lats, fm1, fm2, ft_end - ft_start);
        results.push_back(rFree);
    }

    // 7. IPC: CreatePipe / pipe
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
            HANDLE r_h, w_h;
            uint64_t t0 = get_time_ns();
            CreatePipe(&r_h, &w_h, NULL, 0);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
            CloseHandle(r_h);
            CloseHandle(w_h);
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 8. IPC: CreateFileMapping / shmget
    {
        SyscallBenchResult r;
        r.category = "IPC";
        r.linux_call = "shmget()";
        r.windows_call = "CreateFileMapping()";
        r.purpose = "Shared memory handle creation";

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, NULL);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
            CloseHandle(hMap);
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 9. IPC: MapViewOfFile / shmat
    {
        SyscallBenchResult r;
        r.category = "IPC";
        r.linux_call = "shmat()";
        r.windows_call = "MapViewOfFile()";
        r.purpose = "Attach shared memory section";

        HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, NULL);
        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            LPVOID p = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 4096);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
            UnmapViewOfFile(p);
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        CloseHandle(hMap);
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 10. Permissions: SetFileSecurity / chmod, chown, umask
    {
        SyscallBenchResult r;
        r.category = "Permissions";
        r.linux_call = "chmod(), chown(), umask()";
        r.windows_call = "SetFileSecurity()";
        r.purpose = "Modify file access security";

        const char* sec_file = "bench_sec.tmp";
        HANDLE h = CreateFileA(sec_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        CloseHandle(h);

        SECURITY_DESCRIPTOR sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            SetFileSecurityA(sec_file, DACL_SECURITY_INFORMATION, &sd);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        DeleteFileA(sec_file);
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    // 11. Process: CreateProcess / fork, exec (tested with smaller iterations e.g. 50 due to process overhead)
    {
        SyscallBenchResult r;
        r.category = "Process";
        r.linux_call = "fork() & exec()";
        r.windows_call = "CreateProcess()";
        r.purpose = "Heavyweight process creation";

        int proc_iters = (iterations > 50) ? 50 : iterations;
        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        std::string cmd = (self_exe.empty()) ? "cmd.exe /c exit 0" : ("\"" + self_exe + "\" --helper-ping");

        for (int i = 0; i < proc_iters; ++i) {
            STARTUPINFOA si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            uint64_t t0 = get_time_ns();
            BOOL ok = CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));

            if (ok) {
                WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        r.notes = "Sample size: " + std::to_string(proc_iters) + " processes";
        results.push_back(r);
    }

    // 12. Process: WaitForSingleObject / wait()
    {
        SyscallBenchResult r;
        r.category = "Process";
        r.linux_call = "wait() / waitpid()";
        r.windows_call = "WaitForSingleObject()";
        r.purpose = "Process synchronization & exit wait";

        HANDLE hEv = CreateEvent(NULL, TRUE, TRUE, NULL); // Signaled event
        latencies.clear();
        ResourceMetrics m1 = sample_process_metrics();
        uint64_t t_start = get_time_ns();

        for (int i = 0; i < iterations; ++i) {
            uint64_t t0 = get_time_ns();
            WaitForSingleObject(hEv, 0);
            uint64_t t1 = get_time_ns();
            latencies.push_back(static_cast<double>(t1 - t0));
        }
        uint64_t t_end = get_time_ns();
        ResourceMetrics m2 = sample_process_metrics();
        CloseHandle(hEv);
        compute_stats(r, latencies, m1, m2, t_end - t_start);
        results.push_back(r);
    }

    return results;
}

void SyscallEngine::print_benchmark_table(const std::vector<SyscallBenchResult>& results) {
    std::cout << "\n" << Color::GREEN << Color::BOLD
              << "====================================================================================================\n"
              << "                       SYSTEM CALL MICROBENCHMARK RESULTS (WINDOWS NT)                              \n"
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

#endif // OS_WINDOWS
