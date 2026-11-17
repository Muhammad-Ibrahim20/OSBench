#include "kalman_filter.h"
#include <random>
#include <cmath>
#include <fstream>

KalmanFilter::KalmanFilter(double init_pos, double init_vel, double init_cov, double q, double r)
    : x_pos(init_pos), x_vel(init_vel), 
      P00(init_cov), P01(0.0), P10(0.0), P11(init_cov),
      q_var(q), r_var(r) {}

void KalmanFilter::reset(double init_pos, double init_vel, 
                         double p00, double p01, double p11, 
                         double q, double r) {
    x_pos = init_pos;
    x_vel = init_vel;
    P00 = p00;
    P01 = p01;
    P10 = p01;
    P11 = p11;
    q_var = q;
    r_var = r;
}

void KalmanFilter::predict(double dt, double acceleration) {
    // State extrapolation: x = F*x + B*u
    x_pos += x_vel * dt + 0.5 * acceleration * dt * dt;
    x_vel += acceleration * dt;

    // Covariance extrapolation: P = F*P*F' + Q
    double dt2 = dt * dt;
    double dt3 = dt2 * dt;
    double dt4 = dt2 * dt2;

    double p00_temp = P00 + dt * (P10 + P01) + dt2 * P11 + q_var * (dt4 / 4.0);
    double p01_temp = P01 + dt * P11 + q_var * (dt3 / 2.0);
    double p11_temp = P11 + q_var * dt2;

    P00 = p00_temp;
    P01 = p01_temp;
    P10 = p01_temp;
    P11 = p11_temp;
}

void KalmanFilter::update(double measured_pos) {
    // Measurement residual (innovation): y = z - H*x
    double y = measured_pos - x_pos;

    // Innovation covariance: S = H*P*H' + R
    double S = P00 + r_var;
    if (std::abs(S) < 1e-12) return;

    // Optimal Kalman Gain: K = P*H' * inv(S)
    double K0 = P00 / S;
    double K1 = P10 / S;

    // State update: x = x + K*y
    x_pos += K0 * y;
    x_vel += K1 * y;

    // Covariance update: P = (I - K*H)*P
    double new_p00 = P00 - K0 * P00;
    double new_p01 = P01 - K0 * P01;
    double new_p11 = P11 - K1 * P01;

    P00 = new_p00;
    P01 = new_p01;
    P10 = new_p01;
    P11 = new_p11;
}

bool KalmanEngine::generate_sensor_dataset(const std::string& filepath, size_t sample_count, 
                                           double dt, double noise_std) {
    log_info("Generating synthetic sensor dataset: " + filepath + " (" + std::to_string(sample_count) + " points)...");
    
    std::mt19937_64 rng(42); // Deterministic seed for reproducible evaluation across OS
    std::normal_distribution<double> gaussian(0.0, noise_std);
    
    std::stringstream ss;
    ss << "timestamp,true_pos,true_vel,measured_pos\n";

    double current_time = 0.0;
    double true_pos = 0.0;
    double true_vel = 15.0; // initial velocity 15 m/s

    for (size_t i = 0; i < sample_count; ++i) {
        current_time = i * dt;
        // Continuous trajectory with oscillation and acceleration
        double accel = 1.2 * std::sin(0.1 * current_time) + 0.3 * std::cos(0.02 * current_time);
        true_vel += accel * dt;
        true_pos += true_vel * dt;

        double noise = gaussian(rng);
        double measured_pos = true_pos + noise;

        ss << std::fixed << std::setprecision(5) 
           << current_time << "," 
           << true_pos << "," 
           << true_vel << "," 
           << measured_pos << "\n";
    }

    std::string data = ss.str();

    // Use low-level system call to write file!
#if defined(OS_WINDOWS)
    HANDLE hFile = CreateFileA(filepath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        log_error("CreateFile failed while generating dataset: " + get_last_os_error());
        return false;
    }
    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(hFile, data.data(), static_cast<DWORD>(data.size()), &bytesWritten, NULL);
    CloseHandle(hFile);
    if (!ok || bytesWritten != data.size()) {
        log_error("WriteFile failed: " + get_last_os_error());
        return false;
    }
#else
    int fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        log_error("open() failed while generating dataset: " + get_last_os_error());
        return false;
    }
    ssize_t written = write(fd, data.data(), data.size());
    close(fd);
    if (written < 0 || static_cast<size_t>(written) != data.size()) {
        log_error("write() failed: " + get_last_os_error());
        return false;
    }
