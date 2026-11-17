const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const stagingDir = path.join(__dirname, '..', 'build', 'docx_staging');
const outputDocx = path.join(__dirname, '..', 'docs', 'CT353_Operating_Systems_CCP_Report.docx');

// Ensure staging directories exist
fs.mkdirSync(path.join(stagingDir, '_rels'), { recursive: true });
fs.mkdirSync(path.join(stagingDir, 'word', '_rels'), { recursive: true });

// 1. [Content_Types].xml
const contentTypesXml = `<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>`;
fs.writeFileSync(path.join(stagingDir, '[Content_Types].xml'), contentTypesXml);

// 2. _rels/.rels
const relsXml = `<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>`;
fs.writeFileSync(path.join(stagingDir, '_rels', '.rels'), relsXml);

// 3. word/_rels/document.xml.rels
const docRelsXml = `<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>`;
fs.writeFileSync(path.join(stagingDir, 'word', '_rels', 'document.xml.rels'), docRelsXml);

// 4. word/styles.xml
const stylesXml = `<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:docDefaults>
    <w:rPrDefault>
      <w:rPr>
        <w:rFonts w:ascii="Calibri" w:hAnsi="Calibri" w:cs="Calibri"/>
        <w:sz w:val="22"/>
        <w:szCs w:val="22"/>
        <w:color w:val="262626"/>
      </w:rPr>
    </w:rPrDefault>
    <w:pPrDefault>
      <w:pPr>
        <w:spacing w:line="276" w:lineRule="auto" w:after="160"/>
      </w:pPr>
    </w:pPrDefault>
  </w:docDefaults>

  <w:style w:type="paragraph" w:styleId="Title">
    <w:name w:val="Title"/>
    <w:pPr>
      <w:spacing w:before="240" w:after="120"/>
      <w:jc w:val="center"/>
    </w:pPr>
    <w:rPr>
      <w:rFonts w:ascii="Calibri" w:hAnsi="Calibri"/>
      <w:b/>
      <w:color w:val="1F497D"/>
      <w:sz w:val="48"/>
    </w:rPr>
  </w:style>

  <w:style w:type="paragraph" w:styleId="Subtitle">
    <w:name w:val="Subtitle"/>
    <w:pPr>
      <w:spacing w:before="0" w:after="240"/>
      <w:jc w:val="center"/>
    </w:pPr>
    <w:rPr>
      <w:i/>
      <w:color w:val="595959"/>
      <w:sz w:val="26"/>
    </w:rPr>
  </w:style>

  <w:style w:type="paragraph" w:styleId="Heading1">
    <w:name w:val="heading 1"/>
    <w:pPr>
      <w:spacing w:before="360" w:after="120"/>
    </w:pPr>
    <w:rPr>
      <w:rFonts w:ascii="Calibri" w:hAnsi="Calibri"/>
      <w:b/>
      <w:color w:val="1F497D"/>
      <w:sz w:val="32"/>
    </w:rPr>
  </w:style>

  <w:style w:type="paragraph" w:styleId="Heading2">
    <w:name w:val="heading 2"/>
    <w:pPr>
      <w:spacing w:before="240" w:after="80"/>
    </w:pPr>
    <w:rPr>
      <w:rFonts w:ascii="Calibri" w:hAnsi="Calibri"/>
      <w:b/>
      <w:color w:val="365F91"/>
      <w:sz w:val="26"/>
    </w:rPr>
  </w:style>

  <w:style w:type="paragraph" w:styleId="Heading3">
    <w:name w:val="heading 3"/>
    <w:pPr>
      <w:spacing w:before="160" w:after="60"/>
    </w:pPr>
    <w:rPr>
      <w:rFonts w:ascii="Calibri" w:hAnsi="Calibri"/>
      <w:b/>
      <w:color w:val="595959"/>
      <w:sz w:val="22"/>
    </w:rPr>
  </w:style>
</w:styles>`;
fs.writeFileSync(path.join(stagingDir, 'word', 'styles.xml'), stylesXml);

// Helper functions to generate OpenXML constructs
function escapeXml(str) {
  return str.replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&apos;');
}

