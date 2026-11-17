#if defined(_WIN32) || defined(_WIN64)

#include "kalman_filter.h"
#include <iostream>
#include <vector>
#include <sstream>

// Helper to parse CSV buffer into SensorPoint vector
static std::vector<SensorPoint> parse_csv_buffer(const char* buffer, size_t size) {
    std::vector<SensorPoint> points;
    points.reserve(size / 40); // reasonable initial capacity estimation

    const char* ptr = buffer;
    const char* end = buffer + size;

    // Skip header line
    while (ptr < end && *ptr != '\n') ptr++;
    if (ptr < end && *ptr == '\n') ptr++;

    while (ptr < end) {
        if (*ptr == '\r' || *ptr == '\n' || *ptr == '\0') {
            ptr++;
            continue;
        }
        SensorPoint pt = {};
        char* next_ptr = nullptr;
        pt.timestamp = strtod(ptr, &next_ptr);
        if (next_ptr == ptr || *next_ptr != ',') { while (ptr < end && *ptr != '\n') ptr++; if (ptr < end) ptr++; continue; }
        ptr = next_ptr + 1;

        pt.true_pos = strtod(ptr, &next_ptr);
        if (*next_ptr == ',') ptr = next_ptr + 1; else ptr = next_ptr;

        pt.true_vel = strtod(ptr, &next_ptr);
        if (*next_ptr == ',') ptr = next_ptr + 1; else ptr = next_ptr;

        pt.measured_pos = strtod(ptr, &next_ptr);
        while (ptr < end && *ptr != '\n') ptr++;
        if (ptr < end && *ptr == '\n') ptr++;

        points.push_back(pt);
    }
    return points;
}

bool KalmanEngine::run_single_process(const std::string& input_file, const std::string& output_file,
                                      KalmanStageMetrics& out_metrics, size_t& sample_count) {
    ResourceMetrics m1 = sample_process_metrics();
    uint64_t t_start = get_time_ns();

    // 1. Read input file via Win32 CreateFile & ReadFile
    HANDLE hIn = CreateFileA(input_file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hIn == INVALID_HANDLE_VALUE) {
        log_error("Failed to open input file: " + get_last_os_error());
        return false;
    }

    LARGE_INTEGER fileSize;
    GetFileSizeEx(hIn, &fileSize);
    std::vector<char> buffer(static_cast<size_t>(fileSize.QuadPart) + 1, 0);

    DWORD bytesRead = 0;
    if (!ReadFile(hIn, buffer.data(), static_cast<DWORD>(fileSize.QuadPart), &bytesRead, NULL)) {
        log_error("ReadFile failed: " + get_last_os_error());
        CloseHandle(hIn);
        return false;
    }
    CloseHandle(hIn);

    // 2. Parse sensor measurements
    std::vector<SensorPoint> points = parse_csv_buffer(buffer.data(), bytesRead);
    sample_count = points.size();
    if (points.empty()) {
        log_error("No sensor data parsed from input file.");
        return false;
    }

    uint64_t t_after_read = get_time_ns();
    out_metrics.io_read_ms = (t_after_read - t_start) / 1000000.0;

    // 3. Sequential Kalman Filtering
    uint64_t t_comp_start = get_time_ns();
    KalmanFilter kf(points[0].measured_pos, 0.0, 1.0, 0.1, 0.5);
    points[0].estimated_pos = points[0].measured_pos;
    points[0].estimated_vel = 0.0;

    for (size_t i = 1; i < points.size(); ++i) {
        double dt = points[i].timestamp - points[i - 1].timestamp;
        if (dt <= 0.0) dt = 0.01;
        kf.predict(dt);
        kf.update(points[i].measured_pos);
        points[i].estimated_pos = kf.get_position();
        points[i].estimated_vel = kf.get_velocity();
    }
    uint64_t t_comp_end = get_time_ns();
    out_metrics.compute_ms = (t_comp_end - t_comp_start) / 1000000.0;

    // 4. Format Output CSV
    uint64_t t_write_start = get_time_ns();
    std::stringstream ss;
    ss << "timestamp,true_pos,true_vel,measured_pos,estimated_pos,estimated_vel\n";
    for (const auto& pt : points) {
        ss << std::fixed << std::setprecision(5)
           << pt.timestamp << "," << pt.true_pos << "," << pt.true_vel << ","
           << pt.measured_pos << "," << pt.estimated_pos << "," << pt.estimated_vel << "\n";
    }
    std::string out_data = ss.str();

    // 5. Write output file via Win32 CreateFile & WriteFile
    HANDLE hOut = CreateFileA(output_file.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        log_error("Failed to create output file: " + get_last_os_error());
        return false;
    }
    DWORD bytesWritten = 0;
    WriteFile(hOut, out_data.data(), static_cast<DWORD>(out_data.size()), &bytesWritten, NULL);
    CloseHandle(hOut);

    uint64_t t_end = get_time_ns();
    out_metrics.io_write_ms = (t_end - t_write_start) / 1000000.0;
    ResourceMetrics m2 = sample_process_metrics();

    out_metrics.total_wall_ms = (t_end - t_start) / 1000000.0;
    out_metrics.user_cpu_ms = m2.user_cpu_ms - m1.user_cpu_ms;
    out_metrics.sys_cpu_ms  = m2.sys_cpu_ms - m1.sys_cpu_ms;
    out_metrics.peak_rss_kb = m2.peak_rss_kb;

    log_success("Single-process Kalman complete: " + std::to_string(sample_count) + " points in " +
                std::to_string(out_metrics.total_wall_ms) + " ms.");
    return true;
}

