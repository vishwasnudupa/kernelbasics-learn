# Module 5: High Performance - Memory Mapping & Sysfs

This is the final level. We are optimizing performance and adding system integration.

## 🎓 Concepts Learned

### 1. Memory Mapping (`mmap`)
Normally, `read()` involves copying data from Kernel Space to User Space. This is slow for large amounts of data (like video).
- **Zero Copy**: `mmap` allows us to map the physical memory of the kernel buffer *directly* into the user program's virtual memory.
- The user program can read/write the buffer just by accessing a pointer! No system calls needed.

### 2. Virtual Memory Areas (VMA)
- When you call `mmap`, the kernel creates a `vm_area_struct`.
- Our driver's job is to map the **Physical Page Frames (PFN)** of our buffer to this VMA using `remap_pfn_range`.

### 3. Sysfs (`/sys`)
`/dev` is for accessing the device. `/proc` is for process info. `/sys` is for the **Device Model**.
- It allows us to expose attributes (like `size`, `capacity`, `battery_level`) as files.
- Scripts can read these values easily without writing C code.
- We use `kobjects` and `attributes` to create these files automatically.

---

## 🛠️ Implementation Details

- **`vfifo_mmap`**: Calculates the physical address of `dev->buffer` and maps it to the user's VMA.
- **Sysfs**: Created a group of attributes (`size`, `capacity`, `mode`) that appear in `/sys/class/vfifo/vfifo0/`.

## 🚀 How to Run

1.  **Build**:
    ```bash
    make
    gcc test_mmap.c -o test_mmap
    ```

2.  **Load**:
    ```bash
    sudo insmod vfifo.ko
    ```

3.  **Test Sysfs**:
    ```bash
    # Read the capacity without a C program!
    cat /sys/class/vfifo/vfifo0/capacity
    
    # Turn on auto-generation using echo!
    echo 1 | sudo tee /sys/class/vfifo/vfifo0/mode
    ```

4.  **Test Mmap**:
    ```bash
    sudo ./test_mmap
    ```
    *The test program writes to a memory pointer. It doesn't call `write()`. Yet, the data ends up in the kernel buffer.*

---

## 🏁 Course Completion

Congratulations! You have built a driver that covers the core pillars of Linux Kernel Development:
1.  **Modules & Kbuild**
2.  **Char Device Interface**
3.  **Concurrency & Locking**
4.  **Interrupts & Deferred Work**
5.  **Memory Management & Sysfs**

You are now ready to tackle real hardware drivers!
