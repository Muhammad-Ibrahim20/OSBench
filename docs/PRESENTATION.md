# CT-353 Operating Systems - Complex Computing Problem (CCP)
## Presentation Slide Deck Outline (10–12 Slides)

**Department of Computer Science & Information Technology**
**Project:** Cross-Platform System Call Analysis & Parallel Kalman Filter Computation
**Target Time:** 12–15 Minutes

---

### Slide 1: Title & Group Overview
* **Title:** Cross-Platform System Call Performance Analysis & Parallel Kalman Filter Computation
* **Course:** CT-353 Operating Systems (Complex Computing Problem)
* **Team Members:** [Student Names & Roll Numbers]
* **Supervised By:** Department of Computer Science & Information Technology
* **Key Focus:** 15 System Calls (Linux POSIX vs. Windows NT) + Multi-Process Parallel Kalman Filtering

> **Speaker Notes:** "Good morning everyone. Today we are presenting our Complex Computing Problem project for CT-353 Operating Systems. In this project, we designed and implemented a cross-platform system-level engine that benchmarks 15 essential system calls across Linux and Windows and integrates them into a high-throughput parallel Kalman filter."

---

### Slide 2: Problem Statement & Objectives
* **Objective (CLO 1 & CLO 2):**
  - Understand kernel-mode transitions and OS subsystem architecture.
  - Implement and compare 15 core system calls across Linux and Windows.
  - Parallelize an engineering algorithm (Kalman Filter) using OS-level primitives.
* **Attributes Addressed:**
  - **CP-2:** Depth of analysis into kernel traps, memory mapping, and IPC.
  - **CP-8:** Interdependent subsystems (Process Lifecycle, Memory, IPC, VFS).
  - **CP-9:** System constraints and architecture-specific performance boundaries.

> **Speaker Notes:** "Our objective wasn't just to call high-level library functions, but to understand what happens when a CPU transitions from Ring 3 user mode to Ring 0 kernel mode. We selected 15 system calls spanning 5 critical OS categories."

---

### Slide 3: The 15 System Calls Matrix (Table 1)
| Category | Linux System Call | Windows Win32 API | Underlying OS Purpose |
| :--- | :--- | :--- | :--- |
| **File I/O** | `open`, `read`, `write`, `close` | `CreateFile`, `ReadFile`, `WriteFile`, `CloseHandle` | Direct unbuffered kernel file table & handle management |
| **Process** | `fork`, `exec`, `wait` | `CreateProcess`, `WaitForSingleObject` | Address space cloning, image loading, synchronization |
| **Memory** | `mmap`, `munmap` | `VirtualAlloc`, `VirtualFree` | Direct page table allocation, VAD modification |
| **IPC** | `pipe`, `shmget`, `shmat` | `CreatePipe`, `CreateFileMapping`, `MapViewOfFile` | Kernel circular stream buffer & shared paging sections |
| **Permissions** | `chmod`, `chown`, `umask` | `SetFileSecurity` | File mode bitmask & Discretionary Access Control Lists |

> **Speaker Notes:** "Here is the mapping from Table 1. On Linux, system calls adhere to the Unix 'everything is a file' philosophy. On Windows NT, everything is an Executive Object managed through opaque HANDLEs and Security Descriptors."

---

### Slide 4: Architectural Deep-Dive: Kernel Traps & Transitions
* **Hardware Mechanism:**
  - User mode (Ring 3) to Kernel mode (Ring 0).
  - Machine instruction: `syscall` on x86_64 (`sysretq` return).
* **Linux Dispatching:**
  - Vectoring via System Call Table (`sys_call_table`).
  - Registers: `%rax` for call number, `%rdi`, `%rsi`, `%rdx`, `%r10`, `%r8`, `%r9` for parameters.
* **Windows NT Dispatching:**
  - Vectoring via System Service Descriptor Table (SSDT) in `ntoskrnl.exe`.
  - Microsoft x64 ABI (`RCX`, `RDX`, `R8`, `R9`, stack shadow space).
