# ==============================================================================
# CT-353 Operating Systems - Complex Computing Problem (CCP)
# Windows Automated Benchmark & Execution Script
# ==============================================================================

Write-Host "====================================================================" -ForegroundColor Cyan
Write-Host "  CT-353 OS Project: Cross-Platform Benchmark Suite (Windows NT)    " -ForegroundColor Cyan
Write-Host "====================================================================" -ForegroundColor Cyan

# 1. Compile project with g++ or check existing binary
if (-not (Test-Path "os_project.exe")) {
    Write-Host "[BUILD] Compiling os_project.exe using g++ (MinGW)..." -ForegroundColor Yellow
    g++ -std=c++17 -Wall -Wextra -O3 -Iinclude src/main.cpp src/benchmark.cpp src/kalman_filter.cpp src/windows/syscalls_windows.cpp src/windows/kalman_windows.cpp -o os_project.exe -lpsapi -ladvapi32
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Compilation failed!" -ForegroundColor Red
        exit 1
    }
    Write-Host "[SUCCESS] Compilation completed successfully." -ForegroundColor Green
}

# 2. Run Table 1 display
Write-Host "`n[STEP 1] Displaying Table 1 Specification..." -ForegroundColor Yellow
.\os_project.exe --table1

# 3. Run all 15 system call demonstrations
Write-Host "`n[STEP 2] Executing 15 System Call Functional Demonstrations..." -ForegroundColor Yellow
.\os_project.exe --demo-all

# 4. Run system call microbenchmarks
Write-Host "`n[STEP 3] Executing System Call Microbenchmarks (5,000 iterations)..." -ForegroundColor Yellow
.\os_project.exe --benchmark-syscalls 5000

# 5. Generate synthetic sensor dataset
Write-Host "`n[STEP 4] Generating Synthetic Sensor Dataset (100,000 samples)..." -ForegroundColor Yellow
.\os_project.exe --generate-data 100000 sensor_trajectory_data.csv

# 6. Run comparative Kalman evaluation
Write-Host "`n[STEP 5] Comparing Single vs Multi-Process Kalman Filter Performance..." -ForegroundColor Yellow
.\os_project.exe --kalman-compare sensor_trajectory_data.csv 4

Write-Host "`n====================================================================" -ForegroundColor Green
Write-Host "  Windows Benchmark Suite Execution Completed! Outputs Generated:   " -ForegroundColor Green
Write-Host "   - benchmark_syscalls_report.csv                                  " -ForegroundColor Green
Write-Host "   - sensor_trajectory_data.csv                                     " -ForegroundColor Green
Write-Host "   - kalman_single_out.csv                                          " -ForegroundColor Green
Write-Host "   - kalman_multi_out.csv                                           " -ForegroundColor Green
Write-Host "====================================================================" -ForegroundColor Green

