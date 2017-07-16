# Unix-like\_OS\_Kernel
This repo contains five parts as a OS kernel:
1. back_tracer: Used to print function calling stack, for debug use
2. drivers: Timer, keyboard and console drivers implementation based on interrupt gate and trap gate
3. thread_lib: User space pthread-like thread implementation, with concurrency primitives including mutex, conditional variable, semaphore and reader&writer lock
4. kernel:  Mini kernel that supports multiple memory space paging, preemptive multitasking, interactive shell and system calls including fork, exec, wait, vanish, yield, make_runnable, and readline
5. hypervisor: A virtual environment that can host multiple guest kernel by simulating I/O devices, CPU, timer and X86 privileged instructions