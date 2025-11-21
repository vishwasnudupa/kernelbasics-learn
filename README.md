# Linux Kernel Driver Training Course 🐧

Welcome to your interactive Linux Driver training! This project is designed to take you from "Hello World" to a fully functional, high-performance character device driver.

## 📋 Prerequisites

You **cannot** run this directly on Windows. You need a Linux environment:
1.  **WSL2 (Windows Subsystem for Linux)** - Recommended for Windows users.
2.  **Native Linux** (Ubuntu, Fedora, etc.).
3.  **Linux VM** (VirtualBox/VMware).

**Required Packages:**
You need `gcc`, `make`, and the kernel headers for your running kernel.
```bash
# On Ubuntu/Debian/WSL
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

## 🗺️ Course Structure

The course is divided into 5 progressive modules. Each module builds upon the previous one.

- **[Module 1: The Foundation](./module1)**
    - **Goal**: Compile and load your first kernel module.
    - **Concepts**: `module_init`, `module_exit`, `printk`, `insmod`.

- **[Module 2: The Interface](./module2)**
    - **Goal**: Talk to user space.
    - **Concepts**: Character Devices, `/dev` nodes, `read`/`write`, `copy_to_user`.

- **[Module 3: Concurrency](./module3)**
    - **Goal**: Make it thread-safe.
    - **Concepts**: Race Conditions, Mutexes, Blocking I/O, Wait Queues.

- **[Module 4: Advanced Control](./module4)**
    - **Goal**: Simulate hardware interrupts.
    - **Concepts**: `ioctl`, Kernel Timers, Workqueues (Bottom Halves).

- **[Module 5: High Performance](./module5)**
    - **Goal**: Zero-copy access and system integration.
    - **Concepts**: `mmap` (Memory Mapping), Sysfs attributes.

## 📚 Theory & Reference

We have included a comprehensive textbook-style guide to the concepts used in this course.
👉 **[Read the Kernel Concepts Guide](./kernel-learn.md)**

## 🚀 How to Start

1.  Open your terminal (WSL/Linux).
2.  Navigate to Module 1:
    ```bash
    cd module1
    ```
3.  Read the `README.md` in that folder for specific instructions.
4.  Code, Build, Run, Learn!
5.  Move to the next module when you are ready.

Happy Hacking!
