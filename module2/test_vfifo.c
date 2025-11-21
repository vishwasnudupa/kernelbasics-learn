#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/vfifo0"

int main() {
    int fd;
    char write_buf[] = "Hello from User Space!";
    char read_buf[100];
    ssize_t ret;

    printf("Opening device %s...\n", DEVICE_PATH);
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    printf("Writing data: \"%s\"\n", write_buf);
    ret = write(fd, write_buf, strlen(write_buf));
    if (ret < 0) {
        perror("Failed to write to device");
        close(fd);
        return 1;
    }
    printf("Wrote %zd bytes.\n", ret);

    printf("Reading data back...\n");
    memset(read_buf, 0, sizeof(read_buf));
    ret = read(fd, read_buf, sizeof(read_buf)-1);
    if (ret < 0) {
        perror("Failed to read from device");
        close(fd);
        return 1;
    }
    printf("Read %zd bytes: \"%s\"\n", ret, read_buf);

    if (strcmp(write_buf, read_buf) == 0) {
        printf("SUCCESS: Data verified!\n");
    } else {
        printf("FAILURE: Data mismatch!\n");
    }

    close(fd);
    return 0;
}
