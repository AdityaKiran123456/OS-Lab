# OS Lab 2
**Team Members:** 
* Aditya Kiran Mattewada - CS23B002
* Anshu Kumar - CS23B006

## Implementations

### 1. Pipe System Call (SC_Pipe) - Syscall #57
**syscall.h:** `#define SC_Pipe 57` and `int Pipe(int* fd);`

**pipe.h / pipe.cc:** New `Pipe` class with a 256-byte circular buffer, a `Lock` for mutual exclusion, and two semaphores — `notEmpty` (starts at 0, reader blocks here) and `notFull` (starts at 256, writer blocks here).

**kernel.h / kernel.cc:** Added `Pipe* pipeTable[MAX_PIPES]` and `CreatePipe()` to the kernel. Pipe table initialized in `Kernel::Initialize()`.

**ksyscall.h:** Added `SysPipe()` which calls `CreatePipe()`, assigns read end FD = `PIPE_FD_BASE + 2*i` and write end FD = `PIPE_FD_BASE + 2*i + 1`. Modified `SysRead` and `SysWrite` to detect pipe FDs using `IS_PIPE_FD` and dispatch to the correct pipe.

**exception.cc:** `handle_SC_Pipe()` reads user pointer from register 4, calls `SysPipe()`, writes the two FDs back to user memory using `WriteMem`.

**start.S:** Added MIPS stub for `Pipe`.

**Test:** `./nachos -x ../test/pipe_test` prints `Read from pipe: hello from writer!`

---

### 2. Demand Paging
**addrspace.h:** Added `OpenFile* executable`, segment info fields (`codeSize`, `codeVirtAddr`, `codeFileAddr`, `initDataSize`, `initDataVirtAddr`, `initDataFileAddr`), and `bool LoadPage(int vpn)`.

**addrspace.cc:** Constructor sets all page table entries to `valid = FALSE` and `physicalPage = -1` — no physical memory allocated at load time. Executable file kept open for use by `LoadPage`. `LoadPage(vpn)` allocates a physical frame on demand, zeroes it, loads the correct data from the executable if the page overlaps the code or initData segments, then marks the page valid.

**exception.cc:** `PageFaultException` pulled out of the fatal error group. Handler reads the faulting address from `BadVAddrReg`, computes the vpn, calls `LoadPage(vpn)`, and increments `kernel->stats->numPageFaults`.

**Test:** `./nachos -x ../test/halt` shows `Paging: faults 4` confirming pages are loaded on demand. Before demand paging, faults were 0.

---

## Building
```bash
bash build_nachos.sh && bash build_test.sh
```
