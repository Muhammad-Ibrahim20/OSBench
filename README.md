# CT-353 Operating Systems: Complex Computing Problem (CCP)

## Cross-Platform System Call Analysis & Parallel Kalman Filter Computation (Linux & Windows)

**Department of Computer Science & Information Technology**  
**Course:** CT-353 Operating Systems  
**Course Learning Outcomes:**
* **CLO 1 (C2):** Describe the concepts, structure, and design of operating systems.
* **CLO 2 (C4):** Analyze operating system design and its impact on application design and performance.

---

## 📌 Project Overview

This project is a high-performance, cross-platform CLI systems engineering suite implemented in modern C++ (C++17). It explores the mechanics of hardware-level privilege transitions (User Mode Ring 3 $\leftrightarrow$ Kernel Mode Ring 0) by implementing, benchmarking, and comparing **15 core system calls** on both **Linux (POSIX)** and **Microsoft Windows (Win32 / Windows NT Kernel)**.

Furthermore, these system calls are integrated into a computationally demanding engineering application: a **Kinematic Kalman Filter** tracking continuous noisy sensor trajectories (position and velocity) via:
1. Low-level unbuffered **File I/O system calls**
2. Shared virtual **Memory Mapping** (`mmap` / `MapViewOfFile`)
3. Multi-process **Parallel Execution** (`fork` / `CreateProcess`)
4. Inter-Process Communication via **Anonymous Kernel Pipes** (`pipe` / `CreatePipe`)

---

## 📋 Table 1: 15 System Calls Specification Matrix

| Category | Linux System Call (POSIX) | Windows NT API Equivalent | Purpose |
| :--- | :--- | :--- | :--- |
| **File I/O** | `open()`, `read()`, `write()`, `close()` | `CreateFile()`, `ReadFile()`, `WriteFile()`, `CloseHandle()` | Direct kernel file descriptor / handle operations |
| **Process** | `fork()`, `exec()`, `wait()` / `waitpid()` | `CreateProcess()`, `WaitForSingleObject()` | Process creation, address space allocation, lifecycle control |
| **Memory** | `mmap()`, `munmap()` | `VirtualAlloc()`, `VirtualFree()` | Direct virtual address space page allocation and unmapping |
| **IPC** | `pipe()`, `shmget()`, `shmat()` | `CreatePipe()`, `CreateFileMapping()`, `MapViewOfFile()` | Circular kernel stream buffers & shared memory sections |
| **Permissions** | `chmod()`, `chown()`, `umask()` | `SetFileSecurity()` | Discretionary Access Control Lists (DACL) & file permission masks |

---

## 📂 Project Directory Structure

```
OSProject/
├── CMakeLists.txt              # Cross-platform CMake build configuration (Linux + Windows)
├── Makefile                    # Native Makefile for Linux (make) and Windows (mingw32-make)
├── README.md                   # Complete documentation and quick-start guide
├── include/
│   ├── common.h               # Cross-platform types, high-res timers, CPU & memory telemetry
│   ├── os_syscalls.h          # 15 System Calls abstract interface & benchmark structs
│   ├── kalman_filter.h        # Kalman filter math model & multi-process IPC orchestrator
│   └── benchmark.h            # Interactive CLI menus and benchmark runner declarations
├── src/
│   ├── main.cpp               # CLI entrypoint with interactive menu and command-line flags
│   ├── kalman_filter.cpp      # Discrete Kalman filter prediction and correction logic
│   ├── benchmark.cpp          # Automated benchmark test suite execution
│   ├── linux/
│   │   ├── syscalls_linux.cpp # Native Linux implementations of all 15 system calls
│   │   └── kalman_linux.cpp   # Linux parallel Kalman filter using fork(), mmap(), pipe()
│   └── windows/
│       ├── syscalls_windows.cpp# Native Windows Win32 API implementations of all 15 system calls
│       └── kalman_windows.cpp  # Windows parallel Kalman filter using CreateProcess(), CreatePipe(), MapViewOfFile()
├── data/
│   └── sensor_input.csv       # Sample sensor dataset (10,000 synthetic GPS/IMU readings)
├── docs/
│   ├── REPORT.md              # Formal Academic CCP Technical Report with complete analysis
│   └── PRESENTATION.md        # 10-12 Slide Deck Presentation Outline with speaker notes
└── scripts/
    ├── run_benchmarks.ps1     # Windows automated compilation and benchmark runner
    └── run_benchmarks.sh      # Linux automated compilation and benchmark runner
```

---

## 🛠️ Build & Compilation Guide

