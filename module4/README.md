# Module 4: Advanced Control - IOCTL & Deferred Work

Now we are getting into professional driver territory. We need to control the device (not just read/write) and handle hardware events (simulated).

## 🎓 Concepts Learned

### 1. IOCTL (Input/Output Control)
`read` and `write` are for data. But what if you want to change the baud rate, reset the device, or change a setting?
- We use a special system call: `ioctl`.
- It takes a command number (e.g., `VFIFO_CLEAR`) and an argument.
- It's the "Swiss Army Knife" of system calls.

### 2. Hardware Interrupts (Simulated)
Real hardware (network cards, keyboards) sends **Interrupts** to the CPU when they have data. The CPU stops what it's doing to handle the interrupt.
- Since we don't have real hardware, we use a **Kernel Timer**. It ticks every second, simulating data arriving from a sensor.

### 3. Atomic Context vs Process Context
- **Interrupt Context**: When a timer or hardware interrupt fires, the kernel is in "Atomic Context". **You cannot sleep here.** You cannot hold a Mutex (because waiting for a mutex might sleep).
- **Process Context**: Normal driver code (read/write/ioctl) runs on behalf of a user process. You can sleep.

### 4. Deferred Work (Bottom Halves)
Since the Timer (Atomic Context) cannot take our Mutex to write to the buffer safely, it must ask someone else to do it.
- **Workqueue**: A mechanism to schedule a function to run later in **Process Context**.
- **The Flow**:
    1. Timer fires (Atomic) -> Schedules Work.
    2. Timer returns immediately.
    3. Kernel Worker Thread wakes up (Process Context) -> Locks Mutex -> Writes Data -> Unlocks.

---

## 🛠️ Implementation Details

- **IOCTL**: Added `VFIFO_CLEAR` to wipe the buffer and `VFIFO_SET_MODE` to turn on the timer.
- **Timer**: Fires every 1000ms.
- **Workqueue**: The timer function `vfifo_timer_func` just calls `schedule_work`. The worker function `vfifo_work_handler` does the actual writing.

## 🚀 How to Run

1.  **Build**:
    ```bash
    make
    gcc test_ioctl.c -o test_ioctl
    ```

2.  **Load**:
    ```bash
    sudo insmod vfifo.ko
    ```

3.  **Test**:
    ```bash
    sudo ./test_ioctl
    ```
    *You will see the program enable "Auto-Generation". Then it sleeps. While it sleeps, the Kernel Timer is firing in the background, filling the buffer. When the program wakes up, it reads the data that appeared "magically".*
