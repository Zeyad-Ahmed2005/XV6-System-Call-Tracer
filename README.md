# XV6-System-Call-Tracer

# System Call Tracer (`strace`-like) for xv6

## Overview

This project implements a simplified version of the Linux `strace` utility inside the xv6 operating system.

The tracer monitors and logs system calls made by a process during execution. Whenever a traced process performs a system call, the kernel prints:

* Process ID (`pid`)
* System call name
* Return value
* Duration (`Δt`): approximate wall time from syscall entry to return, printed in **milliseconds** (derived from the `time` CSR using the same scale as the timer in `kernel/trap.c`; blocking syscalls include wait time)

This project demonstrates:

* System call handling
* User mode ↔ kernel mode interaction
* Kernel instrumentation
* Process management in xv6

---

# How to Run This Project

xv6 runs inside QEMU. You need a Linux-like environment (Linux, macOS, or [WSL](https://learn.microsoft.com/en-us/windows/wsl/) on Windows) with the RISC-V toolchain and QEMU installed.

## Prerequisites

1. **RISC-V GNU toolchain** — cross-compiler and binutils for `riscv64-unknown-elf` (or `riscv64-linux-gnu`). Install from the [riscv-gnu-toolchain](https://github.com/riscv/riscv-gnu-toolchain) repo and ensure `riscv64-unknown-elf-gcc` (or equivalent) is on your `PATH`.

2. **QEMU** — build or install QEMU with the `riscv64-softmmu` target so `qemu-system-riscv64` is available.

On Debian/Ubuntu (including WSL), you can often install dependencies with:

```bash
sudo apt-get install git build-essential python3 gdb-multiarch \
  qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

## Build and start xv6

From the project root:

```bash
make qemu
```

This compiles the kernel and user programs (including `trace`), builds the filesystem image, and boots xv6 in QEMU. You should see an `$` shell prompt inside the emulator.

To exit QEMU, press `Ctrl-a` then `x`.

Other useful targets:

| Command       | Description                                      |
| ------------- | ------------------------------------------------ |
| `make qemu`   | Full rebuild of `fs.img` and boot                |
| `make qemu-fs`| Boot using an existing `fs.img` (faster rebuild) |
| `make clean`  | Remove build artifacts                           |

## Run the tracer

At the xv6 `$` prompt, use the `trace` command (see [Example Usage](#example-usage) below):

```bash
trace 32 grep hello README
```

You should see lines like:

```text
pid 4: syscall read -> 512 1 milliseconds
pid 4: syscall write -> 512 0 milliseconds
```

Durations are **approximate**: the kernel converts CSR ticks to ms using `TIME_CSR_UNITS_PER_MS` (10000), matching xv6’s timer step of `1000000` ticks ≈ 100 ms described in `kernel/trap.c`.

## Run automated tests

The lab is configured as `LAB=syscall` in `conf/lab.mk`. To run the grading script:

```bash
make grade
```

This runs `grade-lab-syscall` (requires Python 3). Close any other xv6/QEMU instance first, or `make clean` inside the grade step may fail.

---

# What Is a System Call?

When a user program wants to perform an operation that requires operating system privileges (such as reading files, writing to the screen, creating processes, etc.), it cannot do it directly.

Instead, it requests the kernel through a **system call**.

Example:

```text
User Program → "Kernel, please read this file"
Kernel       → Performs operation
Kernel       → Returns result
```

Examples of system calls:

* `read()`
* `write()`
* `fork()`
* `exec()`
* `open()`

Every interaction between user programs and the operating system happens through system calls.

---

# What Is `trace`?

`trace` is a utility that monitors system calls made by a process.

Whenever the process performs a syscall, the tracer prints:

* which process made it
* which syscall was called
* what value it returned
* how long the syscall took (entry to exit)

This is similar to the real Linux `strace` utility.

Example output:

```text
pid 4: syscall read -> 512 0 milliseconds
pid 4: syscall write -> 512 0 milliseconds
pid 4: syscall exit -> 0 0 milliseconds
```

---

# How It Works Internally

## Execution Flow

```text
User runs: trace 32 grep hello README
                │
                ▼
        trace.c calls trace(32)
        → sets p->tracemask = 32 in kernel
                │
                ▼
        trace.c calls exec("grep", ...)
        → process becomes grep
        → tracemask survives
                │
                ▼
        grep executes and makes syscalls
                │
                ▼
        Every syscall passes through:
            syscall()
        inside kernel/syscall.c
                │
                ▼
        syscall() checks:
        is this syscall enabled in tracemask?
                │
                ▼
        (tracemask >> syscall_number) & 1
                │
            yes │
                ▼
        printf("pid %d: syscall %s -> %d %lu milliseconds\n")
        (CSR delta / TIME_CSR_UNITS_PER_MS)
```

---

# Bitmask-Based Tracing

The tracer uses a **bitmask** to determine which system calls should be logged.

Each bit corresponds to a syscall number.

Example:

```text
Bit 1  → SYS_fork   (1)
Bit 2  → SYS_exit   (2)
Bit 5  → SYS_read   (5)
Bit 16 → SYS_write  (16)
```

Example:

```text
Mask = 32
Binary = 100000
```

This means:

* only bit 5 is enabled
* therefore only `read()` syscalls are traced

---

# Implementation Design

## Kernel Modifications

### 1. Added Tracing Field to `struct proc`

Inside:

```text
kernel/proc.h
```

Added:

```c
int tracemask;
```

This stores which syscalls should be traced for each process.

---

### 2. Added `trace()` System Call

A new syscall was implemented:

```c
int trace(int mask);
```

Purpose:

* enables tracing for selected syscalls
* stores mask inside current process

Implementation:

```c
myproc()->tracemask = mask;
```

---

### 3. Modified Syscall Dispatcher

Inside:

```text
kernel/syscall.c
```

The main syscall dispatcher was modified.

Every syscall in xv6 passes through:

```c
void syscall(void)
```

After executing a syscall, the kernel checks:

```c
if ((p->tracemask >> num) & 1)
```

If enabled:

* print syscall information (including duration)

Timing uses the RISC-V `time` CSR (`r_time()` in `kernel/riscv.h`): the dispatcher reads it immediately before invoking the syscall handler and again after it returns. The printed **milliseconds** value is `(after − before) / TIME_CSR_UNITS_PER_MS`, where `TIME_CSR_UNITS_PER_MS` is 10000 so it lines up with xv6’s comment that adding `1000000` to `stimecmp` is about one tenth of a second (`kernel/trap.c`).

---

### 4. Added Syscall Name Table

A syscall name lookup table was added:

```c
static char *syscallnames[] = {
    [SYS_fork] "fork",
    [SYS_exit] "exit",
    ...
};
```

This converts syscall numbers into readable names.

---

### 5. Tracing Across `fork()`

When a process forks:

* child process inherits tracing mask

Implementation inside process creation:

```c
child->tracemask = parent->tracemask;
```

This ensures tracing continues for child processes.

---

# User-Space `trace` Program

A user-level command named `trace` was implemented.

Usage:

```bash
trace <mask> <command> [arguments]
```

The program:

1. calls `trace(mask)`
2. executes target command using `exec()`

Example:

```bash
trace 32 grep hello README
```

This traces only `read()` syscalls performed by `grep`.

---

# Common Trace Masks

| Syscall    | Calculation  | Mask         |
| ---------- | ------------ | ------------ |
| `fork`     | `1 << 1`     | `2`          |
| `exit`     | `1 << 2`     | `4`          |
| `read`     | `1 << 5`     | `32`         |
| `open`     | `1 << 15`    | `32768`      |
| `write`    | `1 << 16`    | `65536`      |
| Everything | all bits set | `2147483647` |

---

# Example Usage

Trace only `read()`:

```bash
trace 32 grep hello README
```

Trace only `fork()`:

```bash
trace 2 grep hello README
```

Trace all syscalls:

```bash
trace 2147483647 ls
```

Trace only `exit()`:

```bash
trace 4 grep hello README
```

---

# Using `trace()` Inside Your Own Programs

Do **not** include:

```c
#include "user/trace.c"
```

`trace.c` is a standalone executable and contains its own `main()` function.

Instead, directly call the syscall:

```c
#include "kernel/types.h"
#include "user/user.h"

int main() {
    trace(32);

    int fd = open("README", 0);

    char buf[512];

    read(fd, buf, 512);

    return 0;
}
```

The declaration already exists in:

```text
user/user.h
```

---

# Overall System Architecture

```text
┌─────────────────────────────────────────┐
│              User Space                 │
│                                         │
│   trace.c          your_program.c       │
│   (sets mask,      (calls trace(mask)   │
│    runs command)    directly)           │
└────────────────────┬────────────────────┘
                     │ syscall
┌────────────────────▼────────────────────┐
│              Kernel Space               │
│                                         │
│   sys_trace() → sets p->tracemask       │
│                                         │
│   syscall()   → checks mask             │
│                 prints syscall info     │
└─────────────────────────────────────────┘
```

---

# Educational Value

This project provides hands-on experience with:

* Operating system internals
* System call handling
* Kernel-level debugging
* Process structures
* Trap/syscall dispatching
* User/kernel interaction
* Kernel instrumentation

---

# Conclusion

This project successfully implements a lightweight syscall tracing mechanism inside xv6.

The implementation:

* introduces a new `trace()` syscall
* modifies the syscall dispatcher
* tracks process-specific tracing state
* logs syscall activity dynamically

The result is a simplified but functional version of Linux `strace`, useful for debugging, learning, and understanding how operating systems manage user-kernel interactions.