* **Security & Validation:** User-pointer probe checks (`probe_read` / SEH).

> **Speaker Notes:** "When an application issues a system call, the CPU generates a hardware trap. The processor switches to the kernel stack, saves register context, and looks up the handler in either the Linux sys_call_table or Windows SSDT. This boundary crossing incurs a measurable cost."

---

### Slide 5: System Call Microbenchmarks & Latency Profiling
* **Methodology:** 2,000–10,000 iterations per call with sub-microsecond hardware timers.
* **Key Empirical Results (Windows NT):**
  - **Memory:** `VirtualAlloc`: **982 ns** | `MapViewOfFile`: **1.24 μs**
  - **File I/O:** `ReadFile`: **1.78 μs** | `WriteFile`: **5.95 μs** | `CreateFile`: **188.5 μs**
  - **IPC:** `CreatePipe`: **26.4 μs** | `CreateFileMapping`: **1.56 μs**
  - **Process:** `CreateProcess`: **3.26 ms** | `WaitForSingleObject`: **230 ns**
  - **Permissions:** `SetFileSecurity`: **81.7 μs**

> **Speaker Notes:** "Our empirical benchmarks show striking differences. Virtual memory operations like VirtualAlloc and MapViewOfFile complete in around 1 microsecond. However, process creation on Windows takes over 3.2 milliseconds, while handle creation like CreateFile takes 188 microseconds due to security descriptors and filter drivers."

---

### Slide 6: Cross-Platform Comparison: Linux vs. Windows
* **The Process Creation Dichotomy:**
  - **Linux `fork()`:** Fast Copy-On-Write (~150–300 μs). Clones page tables as read-only; physical pages duplicated only on write.
  - **Windows `CreateProcess()`:** Heavyweight (~3,000 μs). Allocates new `EPROCESS`, `PEB`, parses PE image, links DLLs (`ntdll.dll`).
* **Memory Management:**
  - Both platforms excel at demand paging (`mmap` vs `VirtualAlloc`), deferring physical allocation until first page touch.
* **File I/O Overhead:**
  - POSIX `open()` is significantly lighter than Win32 `CreateFile()` due to simpler inode lookup compared to Windows Object Manager parsing and ACL checking.

> **Speaker Notes:** "The most critical architectural finding is the process creation dichotomy. Linux fork() is an order of magnitude faster because of Copy-On-Write page table duplication. Windows treats every process as an isolated subsystem, making CreateProcess much heavier."

---

### Slide 7: Kalman Filter Mathematical Model
* **Application Purpose:** Real-time state estimation of kinematic sensor trajectories (Position $p$ and Velocity $v$).
* **State Space:**
  $$x_k = \begin{bmatrix} p_k \\ v_k \end{bmatrix}, \quad F = \begin{bmatrix} 1 & \Delta t \\ 0 & 1 \end{bmatrix}$$
* **Prediction Equations:**
  $$\hat{x}_k^- = F \hat{x}_{k-1} + B u_k, \quad P_k^- = F P_{k-1} F^T + Q$$
* **Update Equations:**
  $$K_k = P_k^- H^T (H P_k^- H^T + R)^{-1}$$
  $$\hat{x}_k = \hat{x}_k^- + K_k (z_k - H \hat{x}_k^-), \quad P_k = (I - K_k H) P_k^-$$

> **Speaker Notes:** "To stress-test these system calls, we implemented a discrete Kalman filter tracking autonomous vehicle sensor data. The filter computes predictive kinematics and updates error covariances to filter out Gaussian measurement noise."

---