// Windows Worker Process Implementation
int KalmanEngine::run_worker_process(int pipe_read_fd, int pipe_write_fd, const std::string& shm_name) {
    HANDLE hRead = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(pipe_read_fd));
    HANDLE hWrite = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(pipe_write_fd));

    // 1. Read Task Specification from Pipe
    KalmanWorkerTask task;
    DWORD bytesRead = 0;
    if (!ReadFile(hRead, &task, sizeof(task), &bytesRead, NULL) || bytesRead != sizeof(task)) {
        return 101;
    }

    uint64_t w_start = get_time_ns();

    // 2. Open Named File Mapping
    HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shm_name.c_str());
    if (hMap == NULL) {
        return 102;
    }

    // 3. Map View of File into Worker Address Space
    LPVOID pBuf = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (pBuf == NULL) {
        CloseHandle(hMap);
        return 103;
    }

    SensorPoint* shared_points = static_cast<SensorPoint*>(pBuf);

    // 4. Initialize local Kalman Filter from task params
    KalmanFilter kf;
    kf.reset(task.init_pos, task.init_vel, task.init_p00, task.init_p01, task.init_p11, task.q_var, task.r_var);

    // 5. Compute Kalman filter on assigned slice
    for (size_t i = task.start_index; i < task.end_index; ++i) {
        double dt = 0.01;
        if (i > task.start_index) {
            dt = shared_points[i].timestamp - shared_points[i - 1].timestamp;
            if (dt <= 0.0) dt = 0.01;
        }
        kf.predict(dt);
        kf.update(shared_points[i].measured_pos);
        shared_points[i].estimated_pos = kf.get_position();
        shared_points[i].estimated_vel = kf.get_velocity();
    }

    uint64_t w_end = get_time_ns();

    // 6. Build response
    KalmanWorkerResponse resp;
    resp.worker_id = task.worker_id;
    resp.processed_samples = task.end_index - task.start_index;
    resp.last_pos = kf.get_position();
    resp.last_vel = kf.get_velocity();
    resp.computation_time_ms = (w_end - w_start) / 1000000.0;
    resp.status_code = 0;

    // Send back response over pipe
    DWORD written = 0;
    WriteFile(hWrite, &resp, sizeof(resp), &written, NULL);

    // Cleanup worker mappings
    UnmapViewOfFile(pBuf);
    CloseHandle(hMap);
    CloseHandle(hRead);
    CloseHandle(hWrite);

    return 0;
}