function p(text, options = {}) {
  const b = options.bold ? '<w:b/>' : '';
  const i = options.italic ? '<w:i/>' : '';
  const sz = options.size ? `<w:sz w:val="${options.size}"/>` : '';
  const color = options.color ? `<w:color w:val="${options.color}"/>` : '';
  const jc = options.align ? `<w:jc w:val="${options.align}"/>` : '';
  const style = options.style ? `<w:pStyle w:val="${options.style}"/>` : '';
  const spaceBefore = options.spaceBefore ? ` w:before="${options.spaceBefore}"` : '';
  const spaceAfter = options.spaceAfter ? ` w:after="${options.spaceAfter}"` : '';
  const spacing = (spaceBefore || spaceAfter) ? `<w:spacing${spaceBefore}${spaceAfter}/>` : '';

  return `<w:p>
    <w:pPr>${style}${jc}${spacing}</w:pPr>
    <w:r>
      <w:rPr>${b}${i}${sz}${color}</w:rPr>
      <w:t xml:space="preserve">${escapeXml(text)}</w:t>
    </w:r>
  </w:p>`;
}

function heading(text, level = 1) {
  return p(text, { style: `Heading${level}` });
}

function callout(title, text) {
  return `<w:tbl>
    <w:tblPr>
      <w:tblW w:w="9200" w:type="dxa"/>
      <w:tblBorders>
        <w:top w:val="single" w:sz="6" w:space="0" w:color="4F81BD"/>
        <w:left w:val="single" w:sz="36" w:space="0" w:color="1F497D"/>
        <w:bottom w:val="single" w:sz="6" w:space="0" w:color="4F81BD"/>
        <w:right w:val="single" w:sz="6" w:space="0" w:color="4F81BD"/>
      </w:tblBorders>
      <w:tblCellMar>
        <w:top w:w="140" w:type="dxa"/>
        <w:left w:w="200" w:type="dxa"/>
        <w:bottom w:w="140" w:type="dxa"/>
        <w:right w:w="200" w:type="dxa"/>
      </w:tblCellMar>
    </w:tblPr>
    <w:tr>
      <w:tc>
        <w:tcPr>
          <w:tcW w:w="9200" w:type="dxa"/>
          <w:shd w:val="clear" w:color="auto" w:fill="F2F5F8"/>
        </w:tcPr>
        ${p(title, { bold: true, color: '1F497D' })}
        ${p(text, { italic: true, color: '333333' })}
      </w:tc>
    </w:tr>
  </w:tbl>${p('', { spaceAfter: 120 })}`;
}

function table(headers, rows) {
  let xml = `<w:tbl>
    <w:tblPr>
      <w:tblW w:w="9300" w:type="dxa"/>
      <w:tblBorders>
        <w:top w:val="single" w:sz="8" w:space="0" w:color="A6A6A6"/>
        <w:left w:val="none"/>
        <w:bottom w:val="single" w:sz="12" w:space="0" w:color="1F497D"/>
        <w:right w:val="none"/>
        <w:insideH w:val="single" w:sz="4" w:space="0" w:color="D9D9D9"/>
        <w:insideV w:val="none"/>
      </w:tblBorders>
      <w:tblCellMar>
        <w:top w:w="120" w:type="dxa"/>
        <w:left w:w="150" w:type="dxa"/>
        <w:bottom w:w="120" w:type="dxa"/>
        <w:right w:w="150" w:type="dxa"/>
      </w:tblCellMar>
    </w:tblPr>`;

  // Header row
  xml += `<w:tr><w:trPr><w:tblHeader/></w:trPr>`;
  for (const h of headers) {
    xml += `<w:tc>
      <w:tcPr>
        <w:shd w:val="clear" w:color="auto" w:fill="1F497D"/>
      </w:tcPr>
      <w:p>
        <w:pPr><w:spacing w:after="0"/></w:pPr>
        <w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>${escapeXml(h)}</w:t></w:r>
      </w:p>
    </w:tc>`;
  }
  xml += `</w:tr>`;

  // Data rows
  let idx = 0;
  for (const row of rows) {
    const bg = (idx % 2 === 1) ? 'F7F9FB' : 'FFFFFF';
    xml += `<w:tr>`;
    for (const cell of row) {
      xml += `<w:tc>
        <w:tcPr>
          <w:shd w:val="clear" w:color="auto" w:fill="${bg}"/>
        </w:tcPr>
        <w:p>
          <w:pPr><w:spacing w:after="0"/></w:pPr>
          <w:r><w:t>${escapeXml(cell)}</w:t></w:r>
        </w:p>
      </w:tc>`;
    }
    xml += `</w:tr>`;
    idx++;
  }

  xml += `</w:tbl>${p('', { spaceAfter: 160 })}`;
  return xml;
}

