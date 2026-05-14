# XV6-System-Call-Tracer

# System Call Tracer (`strace`-like) for xv6

## Overview

This project implements a simplified version of the Linux `strace` utility inside the xv6 operating system.

The tracer monitors and logs system calls made by a process during execution. Whenever a traced process performs a system call, the kernel prints:

* Process ID (`pid`)
* System call name
* Return value

This project demonstrates:

* System call handling
* User mode ↔ kernel mode interaction
* Kernel instrumentation
* Process management in xv6

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

This is similar to the real Linux `strace` utility.

Example output:

```text
pid 4: syscall read -> 512
pid 4: syscall write -> 512
pid 4: syscall exit -> 0
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
        printf("pid %d: syscall %s -> %d\n")
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

* print syscall information

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
