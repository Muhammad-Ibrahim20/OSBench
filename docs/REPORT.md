# Complex Computing Problem (CCP) Technical Report
## CT-353 Operating Systems
**Department of Computer Science & Information Technology**

---

### Project Title
**Cross-Platform System Call Analysis, OS Architecture Comparison, and Parallel Kalman Filter Computation on Linux and Windows**

**Course Learning Outcomes (CLOs):**
* **CLO 1 (C2):** Describe the concepts, structure, and design of operating systems.
* **CLO 2 (C4):** Analyze operating system design and its impact on application design and performance.

**Complex Problem Solving Attributes:**
* **CP-2 Depth of Analysis Required:** Conceptual thinking and innovative analysis to formulate abstract models for kernel transitions and parallel execution.
* **CP-8 Interdependence:** High-level problem integrating multiple interdependent subsystems: Kernel I/O, Virtual Memory Paging, IPC Streams, and Process Lifecycle.
* **CP-9 Requirements Identification:** Identification of underlying OS constraints, kernel-mode transitions, IPC scheduling, and synchronization boundaries.

---

## 1. Executive Summary & Problem Description

Operating systems act as the primary abstraction layer between computer hardware and user-space applications. Modern computing demands high-throughput, low-latency execution while ensuring strict memory protection and process isolation. The boundary separating unprivileged user space (Ring 3) from privileged kernel space (Ring 0) is mediated strictly through **System Calls**.

This project investigates, implements, benchmarks, and contrasts **15 fundamental system calls** across two dominant operating system architectures:
1. **Linux (POSIX / Monolithic Kernel Architecture)**
2. **Microsoft Windows (Windows NT Hybrid Kernel Architecture)**

Furthermore, these system calls are integrated into a computationally demanding engineering workload: a **Kinematic Kalman Filter** tracking continuous noisy sensor trajectories (position and velocity). The project implements both a sequential system-call pipeline and a multi-process parallel architecture leveraging memory-mapped virtual buffers and anonymous kernel IPC pipes.

---

## 2. Table 1: The 15 System Calls Specification

| Category | Linux System Call (POSIX) | Windows NT API Equivalent | Purpose & OS Mechanism |
| :--- | :--- | :--- | :--- |
| **File I/O** | `open()` | `CreateFile()` | Allocates file table entry/handle in process control block. |
| **File I/O** | `read()` | `ReadFile()` | Transfers data from VFS/page cache to user buffer via kernel mode. |
| **File I/O** | `write()` | `WriteFile()` | Writes user memory into dirty page cache / storage device driver. |
| **File I/O** | `close()` | `CloseHandle()` | Releases file descriptor / decrements kernel object reference count. |
| **Process** | `fork()` | `CreateProcess()` | Spawns child execution context. (Linux: COW cloning; Windows: new PE image creation). |
| **Process** | `exec()` / `execvp()` | *(Integrated in `CreateProcess`)* | Replaces current process image with new executable binary. |
| **Process** | `wait()` / `waitpid()`| `WaitForSingleObject()` | Blocks parent until child process state transitions to TERMINATED. |
| **Memory** | `mmap()` | `VirtualAlloc()` | Directly manipulates process virtual address descriptors (VAD) / page tables. |
| **Memory** | `munmap()` | `VirtualFree()` | Decommits / invalidates virtual page table entries and TLB. |
| **IPC** | `pipe()` | `CreatePipe()` | Constructs anonymous unidirectional circular kernel stream buffer. |
| **IPC** | `shmget()` | `CreateFileMapping()` | Allocates shared physical memory section backed by paging file. |
| **IPC** | `shmat()` | `MapViewOfFile()` | Maps shared section into process virtual address space. |
| **Permissions** | `chmod()` | `SetFileSecurity()` | Updates file mode permission bits / Discretionary Access Control List (DACL). |
| **Permissions** | `chown()` | *(Security Descriptor SID)* | Modifies security identifier (SID) of file owner. |
| **Permissions** | `umask()` | *(Inherited Security Attributes)* | Defines process-wide default creation permission mask. |