// Assemble Complete Document XML Content
let docXml = `<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
<w:body>`;

// Title Page Header
docXml += p("NED UNIVERSITY OF ENGINEERING & TECHNOLOGY", { align: "center", bold: true, size: 28, color: "1F497D" });
docXml += p("Department of Computer Science & Information Technology", { align: "center", bold: true, size: 24, color: "595959" });
docXml += p("CT-353 Operating Systems — Complex Computing Problem (CCP)", { align: "center", italic: true, size: 22, color: "7F7F7F" });
docXml += p("", { spaceAfter: 300 });

// Title
docXml += p("Cross-Platform System Call Analysis, Kernel Architecture Comparison, and Parallel Kalman Filter Computation", { style: "Title" });
docXml += p("Comprehensive Empirical Study on Linux (POSIX) and Microsoft Windows (Windows NT)", { style: "Subtitle" });
docXml += p("", { spaceAfter: 200 });

// Metadata Card
docXml += callout("Academic Metadata & Course Learning Outcomes",
  "Course: CT-353 Operating Systems\n" +
  "CLO 1 (C2): Describe the concepts, structure and design of operating systems.\n" +
  "CLO 2 (C4): Analyze operating system design and its impact on application design and performance.\n" +
  "Complex Problem Solving Attributes: CP-2 (Depth of Analysis), CP-8 (Interdependence), CP-9 (Requirements Identification)\n" +
  "Team Members: [Group of 3 Students] | Submission Date: Fall Term 2026");

// Section 1
docXml += heading("1. Executive Summary & Problem Description", 1);
docXml += p("Operating systems represent the quintessential abstraction bridge between computer microarchitecture and user applications. The strict separation between unprivileged User Mode (Ring 3) and privileged Kernel Mode (Ring 0) ensures process isolation, address space protection, and hardware stability. Mediating every single interaction across this security boundary are System Calls.");
docXml += p("In this Complex Computing Problem (CCP), our team designed and implemented an end-to-end, cross-platform systems software suite in modern C++ (C++17). The project explores the core operating system mechanisms across two distinct architectural paradigms: the Linux monolithic POSIX kernel and the Windows NT hybrid executive kernel.");
docXml += p("The project addresses three fundamental deliverables: (1) implementing and functionally testing 15 essential system calls spanning File I/O, Process Lifecycle, Virtual Memory, Inter-Process Communication (IPC), and File Security; (2) empirically benchmarking their microsecond-level invocation latency and resource overhead; and (3) integrating these low-level system calls into a multi-process, memory-mapped Kinematic Kalman Filter for real-time sensor trajectory estimation.");

// Section 2: Table 1
docXml += heading("2. Table 1: The 15 System Calls Specification Matrix", 1);
docXml += p("Table 1 summarizes the 15 system calls evaluated in this project, categorized by functional subsystem, comparing the native POSIX interface on Linux against the Win32 / NT API on Windows.");

const table1Headers = ["Category", "Linux System Call", "Windows API Equivalent", "Operating System Purpose"];
const table1Rows = [
  ["File I/O", "open()", "CreateFile()", "Allocates unbuffered file descriptor / handle in Process Table"],
  ["File I/O", "read()", "ReadFile()", "Transfers disk cache pages into user virtual address space"],
  ["File I/O", "write()", "WriteFile()", "Flushes user buffer to kernel page cache / disk controller"],
  ["File I/O", "close()", "CloseHandle()", "Releases file descriptor / decrements executive reference count"],
  ["Process", "fork()", "CreateProcess()", "Spawns child process (Linux: Copy-On-Write; Windows: PE image load)"],
  ["Process", "exec() / execvp()", "[Integrated in CreateProcess]", "Replaces process memory space with new binary executable"],
  ["Process", "wait() / waitpid()", "WaitForSingleObject()", "Synchronizes parent until child state transitions to TERMINATED"],
  ["Memory", "mmap()", "VirtualAlloc()", "Direct page allocation & Virtual Address Descriptor (VAD) manipulation"],
  ["Memory", "munmap()", "VirtualFree()", "Decommits physical frames and invalidates virtual page tables"],
  ["IPC", "pipe()", "CreatePipe()", "Creates anonymous circular unidirectional kernel stream buffer"],
  ["IPC", "shmget()", "CreateFileMapping()", "Allocates shared physical memory section backed by paging file"],
  ["IPC", "shmat()", "MapViewOfFile()", "Maps shared kernel memory section into process address space"],
  ["Permissions", "chmod()", "SetFileSecurity()", "Modifies file permission bitmask / Discretionary Access Control List (DACL)"],
  ["Permissions", "chown()", "[Security Descriptor SID]", "Alters user ID / Security Identifier (SID) ownership of file"],
  ["Permissions", "umask()", "[Inherited Security Attributes]", "Configures process-wide default creation permission mask"]
];
docXml += table(table1Headers, table1Rows);

