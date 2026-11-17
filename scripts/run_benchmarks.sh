#!/usr/bin/env bash
# ==============================================================================
# CT-353 Operating Systems - Complex Computing Problem (CCP)
# Linux Automated Benchmark & Execution Script
# ==============================================================================

set -e

CYAN='\033[1;36m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
NC='\033[0m'

echo -e "${CYAN}====================================================================${NC}"
echo -e "${CYAN}  CT-353 OS Project: Cross-Platform Benchmark Suite (Linux / POSIX) ${NC}"
echo -e "${CYAN}====================================================================${NC}"

# 1. Compile project with g++
echo -e "\n${YELLOW}[BUILD] Compiling os_project using g++...${NC}"
g++ -std=c++17 -Wall -Wextra -O3 -Iinclude \
    src/main.cpp src/benchmark.cpp src/kalman_filter.cpp \
    src/linux/syscalls_linux.cpp src/linux/kalman_linux.cpp \
    -o os_project -pthread -lrt

echo -e "${GREEN}[SUCCESS] Compilation completed successfully.${NC}"

# 2. Display Table 1
echo -e "\n${YELLOW}[STEP 1] Displaying Table 1 Specification...${NC}"
./os_project --table1

# 3. Run all 15 system call demonstrations
echo -e "\n${YELLOW}[STEP 2] Executing 15 System Call Functional Demonstrations...${NC}"
./os_project --demo-all

# 4. Run system call microbenchmarks
echo -e "\n${YELLOW}[STEP 3] Executing System Call Microbenchmarks (5,000 iterations)...${NC}"
./os_project --benchmark-syscalls 5000

# 5. Generate synthetic sensor dataset
echo -e "\n${YELLOW}[STEP 4] Generating Synthetic Sensor Dataset (100,000 samples)...${NC}"
./os_project --generate-data 100000 sensor_trajectory_data.csv

# 6. Run comparative Kalman evaluation
echo -e "\n${YELLOW}[STEP 5] Comparing Single vs Multi-Process Kalman Filter Performance...${NC}"
./os_project --kalman-compare sensor_trajectory_data.csv 4

echo -e "\n${GREEN}====================================================================${NC}"
echo -e "${GREEN}  Linux Benchmark Suite Execution Completed! Outputs Generated:     ${NC}"
echo -e "${GREEN}   - benchmark_syscalls_report.csv                                  ${NC}"
echo -e "${GREEN}   - sensor_trajectory_data.csv                                     ${NC}"
echo -e "${GREEN}   - kalman_single_out.csv                                          ${NC}"
echo -e "${GREEN}   - kalman_multi_out.csv                                           ${NC}"
echo -e "${GREEN}====================================================================${NC}"

