# OS Lab
**Team Members:** 
* Aditya Kiran Mattewada - CS23B002
* Anshu Kmar - CS23B006

## Implementations

### 1. Absolute Value System Call (SC_Abs) - Syscall #55

**syscall.h:** `#define SC_Abs 55` and `int Abs(int number);`

**exception.cc:**
case SC_Abs reads register 4, computes abs, writes to register 2, increments PC, returns.

**Test:** `./nachos -x ../test/abs_test` prints 5, 3, 0, 100 for -5, 3, 0, -100

---

### 2. Priority Scheduler

**thread.h:** added `int priority`, `setPriority()`, `getPriority()`  

**scheduler.h:** changed readyList to `SortedList<Thread*>`  

**scheduler.cc:** SortedList initialized with comparator `b->getPriority() - a->getPriority()`, ReadyToRun uses `Insert` instead of `Append`

**Test:** `./nachos -P` threads are forked in order 1,10,5 (priorities of the threads) but run in order 10,5,1

---

### 3. Sleep System Call (SC_Sleep) - Syscall #56 

**Unit:** Seconds

**syscall.h:** `#define SC_Sleep 56` and `void Sleep(int seconds);`  

**exception.cc:** converts seconds to ticks (×100), calls `kernel->alarm->WaitUntil(ticks)`  

**alarm.cc:** WaitUntil implemented keeps yielding until enough ticks have passed

**Test:** `./nachos -x ../test/sleep_test` has higher idle ticks than `./nachos -x ../test/no_sleep_test`

## Building
```bash
bash build_nachos.sh && bash build_test.sh
```
