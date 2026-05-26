
# 🖥️ EduOS – Operating System Simulator

## Module Code: CS 2104  
## Student Name: [kondwani harawa]  
## Registration Number: [25311351029]  
## Institution: Department of Computer Science & IT  

---

# 1. Project Overview

EduOS is a hybrid **Operating System simulator** developed using **C (low-level systems programming)** and **Python (high-level scheduling & visualization)**.

It simulates core operating system concepts including:
- Process management (PCB lifecycle)
- CPU scheduling algorithms
- Threading and race conditions
- Inter-process communication (IPC)
- System call simulation
- OS-level integration between kernel-like and user-space components

The goal of this project is to demonstrate how real operating systems manage processes, threads, and scheduling decisions internally.

# 2. System Requirements

# Required Software:
- GCC Compiler (C11 standard)
- Python 3.8+
- pip (Python package manager)
- Git
#  3. Python Dependencies
The Python scheduler and visualization components require the following libraries:
 # Required Libraries:
- matplotlib (for Gantt charts and visualisation)
- pandas (for process data handling)
- numpy (for numerical computations)
- tabulate (for formatted output tables)
  
  #  Scheduling Algorithms
  
- FCFS: executes by arrival order
- SJF: shortest burst first
- Priority: higher priority executes first
- Round Robin: uses time slices for fairness

# Thread Synchronization

- Demonstrated race conditions
- Used mutex locks for thread safety

#  IPC

- Shared memory using `shm_open()` and `mmap()`
- Pipes using `pipe()` and `fork()`

#  System Architecture

- C Core → process & thread management
- Python Layer → scheduling & charts
- Controller → connects C and Python

# Complexity

| Algorithm | Complexity |
|-----------|------------|
| FCFS | O(n log n) |
| SJF | O(n²) |
| Priority | O(n²) |
| RR | O(n × q) |

# for Future Improvements

- Multi-core simulation
- Virtual memory
- GUI dashboard
- File system simulation
  
# Testing
Tested:
- scheduling correctness
- thread synchronization
- IPC communication
- controller integration

#  Installation
Install all dependencies using:
bash
pip install -r python_schedu
---

heduler/requirements.txt
