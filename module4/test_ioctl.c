#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/vfifo0"

#define VFIFO_IOC_MAGIC 'k'
#define VFIFO_CLEAR     _IO(VFIFO_IOC_MAGIC, 1)
#define VFIFO_SET_MODE  _IOW(VFIFO_IOC_MAGIC, 2, int)

int main() {
    int fd;
    int mode_auto = 1;
    int mode_manual = 0;
    char buf[100];
    ssize_t ret;

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    printf("1. Clearing Buffer via IOCTL...\n");
    if (ioctl(fd, VFIFO_CLEAR) < 0) {
        perror("IOCTL Clear failed");
        return 1;
    }

    printf("2. Enabling Auto-Generation (Hardware Simulation)...\n");
    if (ioctl(fd, VFIFO_SET_MODE, &mode_auto) < 0) {
        perror("IOCTL Set Mode failed");
        return 1;
    }

    printf("   Waiting for 3 seconds while kernel generates data...\n");
    sleep(3);

    printf("3. Reading generated data...\n");
    memset(buf, 0, sizeof(buf));
    ret = read(fd, buf, sizeof(buf)-1);
    if (ret > 0) {
        printf("   Read %zd bytes: \"%s\"\n", ret, buf);
    } else {
        printf("   Read 0 bytes (Buffer empty?)\n");
    }

    printf("4. Disabling Auto-Generation...\n");
    ioctl(fd, VFIFO_SET_MODE, &mode_manual);

    close(fd);
    return 0;
}