#endif

    log_success("Dataset generated successfully (" + std::to_string(data.size() / 1024) + " KB).");
    return true;
}

void KalmanEngine::compare_performance(const std::string& dataset_path, int num_workers, 
                                       const std::string& self_exe) {
    std::cout << "\n" << Color::MAGENTA << Color::BOLD
              << "========================================================================\n"
              << "      KALMAN FILTER PERFORMANCE & PARALLEL SPEEDUP EVALUATION           \n"
              << "========================================================================\n"
              << Color::RESET;

    std::string single_out = "kalman_single_out.csv";
    std::string multi_out = "kalman_multi_out.csv";

    KalmanStageMetrics single_m, multi_m;
    size_t count_single = 0, count_multi = 0;

    log_info("Executing Stage 1: Single-Process System-Call-Based Kalman Filter...");
    bool ok_single = run_single_process(dataset_path, single_out, single_m, count_single);
    if (!ok_single) {
        log_error("Single-process Kalman run failed.");
        return;
    }

    log_info("Executing Stage 2: Multi-Process (" + std::to_string(num_workers) + " workers) System-Call-Based Kalman Filter...");
    bool ok_multi = run_multi_process(dataset_path, multi_out, num_workers, self_exe, multi_m, count_multi);
    if (!ok_multi) {
        log_error("Multi-process Kalman run failed.");
        return;
    }

    double total_speedup = (multi_m.total_wall_ms > 0) ? (single_m.total_wall_ms / multi_m.total_wall_ms) : 1.0;
    double compute_speedup = (multi_m.compute_ms > 0) ? (single_m.compute_ms / multi_m.compute_ms) : 1.0;
    double efficiency = (num_workers > 0) ? (compute_speedup / num_workers) * 100.0 : 0.0;

    std::cout << "\n" << Color::BOLD << "---------------- Kalman Filter Comparative Metrics ----------------" << Color::RESET << "\n";
    std::cout << std::left << std::setw(32) << "Dataset Sample Count:" << count_single << " points\n";
    std::cout << std::left << std::setw(32) << "Parallel Worker Processes:" << num_workers << "\n";
    std::cout << "-------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(32) << "Single I/O Read Time:" << std::fixed << std::setprecision(2) << single_m.io_read_ms << " ms\n";
    std::cout << std::left << std::setw(32) << "Single Pure Compute Time:" << std::fixed << std::setprecision(2) << single_m.compute_ms << " ms\n";
    std::cout << std::left << std::setw(32) << "Single I/O Write Time:" << std::fixed << std::setprecision(2) << single_m.io_write_ms << " ms\n";
    std::cout << std::left << std::setw(32) << "Single Total Wall Time:" << std::fixed << std::setprecision(2) << single_m.total_wall_ms << " ms\n";
    std::cout << "-------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(32) << "Multi I/O Read Time:" << std::fixed << std::setprecision(2) << multi_m.io_read_ms << " ms\n";
    std::cout << std::left << std::setw(32) << "Multi Parallel Compute Time:" << std::fixed << std::setprecision(2) << multi_m.compute_ms << " ms\n";
    std::cout << std::left << std::setw(32) << "Multi I/O Write Time:" << std::fixed << std::setprecision(2) << multi_m.io_write_ms << " ms\n";
    std::cout << std::left << std::setw(32) << "Multi Total Wall Time:" << std::fixed << std::setprecision(2) << multi_m.total_wall_ms << " ms\n";
    std::cout << "-------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(32) << "Pure Compute Speedup:" << Color::GREEN << std::fixed << std::setprecision(3) << compute_speedup << "x" << Color::RESET << "\n";
    std::cout << std::left << std::setw(32) << "Total End-to-End Speedup:" << Color::CYAN << std::fixed << std::setprecision(3) << total_speedup << "x" << Color::RESET << "\n";
    std::cout << std::left << std::setw(32) << "Parallel Compute Efficiency:" << Color::YELLOW << std::fixed << std::setprecision(1) << efficiency << "%" << Color::RESET << "\n";
    std::cout << std::left << std::setw(32) << "Single Peak RSS:" << single_m.peak_rss_kb << " KB\n";
    std::cout << std::left << std::setw(32) << "Multi Peak RSS:" << multi_m.peak_rss_kb << " KB\n";
    std::cout << Color::BOLD << "-------------------------------------------------------------------" << Color::RESET << "\n\n";
}
