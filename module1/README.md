# Module 1: The Foundation - Hello Kernel

Welcome to the world of Linux Kernel Development! In this first module, we are building the simplest possible kernel module. This serves as the foundation for everything else.

## 🎓 Concepts Learned

### 1. What is a Kernel Module?
The Linux Kernel is a monolithic piece of software, but it's designed to be extensible. A **Kernel Module** (LKM) is a piece of code that can be loaded and unloaded into the kernel *on demand*, without recompiling the whole kernel or rebooting.
- **Why?** It allows us to add support for new hardware (drivers) or filesystems dynamically.

### 2. `printk` vs `printf`
In user space (normal C programs), we use `printf` to write to the screen.
- **Why not `printf`?** The kernel runs *before* and *underneath* the C standard library. It doesn't have access to `libc`.
- **Solution**: The kernel provides its own logging function called `printk`. It writes messages to a circular buffer in memory.
- **Viewing Logs**: We use the command `dmesg` (Display Message) to read this buffer.

### 3. Entry and Exit Points
A C program starts at `main()`. A kernel module starts at the function marked with `module_init()` and ends at `module_exit()`.
- **`__init`**: A macro that tells the kernel "this function is only used during initialization". Once the module is loaded, the kernel can free this memory to save RAM.
- **`__exit`**: Used only when removing the module.

### 4. The Build System (Kbuild)
You cannot compile a kernel module with just `gcc main.c`. You need to link it against the **Kernel Headers** (the specific data structures and definitions of the running kernel).
- **Makefile**: Our Makefile invokes the kernel's own build system (`make -C /lib/modules/...`) to ensure binary compatibility.

---

## 🛠️ Implementation Details

We created `vfifo.c` which:
1.  Defines a `buffer_size` parameter.
2.  Prints a message when loaded (`insmod`).
3.  Prints a message when unloaded (`rmmod`).

## 🚀 How to Run

1.  **Compile**:
    ```bash
    make
    ```
    *This creates `vfifo.ko` (Kernel Object).*

2.  **Load the Module**:
    ```bash
    sudo insmod vfifo.ko
    ```
    *This inserts the code into the running kernel (Ring 0).*

3.  **Verify**:
    ```bash
    sudo dmesg | tail
    ```
    *You should see: `vfifo: Module loaded`*

4.  **Unload**:
    ```bash
    sudo rmmod vfifo
    ```
    *This calls the exit function and removes the code.*