bool KalmanEngine::run_multi_process(const std::string& input_file, const std::string& output_file,
                                     int num_workers, const std::string& self_exe,
                                     KalmanStageMetrics& out_metrics, size_t& sample_count) {
    if (num_workers <= 0) num_workers = 4;
    ResourceMetrics m1 = sample_process_metrics();
    uint64_t t_start = get_time_ns();

    // 1. Read input file using CreateFile & ReadFile
    HANDLE hIn = CreateFileA(input_file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hIn == INVALID_HANDLE_VALUE) {
        log_error("Failed to open input file: " + get_last_os_error());
        return false;
    }
    LARGE_INTEGER fileSize;
    GetFileSizeEx(hIn, &fileSize);
    std::vector<char> buffer(static_cast<size_t>(fileSize.QuadPart) + 1, 0);
    DWORD bytesRead = 0;
    ReadFile(hIn, buffer.data(), static_cast<DWORD>(fileSize.QuadPart), &bytesRead, NULL);
    CloseHandle(hIn);

    std::vector<SensorPoint> points = parse_csv_buffer(buffer.data(), bytesRead);
    sample_count = points.size();
    if (points.empty()) {
        log_error("No sensor data parsed.");
        return false;
    }

    uint64_t t_after_read = get_time_ns();
    out_metrics.io_read_ms = (t_after_read - t_start) / 1000000.0;

    // 2. Allocate Memory-Mapped Shared Buffer via CreateFileMapping & MapViewOfFile
    uint64_t t_comp_start = get_time_ns();
    std::string shm_name = "Local\\KalmanShm_" + std::to_string(GetCurrentProcessId());
    size_t total_shm_size = points.size() * sizeof(SensorPoint);

    HANDLE hShmMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 
                                        static_cast<DWORD>((total_shm_size >> 32) & 0xFFFFFFFF),
                                        static_cast<DWORD>(total_shm_size & 0xFFFFFFFF),
                                        shm_name.c_str());
    if (hShmMap == NULL) {
        log_error("CreateFileMapping failed for Kalman buffer: " + get_last_os_error());
        return false;
    }

    LPVOID pShm = MapViewOfFile(hShmMap, FILE_MAP_ALL_ACCESS, 0, 0, total_shm_size);
    if (pShm == NULL) {
        log_error("MapViewOfFile failed: " + get_last_os_error());
        CloseHandle(hShmMap);
        return false;
    }

    // Copy initial sensor points into shared memory
    std::memcpy(pShm, points.data(), total_shm_size);

    // 3. Partition samples among workers
    size_t chunk_size = points.size() / num_workers;

    struct WorkerHandles {
        HANDLE hProcess;
        HANDLE hThread;
        HANDLE hParentWrite;
        HANDLE hParentRead;
    };
    std::vector<WorkerHandles> workers(num_workers);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE; // allow child to inherit pipe handles
    sa.lpSecurityDescriptor = NULL;

    for (int w = 0; w < num_workers; ++w) {
        HANDLE hChildRead = NULL, hParentWrite = NULL;
        HANDLE hParentRead = NULL, hChildWrite = NULL;

        // Pipe 1: Parent -> Child
        CreatePipe(&hChildRead, &hParentWrite, &sa, 0);
        SetHandleInformation(hParentWrite, HANDLE_FLAG_INHERIT, 0); // don't let child inherit parent end

        // Pipe 2: Child -> Parent
        CreatePipe(&hParentRead, &hChildWrite, &sa, 0);
        SetHandleInformation(hParentRead, HANDLE_FLAG_INHERIT, 0);

        workers[w].hParentWrite = hParentWrite;
        workers[w].hParentRead = hParentRead;

        // Command line for child worker
        std::stringstream cmd;
        cmd << "\"" << self_exe << "\" --worker "
            << reinterpret_cast<uintptr_t>(hChildRead) << " "
            << reinterpret_cast<uintptr_t>(hChildWrite) << " "
            << shm_name;

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        BOOL ok = CreateProcessA(NULL, const_cast<char*>(cmd.str().c_str()),
                                 NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
        if (!ok) {
            log_error("CreateProcess failed for worker " + std::to_string(w) + ": " + get_last_os_error());
            CloseHandle(hChildRead);
            CloseHandle(hChildWrite);
            continue;
        }

        workers[w].hProcess = pi.hProcess;
        workers[w].hThread = pi.hThread;

        // Close child ends in parent
        CloseHandle(hChildRead);
        CloseHandle(hChildWrite);

        // Prepare Task specification
        KalmanWorkerTask task;
        task.worker_id = w;
        task.start_index = w * chunk_size;
        task.end_index = (w == num_workers - 1) ? points.size() : (w + 1) * chunk_size;
        task.init_pos = points[task.start_index].measured_pos;
        task.init_vel = 0.0;
        task.init_p00 = 1.0;
        task.init_p01 = 0.0;
        task.init_p11 = 1.0;
        task.q_var = 0.1;
        task.r_var = 0.5;

        // Dispatch task over Pipe using WriteFile
        DWORD written = 0;
        WriteFile(workers[w].hParentWrite, &task, sizeof(task), &written, NULL);
    }

    // 4. Synchronize: Wait for all worker processes via WaitForSingleObject
    for (int w = 0; w < num_workers; ++w) {
        if (workers[w].hProcess != NULL) {
            WaitForSingleObject(workers[w].hProcess, INFINITE);

            // Read response from Pipe via ReadFile
            KalmanWorkerResponse resp;
            DWORD bytesR = 0;
            ReadFile(workers[w].hParentRead, &resp, sizeof(resp), &bytesR, NULL);

            CloseHandle(workers[w].hProcess);
            CloseHandle(workers[w].hThread);
            CloseHandle(workers[w].hParentWrite);
            CloseHandle(workers[w].hParentRead);
        }
    }
    uint64_t t_comp_end = get_time_ns();
    out_metrics.compute_ms = (t_comp_end - t_comp_start) / 1000000.0;

    // 5. Read back mapped results and write output file using CreateFile & WriteFile
    uint64_t t_write_start = get_time_ns();
    SensorPoint* final_points = static_cast<SensorPoint*>(pShm);
    std::stringstream ss;
    ss << "timestamp,true_pos,true_vel,measured_pos,estimated_pos,estimated_vel\n";
    for (size_t i = 0; i < points.size(); ++i) {
        ss << std::fixed << std::setprecision(5)
           << final_points[i].timestamp << ","
           << final_points[i].true_pos << ","
           << final_points[i].true_vel << ","
           << final_points[i].measured_pos << ","
           << final_points[i].estimated_pos << ","
           << final_points[i].estimated_vel << "\n";
    }
    std::string out_data = ss.str();

    HANDLE hOut = CreateFileA(output_file.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(hOut, out_data.data(), static_cast<DWORD>(out_data.size()), &written, NULL);
        CloseHandle(hOut);
    }

    // 6. Cleanup Memory Mapping
    UnmapViewOfFile(pShm);
    CloseHandle(hShmMap);

    uint64_t t_end = get_time_ns();
    out_metrics.io_write_ms = (t_end - t_write_start) / 1000000.0;
    ResourceMetrics m2 = sample_process_metrics();

    out_metrics.total_wall_ms = (t_end - t_start) / 1000000.0;
    out_metrics.user_cpu_ms = m2.user_cpu_ms - m1.user_cpu_ms;
    out_metrics.sys_cpu_ms  = m2.sys_cpu_ms - m1.sys_cpu_ms;
    out_metrics.peak_rss_kb = m2.peak_rss_kb;

    log_success("Multi-process Kalman complete: " + std::to_string(sample_count) + " points across " +
                std::to_string(num_workers) + " workers in " + std::to_string(out_metrics.total_wall_ms) + " ms.");
    return true;
}

#endif // OS_WINDOWS
