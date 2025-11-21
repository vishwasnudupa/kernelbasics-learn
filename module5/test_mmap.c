#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define DEVICE_PATH "/dev/vfifo0"
#define BUFFER_SIZE 4096

int main() {
    int fd;
    char *map;
    char write_msg[] = "Hello via Memory Map!";

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    /* Map the kernel buffer into our address space */
    map = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("MMAP failed");
        close(fd);
        return 1;
    }

    printf("1. Writing to mapped memory directly...\n");
    /* Note: We are bypassing the driver's read/write logic (head/tail), 
       so we are just writing raw bytes to the start of the buffer. */
    strncpy(map, write_msg, sizeof(write_msg));

    printf("2. Verifying data in memory...\n");
    printf("   Mapped Content: \"%s\"\n", map);

    /* Clean up */
    munmap(map, BUFFER_SIZE);
    close(fd);
    return 0;
}