### 🔹 On Linux (Ubuntu / Debian / WSL / Fedora)
Make sure `g++` (GCC 9+), `make`, and optionally `cmake` are installed:
```bash
# Option 1: Using Makefile (Quickest)
make

# Option 2: Using CMake
mkdir build && cd build
cmake ..
make

# Option 3: Direct g++ invocation
g++ -std=c++17 -Wall -Wextra -O3 -Iinclude \
    src/main.cpp src/benchmark.cpp src/kalman_filter.cpp \
    src/linux/syscalls_linux.cpp src/linux/kalman_linux.cpp \
    -o os_project -pthread -lrt
```

### 🔹 On Windows (MinGW / MSYS2 / PowerShell)
```powershell
# Option 1: Direct g++ invocation
g++ -std=c++17 -Wall -Wextra -O3 -Iinclude src/main.cpp src/benchmark.cpp src/kalman_filter.cpp src/windows/syscalls_windows.cpp src/windows/kalman_windows.cpp -o os_project.exe -lpsapi -ladvapi32

# Option 2: Using CMake
mkdir build; cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

---

## 🚀 Running the Project

### 1. Interactive CLI Dashboard (Recommended)
Run without any arguments to launch the colorful interactive dashboard:
```bash
# On Linux:
./os_project

# On Windows:
.\os_project.exe
```

Interactive Menu options:
* `[1]` Display Table 1 Specification Matrix
* `[2]` Demonstrate and functionally validate all 15 system calls with detailed logs
* `[3]` Run system call microbenchmarks & latency profiling (exports to CSV)
* `[4]` Generate synthetic sensor trajectory dataset (configurable sample count)
* `[5]` Run single-process system-call-based Kalman filter
* `[6]` Run multi-process parallel Kalman filter (IPC pipes + shared memory)
* `[7]` Run full comparative evaluation (Single vs Multi-Process speedup & Amdahl's Law analysis)
* `[8]` Run complete automated test & benchmark pipeline
* `[0]` Exit

### 2. Command-Line Batch Mode
You can also run specific actions directly via flags:
```bash
# Display Table 1 specification
./os_project --table1

# Validate all 15 system calls
./os_project --demo-all

# Run microbenchmarks (5,000 iterations per syscall)
./os_project --benchmark-syscalls 5000

# Generate 100,000 synthetic sensor readings
./os_project --generate-data 100000 sensor_trajectory_data.csv

# Run comparative single vs 4-worker parallel Kalman filter
./os_project --kalman-compare sensor_trajectory_data.csv 4

# Run complete automated end-to-end suite
./os_project --run-all
```

### 3. One-Click Automation Scripts
* **On Linux:** `chmod +x scripts/run_benchmarks.sh && ./scripts/run_benchmarks.sh`
* **On Windows:** `.\scripts\run_benchmarks.ps1`

---

## 📊 Summary of Empirical Results

### System Call Latencies (Empirical Microbenchmarks on Windows NT):
* **`VirtualAlloc` (Memory):** ~982 ns (sub-microsecond virtual page reservation)
* **`MapViewOfFile` (IPC):** ~1.24 μs
* **`CreateFileMapping` (IPC):** ~1.56 μs
* **`ReadFile` (File I/O):** ~1.78 μs
* **`WriteFile` (File I/O):** ~5.95 μs
* **`CloseHandle` (Handle):** ~10.09 μs
* **`CreatePipe` (IPC):** ~26.37 μs
* **`SetFileSecurity` (Permissions):** ~81.73 μs
* **`CreateFile` (File I/O):** ~188.49 μs (due to DACL security attributes & I/O filter drivers)
* **`WaitForSingleObject` (Sync):** ~230 ns (highly optimized dispatcher object wait)
* **`CreateProcess` (Process):** ~3.26 ms (reflects PE image loading, `ntdll.dll` initialization)

### Kalman Filter Parallel Speedup (200,000 Points):
* **Single-Process Pure Compute:** 3.78 ms
* **Multi-Process Parallel Compute (4 Workers):** 45.91 ms (dominated by fixed ~13 ms Windows `CreateProcess` spawning + pipe IPC overhead)
* **File I/O Bottleneck:** Over 97% of wall time is spent in raw text serialization/file writes (~1000 ms), demonstrating **Amdahl's Law** in OS application design.

---

## 📖 Documentation & Deliverables

1. **Formal Technical Report:** See [docs/REPORT.md](docs/REPORT.md) for full theoretical background, kernel trap mechanics, memory paging, Amdahl's Law analysis, and rubric mapping.
2. **Presentation Slide Deck:** See [docs/PRESENTATION.md](docs/PRESENTATION.md) for the 10–12 slide presentation outline with comprehensive speaker notes.
3. **Benchmark Results CSV:** Exported automatically as `benchmark_syscalls_report.csv`.

---

## 👥 Authors & Academic Honesty
* **Course:** CT-353 Operating Systems
* **Department:** Computer Science & Information Technology
* **Important Note:** All implementations and analyses were developed in accordance with academic integrity guidelines.