// Section 3
docXml += heading("3. Kernel Transition Mechanics & Architecture Analysis", 1);
docXml += heading("3.1 Privilege Rings and Hardware Traps (Ring 3 to Ring 0)", 2);
docXml += p("When an application requires operating system services, it cannot execute privileged CPU instructions directly. Instead, modern x86_64 processors execute the hardware SYSCALL instruction:");
docXml += p("1. CPU Privilege Switch: The processor transitions from Current Privilege Level (CPL) 3 to CPL 0 (Kernel Mode).");
docXml += p("2. Stack Pointer Switch: The CPU switches the stack pointer %rsp from the user stack to the per-thread kernel task stack.");
docXml += p("3. Register Context Preservation: User-space instruction pointer (%rip) and flags (%rflags) are saved into %rcx and %r11.");
docXml += p("4. Vector Dispatching: The CPU vectors to the System Call Handler address previously registered in the Model Specific Register (MSR_LSTAR).");

docXml += heading("3.2 Linux sys_call_table vs. Windows NT SSDT", 2);
docXml += p("In Linux, the kernel inspects the %rax register as an index into the global sys_call_table array. Parameters are passed according to System V AMD64 ABI (%rdi, %rsi, %rdx, %r10, %r8, %r9). Upon completion, SYSRETQ restores user-space execution.");
docXml += p("In Windows NT, the transition reaches KiSystemCall64, which vectors through the System Service Descriptor Table (SSDT). Arguments follow Microsoft x64 ABI (RCX, RDX, R8, R9, plus 32-byte shadow space on the stack). Windows enforces strict Structured Exception Handling (SEH) and Object Manager handle validation.");

docXml += heading("3.3 The Process Spawning Divergence: fork() vs CreateProcess()", 2);
docXml += p("The most radical architectural divergence between Linux and Windows occurs in process creation:");
docXml += p("• Linux fork(): Relies on Copy-On-Write (COW). The kernel duplicates the parent's page tables and marks all pages read-only. Physical memory is NOT copied during fork(). Only when a process writes to a page does the CPU trigger a minor page fault, causing the kernel to allocate a new physical page frame. Hence, fork() executes in just 150 to 300 microseconds.");
docXml += p("• Windows CreateProcess(): Windows lacks a native fork() primitive. CreateProcess creates a new executive process object (EPROCESS), sets up the Process Environment Block (PEB), opens and parses the Portable Executable (PE) headers from disk, maps imports, allocates the primary thread (ETHREAD), and initializes core DLLs (ntdll.dll, kernel32.dll). This heavyweight lifecycle requires over 3,000 microseconds (~10 to 20 times slower than Linux).");

// Section 4
docXml += heading("4. Empirical System Call Microbenchmark Results", 1);
docXml += p("Microbenchmarks were conducted using high-resolution hardware timers (QueryPerformanceCounter on Windows and clock_gettime(CLOCK_MONOTONIC) on Linux) over 2,000 to 10,000 iterations per system call. CPU times and memory working sets were collected via GetProcessTimes/GetProcessMemoryInfo and getrusage.");