---

## 3. Kernel Transition & Architecture Comparison

### 3.1 Kernel Traps and Context Switching
A system call forces a hardware-level transition across privilege rings:
* **Linux (x86_64):** Invoked via the `syscall` machine instruction (legacy x86: `int 0x80`). The CPU switches from Ring 3 to Ring 0, saves user registers onto the kernel stack, switches `%rsp` to the kernel task stack, and vectors through the System Call Table (`sys_call_table`) via `%rax`. Return is mediated by `sysretq`.
* **Windows NT (x86_64):** Invoked via the `syscall` instruction into `KiSystemCall64`. The processor queries the System Service Descriptor Table (SSDT). Arguments are passed according to Microsoft x64 ABI (`RCX`, `RDX`, `R8`, `R9`, then stack shadow space). Return is mediated via `sysret`.

### 3.2 Process Spawning: `fork()` vs. `CreateProcess()`
* **Linux Copy-On-Write (`fork`):** Linux clones the parent's `task_struct`, file descriptor table, and page tables marked as read-only. Physical memory pages are only duplicated when either process performs a write operation (triggering a minor page fault). This makes `fork()` extremely lightweight (typically 100–300 microseconds).
* **Windows Process Creation (`CreateProcess`):** Windows does not possess a native `fork()` semantic. `CreateProcess` allocates a brand-new Executive Process (`EPROCESS`) block, Process Environment Block (`PEB`), parses the Portable Executable (PE) headers, initializes subsystem DLLs (`ntdll.dll`, `kernel32.dll`), and allocates the initial thread (`ETHREAD`). This results in orders of magnitude higher creation latency (~3,000 microseconds).

---

## 4. Empirical Microbenchmark Results

The 15 system calls were benchmarked under controlled conditions. High-resolution timestamps (`QueryPerformanceCounter` on Windows, `clock_gettime(CLOCK_MONOTONIC)` on Linux) and process telemetry (`GetProcessTimes`, `GetProcessMemoryInfo`, `getrusage`) were collected over 2,000–10,000 iterations per system call.

### 4.1 Windows NT Benchmark Measurements

```
====================================================================================================
                       SYSTEM CALL MICROBENCHMARK RESULTS (WINDOWS NT)                              
====================================================================================================
Category    Linux Syscall           Windows API             Avg Latency   Min         Max         StdDev      
----------------------------------------------------------------------------------------------------
File I/O    open()                  CreateFile()            188.49 us     119.3 us    955.0 us    55.2 us     
File I/O    write()                 WriteFile()             5.95 us       2.1 us      135.8 us    9.3 us      
File I/O    read()                  ReadFile()              1.78 us       1.4 us      26.3 us     0.7 us      
File I/O    close()                 CloseHandle()           10.09 us      6.4 us      259.2 us    6.7 us      
Memory      mmap()                  VirtualAlloc()          982 ns        810 ns      20.5 us     520 ns      
Memory      munmap()                VirtualFree()           1.71 us       1.2 us      23.1 us     810 ns      
IPC         pipe()                  CreatePipe()            26.37 us      18.1 us     254.2 us    13.2 us     
IPC         shmget()                CreateFileMapping()     1.56 us       1.0 us      41.0 us     1.7 us      
IPC         shmat()                 MapViewOfFile()         1.24 us       720 ns      18.8 us     940 ns      
Permissions chmod(), chown(), umask()SetFileSecurity()       81.73 us      67.5 us     1343.9 us   45.7 us     
Process     fork() & exec()         CreateProcess()         3.26 ms       2.52 ms     8.90 ms     1.05 ms     
Process     wait() / waitpid()      WaitForSingleObject()   230 ns        190 ns      3.7 us      210 ns      
====================================================================================================
```

### 4.2 Comparative Latency Analysis (Linux vs Windows)

