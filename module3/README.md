# Module 3: Concurrency - Making it Safe

Module 2 worked, but it had a fatal flaw: **Race Conditions**. If two programs tried to write at the exact same time, they would corrupt the buffer pointers. Module 3 fixes this.

## 🎓 Concepts Learned

### 1. Race Conditions
Imagine two threads trying to increment a variable `size` (currently 5) at the same time.
- Thread A reads 5.
- Thread B reads 5.
- Thread A adds 1 -> 6, writes it back.
- Thread B adds 1 -> 6, writes it back.
- **Result**: 6. **Expected**: 7.
In a kernel driver, this can crash the system or leak memory.

### 2. The Mutex (`struct mutex`)
A **Mutex** (Mutual Exclusion) is like a key to a bathroom. Only one person (thread) can hold the key at a time.
- **Lock**: "I want the key." If someone else has it, I wait.
- **Unlock**: "I am done, here is the key."
- We wrap our buffer access code in `mutex_lock()` and `mutex_unlock()` to ensure only one thread modifies the buffer at a time.

### 3. Blocking I/O (Sleeping)
What happens if a user tries to `read()` from an empty buffer?
- **Busy Wait (Bad)**: `while(empty);` This burns 100% CPU doing nothing.
- **Blocking (Good)**: The process says "Wake me up when there is data" and goes to sleep. It uses 0% CPU while waiting.

### 4. Wait Queues
This is how Linux implements sleeping.
- **`wait_event_interruptible(queue, condition)`**: "Put me to sleep on this queue until `condition` becomes true."
- **`wake_up_interruptible(queue)`**: "Hey everyone on this queue, check your condition again!"

---

## 🛠️ Implementation Details

- Added `struct mutex lock` to `vfifo_dev`.
- Added `wait_queue_head_t read_queue` and `write_queue`.
- **Read Logic**:
    1. Lock Mutex.
    2. Is buffer empty?
       - Yes: Unlock Mutex, Sleep on `read_queue`. (Repeat when woken).
    3. Copy data.
    4. Wake up writers (since we made space).
    5. Unlock Mutex.

## 🚀 How to Run

1.  **Build**:
    ```bash
    make
    gcc test_concurrency.c -o test_concurrency -pthread
    ```

2.  **Load**:
    ```bash
    sudo insmod vfifo.ko
    ```

3.  **Test**:
    ```bash
    sudo ./test_concurrency
    ```
    *Watch closely! The reader thread will start, print "Trying to read...", and then **STOP**. It is sleeping in the kernel. 2 seconds later, the main thread writes, and the reader immediately wakes up.*
