# Module 2: The Interface - Character Devices

In Module 1, our driver lived in the kernel but couldn't talk to anyone. In Module 2, we turn it into a **Character Device** so user-space programs can read and write to it.

## 🎓 Concepts Learned

### 1. User Space vs Kernel Space
- **User Space**: Where normal apps (Chrome, Python, your C code) run. Restricted access.
- **Kernel Space**: Where the driver runs. Full access to hardware.
- **The Barrier**: You cannot just pass a pointer from user space to the kernel. If the user passes a bad pointer, the kernel could crash!
- **Solution**: We use `copy_from_user()` and `copy_to_user()`. These functions safely copy data across the barrier and check for validity.

### 2. Character Devices (`cdev`)
Linux treats almost everything as a file. A **Character Device** is a device that is read/written as a stream of bytes (like a serial port or a terminal).
- **Major Number**: Identifies *which driver* handles the device (e.g., "The vfifo driver").
- **Minor Number**: Identifies *which specific device* it is (e.g., "vfifo number 0", "vfifo number 1").

### 3. File Operations (`fops`)
This is the most important structure in a driver. It maps standard System Calls to your driver functions:
- User calls `open()` -> Kernel calls `vfifo_open()`
- User calls `read()` -> Kernel calls `vfifo_read()`
- User calls `write()` -> Kernel calls `vfifo_write()`

### 4. The Device Node (`/dev/vfifo0`)
To interact with the driver, we need a "file" in the `/dev` directory.
- **Old Way**: Manually run `mknod`.
- **Modern Way**: We use `class_create` and `device_create` in our code to automatically tell `udev` to create `/dev/vfifo0` when the module loads.

---

## 🛠️ Implementation Details

We implemented a **Circular Buffer** (FIFO).
- **Write**: Data comes from user space, we copy it to `dev->buffer`, and move the `head` pointer.
- **Read**: We copy data from `dev->buffer` to user space, and move the `tail` pointer.

## 🚀 How to Run

1.  **Build**:
    ```bash
    make
    gcc test_vfifo.c -o test_vfifo
    ```

2.  **Load**:
    ```bash
    sudo insmod vfifo.ko
    ```

3.  **Test**:
    ```bash
    # Use the provided test app
    sudo ./test_vfifo
    
    # OR use standard Linux tools!
    echo "Hello Linux" | sudo tee /dev/vfifo0
    sudo cat /dev/vfifo0
    ```