### Slide 8: System-Call-Based Kalman Filter Architecture
* **Step 1: Ingestion:** `open()` / `CreateFile()` and `read()` / `ReadFile()` ingest sensor CSV.
* **Step 2: Shared Virtual Memory:** Allocates memory-mapped region (`mmap` / `CreateFileMapping` + `MapViewOfFile`).
* **Step 3: Process Forking:** Master partitions dataset and spawns $W$ workers (`fork()` / `CreateProcess()`).
* **Step 4: IPC Coordination:** Tasks dispatched across anonymous kernel pipes (`pipe()` / `CreatePipe()`).
* **Step 5: Parallel Computation:** Workers compute Kalman updates directly into the shared virtual memory buffer.
* **Step 6: Join & Flush:** Master blocks on `waitpid()` / `WaitForSingleObject()`, then persists filtered trajectory via `write()` / `WriteFile()`.

> **Speaker Notes:** "Notice our system design: there are zero third-party libraries or standard file streams. Everything is mediated by low-level OS system calls, memory mapping, and IPC pipes."

---

### Slide 9: Kalman Filter Performance & Speedup Analysis
* **Experimental Setup:** 200,000 sensor measurements, 4 parallel worker processes.
* **Stage Timing Breakdown:**
  - **Single-Process:** Read: 483 ms | **Compute: 3.78 ms** | Write: 1055 ms | Total: 1543 ms
  - **Multi-Process (4 Workers):** Read: 441 ms | **Compute: 45.91 ms** | Write: 960 ms | Total: 1447 ms
* **Key Observations:**
  - **Amdahl's Law:** Over 97% of wall-clock time is spent in file I/O operations.
  - **Process Overhead:** Pure 1D scalar compute is so fast (3.78 ms) that spawning 4 Windows processes (~13 ms) and pipe IPC dominates worker compute time.

> **Speaker Notes:** "Here are our empirical results for 200,000 data points. A classic demonstration of Amdahl's Law: because floating-point scalar Kalman math is so efficient on modern CPUs, file I/O is the true bottleneck. Spawning 4 heavyweight processes on Windows introduced fixed startup costs that exceeded the arithmetic compute time."

---

### Slide 10: Modularity & Code Architecture
* **Project Structure:**
  - `include/common.h`: Cross-platform macros, timers, and resource telemetry (`rusage`, `GetProcessTimes`).
  - `include/os_syscalls.h`: Unified abstract API for the 15 system calls.
  - `include/kalman_filter.h`: Kinematic filter model and IPC orchestrator.
  - `src/windows/`: Win32 native implementations.
  - `src/linux/`: POSIX native implementations.
  - Build System: Cross-platform `CMakeLists.txt` and native `Makefile`.

> **Speaker Notes:** "Our codebase emphasizes high modularity and separation of concerns. The platform-specific system call implementations in src/windows and src/linux are decoupled from the mathematical Kalman core, allowing seamless compilation on both Ubuntu Linux and Windows."

---

### Slide 11: Real-World OS Design Takeaways
1. **Choose the Right Primitive:**
   - On Windows, lightweight tasks should utilize thread pools rather than short-lived processes to avoid PE initialization penalties.
   - On Linux, `fork()` remains viable for granular multi-process tasks due to fast Copy-On-Write page mapping.
2. **Memory Mapping Advantage:**
   - Memory-mapped files (`mmap` / `MapViewOfFile`) avoid redundant user-to-kernel memory copies, maximizing IPC bandwidth.
3. **I/O Bound Systems:**
   - File I/O buffering and asynchronous I/O (`io_uring` on Linux, Overlapped I/O on Windows) are essential to prevent disk bottlenecks.

> **Speaker Notes:** "What did we learn? First, application performance is intimately tied to OS design choices. Windows and Linux optimize for different use cases. Understanding these tradeoffs is essential for systems programming and distributed systems."

---

### Slide 12: Conclusion & Q&A
* **Summary of Achievements:**
  - Implemented, validated, and benchmarked all 15 required system calls.
  - Integrated system calls into an end-to-end parallel Kalman filter computation.
  - Documented empirical metrics, architectural drivers, and lessons learned.
* **Questions & Discussion:**
  - Thank you for your time. We welcome any questions!

> **Speaker Notes:** "In conclusion, our project thoroughly fulfilled all CCP requirements, demonstrating both theoretical understanding of OS concepts and practical systems engineering. We are now open to questions."

