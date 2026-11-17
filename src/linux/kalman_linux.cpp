#if defined(__linux__) || defined(__unix__)

#include "kalman_filter.h"
#include <iostream>
#include <vector>
#include <sstream>

// Helper to parse CSV buffer into SensorPoint vector
static std::vector<SensorPoint> parse_csv_buffer(const char* buffer, size_t size) {
    std::vector<SensorPoint> points;
    points.reserve(size / 40);

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

    // 1. Read input file via open, lseek, read, close
    int fd_in = open(input_file.c_str(), O_RDONLY);
    if (fd_in < 0) {
        log_error("Failed to open input file: " + get_last_os_error());
        return false;
    }

    off_t file_size = lseek(fd_in, 0, SEEK_END);
    lseek(fd_in, 0, SEEK_SET);

    std::vector<char> buffer(file_size + 1, 0);
    ssize_t bytes_read = read(fd_in, buffer.data(), file_size);
    close(fd_in);

    if (bytes_read < 0) {
        log_error("read() failed: " + get_last_os_error());
        return false;
    }

    // 2. Parse sensor measurements
    std::vector<SensorPoint> points = parse_csv_buffer(buffer.data(), bytes_read);
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

    // 5. Write output file via open, write, close
    int fd_out = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        log_error("Failed to create output file: " + get_last_os_error());
        return false;
    }
    ssize_t written = write(fd_out, out_data.data(), out_data.size());
    (void)written;
    close(fd_out);

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

int KalmanEngine::run_worker_process(int pipe_read_fd, int pipe_write_fd, const std::string& shm_name) {
    // Used on Windows; on Linux child runs directly inside fork()
    (void)pipe_read_fd; (void)pipe_write_fd; (void)shm_name;
    return 0;
}

bool KalmanEngine::run_multi_process(const std::string& input_file, const std::string& output_file,
                                     int num_workers, const std::string& self_exe,
                                     KalmanStageMetrics& out_metrics, size_t& sample_count) {
    (void)self_exe;
    if (num_workers <= 0) num_workers = 4;
    ResourceMetrics m1 = sample_process_metrics();
    uint64_t t_start = get_time_ns();

    // 1. Read input file
    int fd_in = open(input_file.c_str(), O_RDONLY);
    if (fd_in < 0) {
        log_error("Failed to open input file: " + get_last_os_error());
        return false;
    }
    off_t file_size = lseek(fd_in, 0, SEEK_END);
    lseek(fd_in, 0, SEEK_SET);
    std::vector<char> buffer(file_size + 1, 0);
    ssize_t bytes_read = read(fd_in, buffer.data(), file_size);
    close(fd_in);

    std::vector<SensorPoint> points = parse_csv_buffer(buffer.data(), bytes_read);
    sample_count = points.size();
    if (points.empty()) {
        log_error("No sensor data parsed.");
        return false;
    }

    uint64_t t_after_read = get_time_ns();
    out_metrics.io_read_ms = (t_after_read - t_start) / 1000000.0;

    // 2. Allocate Memory-Mapped Shared Buffer via mmap(MAP_SHARED | MAP_ANONYMOUS)
    uint64_t t_comp_start = get_time_ns();
    size_t total_shm_size = points.size() * sizeof(SensorPoint);
    void* shared_mem = mmap(NULL, total_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_mem == MAP_FAILED) {
        log_error("mmap() failed for Kalman shared buffer: " + get_last_os_error());
        return false;
    }

    // Copy initial sensor points into shared memory
    std::memcpy(shared_mem, points.data(), total_shm_size);
    SensorPoint* shared_points = static_cast<SensorPoint*>(shared_mem);

    // 3. Partition samples among workers and spawn processes via fork() & pipe()
    size_t chunk_size = points.size() / num_workers;

    struct WorkerPipes {
        pid_t pid;
        int parent_to_child[2];
        int child_to_parent[2];
    };
    std::vector<WorkerPipes> workers(num_workers);

    for (int w = 0; w < num_workers; ++w) {
        if (pipe(workers[w].parent_to_child) < 0 || pipe(workers[w].child_to_parent) < 0) {
            log_error("pipe() failed for worker " + std::to_string(w) + ": " + get_last_os_error());
            munmap(shared_mem, total_shm_size);
            return false;
        }

        pid_t pid = fork();
        if (pid < 0) {
            log_error("fork() failed for worker " + std::to_string(w) + ": " + get_last_os_error());
            munmap(shared_mem, total_shm_size);
            return false;
        }

        if (pid == 0) {
            // === CHILD PROCESS ===
            close(workers[w].parent_to_child[1]); // Close write end of incoming pipe
            close(workers[w].child_to_parent[0]); // Close read end of outgoing pipe

            KalmanWorkerTask task;
            read(workers[w].parent_to_child[0], &task, sizeof(task));
            close(workers[w].parent_to_child[0]);

            uint64_t w_start = get_time_ns();
            KalmanFilter kf;
            kf.reset(task.init_pos, task.init_vel, task.init_p00, task.init_p01, task.init_p11, task.q_var, task.r_var);

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

            KalmanWorkerResponse resp;
            resp.worker_id = task.worker_id;
            resp.processed_samples = task.end_index - task.start_index;
            resp.last_pos = kf.get_position();
            resp.last_vel = kf.get_velocity();
            resp.computation_time_ms = (w_end - w_start) / 1000000.0;
            resp.status_code = 0;

            write(workers[w].child_to_parent[1], &resp, sizeof(resp));
            close(workers[w].child_to_parent[1]);
            _exit(0);
        } else {
            // === PARENT PROCESS ===
            workers[w].pid = pid;
            close(workers[w].parent_to_child[0]); // Close unused read end
            close(workers[w].child_to_parent[1]); // Close unused write end

            // Send task specification to worker
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

            write(workers[w].parent_to_child[1], &task, sizeof(task));
            close(workers[w].parent_to_child[1]);
        }
    }

    // 4. Synchronize via waitpid() & collect metrics from pipes
    for (int w = 0; w < num_workers; ++w) {
        int status = 0;
        waitpid(workers[w].pid, &status, 0);

        KalmanWorkerResponse resp;
        read(workers[w].child_to_parent[0], &resp, sizeof(resp));
        close(workers[w].child_to_parent[0]);
    }
    uint64_t t_comp_end = get_time_ns();
    out_metrics.compute_ms = (t_comp_end - t_comp_start) / 1000000.0;

    // 5. Write filtered data to output file using open, write, close
    uint64_t t_write_start = get_time_ns();
    std::stringstream ss;
    ss << "timestamp,true_pos,true_vel,measured_pos,estimated_pos,estimated_vel\n";
    for (size_t i = 0; i < points.size(); ++i) {
        ss << std::fixed << std::setprecision(5)
           << shared_points[i].timestamp << ","
           << shared_points[i].true_pos << ","
           << shared_points[i].true_vel << ","
           << shared_points[i].measured_pos << ","
           << shared_points[i].estimated_pos << ","
           << shared_points[i].estimated_vel << "\n";
    }
    std::string out_data = ss.str();

    int fd_out = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out >= 0) {
        ssize_t wr = write(fd_out, out_data.data(), out_data.size());
        (void)wr;
        close(fd_out);
    }

    // 6. munmap() shared memory buffer
    munmap(shared_mem, total_shm_size);

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

#endif // OS_LINUX