const benchHeaders = ["Category", "Linux Syscall", "Windows API", "Avg Latency", "Min", "Max", "StdDev"];
const benchRows = [
  ["File I/O", "open()", "CreateFile()", "188.49 μs", "119.3 μs", "955.0 μs", "55.2 μs"],
  ["File I/O", "write()", "WriteFile()", "5.95 μs", "2.1 μs", "135.8 μs", "9.3 μs"],
  ["File I/O", "read()", "ReadFile()", "1.78 μs", "1.4 μs", "26.3 μs", "0.7 μs"],
  ["File I/O", "close()", "CloseHandle()", "10.09 μs", "6.4 μs", "259.2 μs", "6.7 μs"],
  ["Memory", "mmap()", "VirtualAlloc()", "982 ns", "810 ns", "20.5 μs", "520 ns"],
  ["Memory", "munmap()", "VirtualFree()", "1.71 μs", "1.2 μs", "23.1 μs", "810 ns"],
  ["IPC", "pipe()", "CreatePipe()", "26.37 μs", "18.1 μs", "254.2 μs", "13.2 μs"],
  ["IPC", "shmget()", "CreateFileMapping()", "1.56 μs", "1.0 μs", "41.0 μs", "1.7 μs"],
  ["IPC", "shmat()", "MapViewOfFile()", "1.24 μs", "720 ns", "18.8 μs", "940 ns"],
  ["Permissions", "chmod(), umask()", "SetFileSecurity()", "81.73 μs", "67.5 μs", "1.34 ms", "45.7 μs"],
  ["Process", "fork() & exec()", "CreateProcess()", "3.26 ms", "2.52 ms", "8.90 ms", "1.05 ms"],
  ["Process", "wait() / waitpid()", "WaitForSingleObject()", "230 ns", "190 ns", "3.7 μs", "210 ns"]
];
docXml += table(benchHeaders, benchRows);

// Section 5
docXml += heading("5. Kalman Filter Integration & Parallel Architecture", 1);
docXml += heading("5.1 Mathematical State Formulation", 2);
docXml += p("The discrete linear Kalman Filter estimates the true physical state of a kinematic system tracking position (p) and velocity (v) under process acceleration noise (q) and measurement noise (r).");
docXml += p("State Prediction:\n" +
            "x_hat_k(-) = F * x_hat_{k-1} + B * u_k\n" +
            "P_k(-) = F * P_{k-1} * F^T + Q\n\n" +
            "Measurement Correction:\n" +
            "y_k = z_k - H * x_hat_k(-)\n" +
            "S_k = H * P_k(-) * H^T + R\n" +
            "K_k = P_k(-) * H^T * S_k^(-1)\n" +
            "x_hat_k = x_hat_k(-) + K_k * y_k\n" +
            "P_k = (I - K_k * H) * P_k(-)");

docXml += heading("5.2 Multi-Process System-Call Pipeline", 2);
docXml += p("Rather than using standard C++ iostreams or thread pools, our architecture executes strictly through operating system system calls:");
docXml += p("1. Sensor Ingestion: Master opens sensor CSV using open() / CreateFile() and buffers raw data using read() / ReadFile().");
docXml += p("2. Shared Memory Allocation: Master maps an anonymous shared memory arena via mmap(MAP_SHARED | MAP_ANONYMOUS) on Linux or CreateFileMapping(INVALID_HANDLE_VALUE) + MapViewOfFile() on Windows.");
docXml += p("3. Worker Forking: Master partitions the N dataset points across W worker processes using fork() on Linux or CreateProcess() on Windows.");
docXml += p("4. IPC Pipe Coordination: Work assignments are dispatched across anonymous pipes (pipe() / CreatePipe()). Workers compute the Kalman estimates directly in the shared memory array.");
docXml += p("5. Synchronization & Persistence: Master waits for all worker PIDs/Handles via waitpid() / WaitForSingleObject(), collects status structs from pipes, and persists the filtered trajectory to disk via write() / WriteFile().");

// Section 6
docXml += heading("6. Comparative Performance Evaluation & Amdahl's Law Analysis", 1);
docXml += p("The table below reports the empirical performance breakdown on a dataset of 200,000 sensor measurements comparing single-process execution against a 4-worker multi-process architecture on Windows NT.");

const kfHeaders = ["Metric / Execution Stage", "Single-Process (Pure Syscalls)", "Multi-Process (4 Workers)", "Analysis / Driver"];
const kfRows = [
  ["Dataset Sample Count", "200,000 points", "200,000 points", "GPS / IMU sensor trajectory"],
  ["File I/O Read Time", "483.68 ms", "441.07 ms", "Kernel CreateFile + ReadFile buffer parsing"],
  ["Pure Kalman Compute Time", "3.78 ms", "45.91 ms", "Sequential scalar math vs process spawn + IPC overhead"],
  ["File I/O Write Time", "1055.56 ms", "960.69 ms", "Disk formatting + WriteFile flushing to disk"],
  ["Total Wall Clock Time", "1543.03 ms", "1447.67 ms", "End-to-end execution latency"],
  ["Pure Compute Speedup", "1.000x", "0.082x", "Process creation overhead dominates scalar computation"],
  ["Total End-to-End Speedup", "1.000x", "1.066x", "Overall wall-time speedup achieved"],
  ["Peak Resident Memory (RSS)", "48,512 KB", "57,368 KB", "Includes memory-mapped paging segment overhead"]
];
docXml += table(kfHeaders, kfRows);