| Category / Syscall | Linux Typical Latency | Windows NT Latency | Analysis / Architectural Driver |
| :--- | :--- | :--- | :--- |
| **File Open** (`open` vs `CreateFile`) | ~2.5–5.0 μs | ~188.5 μs | Windows security descriptors, I/O Manager filter drivers, and file path canonicalization introduce substantial overhead compared to POSIX inode lookup. |
| **File Read** (`read` vs `ReadFile`) | ~1.2–2.0 μs | ~1.78 μs | Both architectures benefit from system page caching and zero-copy kernel read paths. |
| **Memory Alloc** (`mmap` vs `VirtualAlloc`) | ~800–1200 ns | ~982 ns | Near parity. Both modify Virtual Address Descriptors without immediately committing physical page frames (demand paging). |
| **Process Spawning** (`fork` vs `CreateProcess`) | ~150–350 μs | ~3260 μs | **Windows is ~10-20x slower**. Linux uses Copy-On-Write page tables; Windows constructs full PE subsystem environments. |
| **Wait / Sync** (`waitpid` vs `WaitForSingleObject`)| ~300–600 ns | ~230 ns | Windows Dispatcher Object synchronization is highly optimized in the NT kernel. |
| **IPC Pipe Creation** (`pipe` vs `CreatePipe`) | ~3.0–6.0 μs | ~26.4 μs | Windows sets up two full executive file objects with NamedPipe file system driver involvement. |

---

## 5. Kalman Filter Integration Architecture

### 5.1 Mathematical Foundation
The kinematic Kalman filter tracks 1D/2D state vectors $x = [p, v]^T$ representing physical position and velocity under process acceleration noise $q$:

**Prediction Step (State Extrapolation):**
$$\hat{x}_k^- = F \hat{x}_{k-1} + B u_k = \begin{bmatrix} 1 & \Delta t \\ 0 & 1 \end{bmatrix} \begin{bmatrix} p_{k-1} \\ v_{k-1} \end{bmatrix} + \begin{bmatrix} \frac{\Delta t^2}{2} \\ \Delta t \end{bmatrix} a$$
$$P_k^- = F P_{k-1} F^T + Q$$

**Update Step (Measurement Correction):**
$$y_k = z_k - H \hat{x}_k^-$$
$$S_k = H P_k^- H^T + R$$
$$K_k = P_k^- H^T S_k^{-1}$$
$$\hat{x}_k = \hat{x}_k^- + K_k y_k$$
$$P_k = (I - K_k H) P_k^-$$

### 5.2 System Call Integration Pipeline
The computational workflow strictly replaces high-level runtime streams with native OS system calls:
1. **Sensor Ingestion:** Master process opens raw CSV sensor stream using `open()` / `CreateFile()` and reads buffers with `read()` / `ReadFile()`.
2. **Shared Virtual Memory Mapping:** Allocates a shared buffer across processes via `mmap(MAP_SHARED | MAP_ANONYMOUS)` on Linux or `CreateFileMapping() + MapViewOfFile()` on Windows.
3. **Process Spawning:** Master partitions data into $N$ contiguous slices and spawns $N$ worker processes (`fork()` on Linux, `CreateProcess()` on Windows).
4. **IPC Channel Dispatch:** Master creates anonymous kernel pipes (`pipe()` / `CreatePipe()`) and transmits `KalmanWorkerTask` structs.
5. **Worker Execution:** Workers compute state estimates in parallel, writing results directly into the mapped virtual memory buffer.
6. **Synchronization & State Join:** Workers report metrics via pipe; Master blocks on `waitpid()` / `WaitForSingleObject()`.
7. **Persistence:** Master flushes filtered trajectories using `write()` / `WriteFile()` and releases memory via `munmap()` / `UnmapViewOfFile() + CloseHandle()`.

### 5.3 Comparative Kalman Filter Performance

Tested on a dataset of **200,000 sensor measurements**:

