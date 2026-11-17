#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include "common.h"
#include <vector>

// Sensor data point
struct SensorPoint {
    double timestamp;
    double true_pos;
    double true_vel;
    double measured_pos;
    double estimated_pos;
    double estimated_vel;
};

// Inter-process Kalman Chunk Work Specification
struct KalmanWorkerTask {
    int worker_id;
    size_t start_index;
    size_t end_index;
    double init_pos;
    double init_vel;
    double init_p00;
    double init_p01;
    double init_p11;
    double q_var;
    double r_var;
};

// Worker Completion Status returned via IPC
struct KalmanWorkerResponse {
    int worker_id;
    size_t processed_samples;
    double last_pos;
    double last_vel;
    double computation_time_ms;
    int status_code; // 0 = SUCCESS
};

// 1D/2D Kinematic Kalman Filter Core Model
class KalmanFilter {
private:
    double x_pos; // State: Position
    double x_vel; // State: Velocity
    
    // Covariance Matrix P [2x2]
    double P00, P01;
    double P10, P11;

    // Process noise variance (acceleration variance)
    double q_var;
    // Measurement noise variance
    double r_var;

public:
    KalmanFilter(double init_pos = 0.0, double init_vel = 0.0, 
                 double init_cov = 1.0, double q = 0.1, double r = 0.5);

    void reset(double init_pos, double init_vel, double p00, double p01, double p11, double q, double r);
    
    // Prediction step based on physics model: x = F*x + B*u
    void predict(double dt, double acceleration = 0.0);

    // Measurement update step: K = P*H' / (H*P*H' + R), x = x + K*y, P = (I - K*H)*P
    void update(double measured_pos);

    double get_position() const { return x_pos; }
    double get_velocity() const { return x_vel; }
    void get_covariance(double& p00, double& p01, double& p11) const {
        p00 = P00; p01 = P01; p11 = P11;
    }
};

// Telemetry for Kalman execution breakdown
struct KalmanStageMetrics {
    double io_read_ms = 0.0;
    double compute_ms = 0.0;
    double io_write_ms = 0.0;
    double total_wall_ms = 0.0;
    double user_cpu_ms = 0.0;
    double sys_cpu_ms = 0.0;
    size_t peak_rss_kb = 0;
};

// Orchestrator for Single-Process & Multi-Process Kalman Filtering
class KalmanEngine {
public:
    // Synthetic sensor dataset generator (writes via low-level system call file I/O)
    static bool generate_sensor_dataset(const std::string& filepath, size_t sample_count, 
                                        double dt = 0.01, double noise_std = 1.5);

    // Single-Process filtering using pure system calls
    static bool run_single_process(const std::string& input_file, const std::string& output_file,
                                   KalmanStageMetrics& out_metrics, size_t& sample_count);

    // Multi-Process Parallel filtering using system calls + memory mapping + IPC
    static bool run_multi_process(const std::string& input_file, const std::string& output_file,
                                  int num_workers, const std::string& self_exe,
                                  KalmanStageMetrics& out_metrics, size_t& sample_count);

    // Multi-process worker entrypoint (invoked when binary is called with --worker)
    static int run_worker_process(int pipe_read_fd, int pipe_write_fd, const std::string& shm_name);

    // Comparative evaluation runner (Single vs Multi-Process Speedup & RMSE evaluation)
    static void compare_performance(const std::string& dataset_path, int num_workers, 
                                    const std::string& self_exe);
};

#endif // KALMAN_FILTER_H