docXml += heading("6.1 In-Depth OS Performance Observations (CP-2 & CP-8)", 2);
docXml += p("1. Amdahl's Law and I/O Dominance: Over 97% of the total execution time is spent in raw text serialization and disk I/O (CreateFile, WriteFile). The actual floating-point Kalman filtering arithmetic is computationally lightweight, requiring only 3.78 milliseconds for 200,000 points.");
docXml += p("2. The Process Creation Overhead Penalty: In Windows NT, creating 4 child processes consumes approximately 4 x 3.26 ms = 13.04 ms, plus inter-process pipe communication latency. Because sequential computation is faster than process initialization, short-lived child processes are inefficient for fine-grained scalar math on Windows.");
docXml += p("3. Architectural Recommendation: For CPU-bound tasks with lightweight compute per sample on Windows, multi-threading (CreateThread) or persistent worker pools should be utilized to avoid the heavy PE loader penalty. On Linux, fork() remains highly effective for fine-grained tasks due to lightweight Copy-On-Write cloning.");

// Section 7
docXml += heading("7. Assessment Rubrics Self-Evaluation Matrix", 1);
const rubricHeaders = ["Assessment Criterion", "Target Attribute", "Self-Rating", "Detailed Justification & Implementation Evidence"];
const rubricRows = [
  ["Criterion 1: Clarity of Objective", "CP-9: Requirements Identification", "Satisfactory (2/2)", "Correctly identified all 15 required system calls across 5 functional categories. Modeled kernel-mode privilege transitions, security contexts, and memory barriers."],
  ["Criterion 2: Innovation & Depth", "CP-2: Depth of Analysis Required", "Satisfactory (2/2)", "Formulated conceptual models comparing Linux COW page table duplication vs Windows PE Executive object creation. Applied Amdahl's Law to explain process granularity trade-offs."],
  ["Criterion 3: Modularity & Architecture", "CP-8: Interdependence", "Satisfactory (2/2)", "Engineered clean modular boundaries: cross-platform common types (common.h), unified syscall API (os_syscalls.h), mathematical filter (kalman_filter.h), and platform implementations."],
  ["Criterion 4: Validation & Testing", "CP-2: Depth of Analysis Required", "Satisfactory (2/2)", "Validated all 15 system calls with automated assertion checks, statistical microbenchmarks (min, max, mean, stddev), and CSV telemetry export."],
  ["Criterion 5: Planning & Execution", "CP-8: Interdependence", "Satisfactory (2/2)", "Formulated a robust cross-platform project plan with CMakeLists.txt, native Makefile, automated PowerShell/Bash runners, and comprehensive documentation."]
];
docXml += table(rubricHeaders, rubricRows);

// Section 8
docXml += heading("8. Conclusion & Lessons Learned", 1);
docXml += p("1. System Calls Are Not Simple Functions: A system call involves hardware privilege boundary switching, processor stack swaps, pipeline flushes, and kernel dispatch tables. High-performance software must minimize unnecessary crossings.");
docXml += p("2. Paradigm Comparison: Linux POSIX emphasizes simple, orthogonal abstractions (fork, pipe, mmap), whereas Windows NT provides rich, declarative, object-oriented executive services (DACLs, handles, security descriptors) at the cost of higher invocation latency.");
docXml += p("3. Systems Engineering Takeaway: Application performance is fundamentally dictated by operating system design choices. Systems programmers must tailor their concurrency and I/O models to the underlying OS architecture.");

docXml += `</w:body></w:document>`;
fs.writeFileSync(path.join(stagingDir, 'word', 'document.xml'), docXml);

console.log("Staging OpenXML files completed. Packing into DOCX...");

const src = stagingDir.replace(/\//g, '\\');
const dst = outputDocx.replace(/\//g, '\\');

const psCommand = `powershell -NoProfile -Command "Add-Type -AssemblyName System.IO.Compression.FileSystem; if (Test-Path '${dst}') { Remove-Item '${dst}' -Force }; [System.IO.Compression.ZipFile]::CreateFromDirectory('${src}', '${dst}')"`;

try {
  execSync(psCommand, { stdio: 'inherit' });
  console.log("Successfully generated DOCX report:", outputDocx);
} catch (e) {
  console.error("Failed to generate DOCX:", e);
  process.exit(1);
}