```
---------------- Kalman Filter Comparative Metrics ----------------
Dataset Sample Count:           200,000 points
Parallel Worker Processes:      4
-------------------------------------------------------------------
Single I/O Read Time:           483.68 ms
Single Pure Compute Time:       3.78 ms
Single I/O Write Time:          1055.56 ms
Single Total Wall Time:         1543.03 ms
-------------------------------------------------------------------
Multi I/O Read Time:            441.07 ms
Multi Parallel Compute Time:    45.91 ms
Multi I/O Write Time:           960.69 ms
Multi Total Wall Time:          1447.67 ms
-------------------------------------------------------------------
Pure Compute Speedup:           0.082x (Overhead dominated)
Total End-to-End Speedup:       1.066x
Parallel Compute Efficiency:    2.1%
Single Peak RSS:                48,512 KB
Multi Peak RSS:                 57,368 KB
-------------------------------------------------------------------
```

### 5.4 OS-Level Performance Insights (CP-2 & CP-8 Depth of Analysis)
1. **Amdahl's Law and I/O Bottlenecks:** Over 97% of wall-clock time is spent in text parsing and formatting file I/O operations (`CreateFile`, `WriteFile`). Pure floating-point Kalman arithmetic takes only ~3.78 ms for 200,000 points.
2. **Process Granularity vs. Creation Cost:** Creating 4 processes on Windows takes $\approx 4 \times 3.26\text{ ms} = 13.04\text{ ms}$, plus inter-process pipe messaging overhead. Because the sequential calculation is so fast, the fixed cost of process creation exceeds computational execution time for small tasks.
3. **Architectural Recommendation:** For fine-grained parallel floating-point filtering on Windows, threads (`CreateThread`) or persistent worker pools should be favored over short-lived child processes to bypass PE initialization overhead. On Linux, `fork()` remains feasible for fine-grained tasks due to lightweight Copy-On-Write semantics.

---

## 6. Assessment Rubrics Self-Evaluation

| Rubric Criterion | Target Attribute | Justification of Implementation |
| :--- | :--- | :--- |
| **Criterion 1: Clarity of Objective** | **CP-9 (Requirement Identification)** | Correctly identified all 15 required system calls across 5 distinct OS categories (File I/O, Process, Memory, IPC, Permissions). Verified each call's semantic role in Linux POSIX and Windows NT. |
| **Criterion 2: Innovation** | **CP-2 (Depth of Analysis Required)** | Formulated rigorous abstract models comparing hardware privilege transitions, Copy-On-Write mechanics, SSDT dispatching, and Amdahl's Law limitations in kernel-mediated parallel pipelines. |
| **Criterion 3: Modularity** | **CP-8 (Interdependence)** | Designed cleanly separated modules: platform abstraction headers (`os_syscalls.h`), cross-platform mathematical engine (`kalman_filter.h`), and platform-specific implementations (`src/windows/`, `src/linux/`). |
| **Criterion 4: Validation** | **CP-2 (Depth of Analysis Required)** | Implemented comprehensive validation suites: every system call is functionally validated with runtime assertions, statistical error bounds, and CSV performance export. |
| **Criterion 5: Planning** | **CP-8 (Interdependence)** | Established cross-platform CMake and native Makefile build systems, cross-platform telemetry sampling, and automated shell/PowerShell test harnesses. |

---

## 7. Conclusions & Lessons Learned

1. **System Call Abstraction:** System calls are not simple C functions; they are privilege boundary crossing mechanisms that invalidate hardware pipelines, switch page tables, and incur kernel trap overhead.
2. **POSIX vs. Win32 Paradigm:** POSIX maintains simple, composable, orthogonal abstractions (`fork`, `exec`, `pipe`, `mmap`). Windows Win32 provides rich, declarative, object-oriented security descriptors and executive handles at the expense of higher initial invocation latency.
3. **Application Design Implications:** Application architects must design memory and concurrency strategies around target OS kernels. Workloads demanding frequent process instantiation should rely on POSIX COW architectures or utilize thread pools on Windows NT.

