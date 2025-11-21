#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define DEVICE_PATH "/dev/vfifo0"

void *reader_thread(void *arg) {
    int fd = *(int *)arg;
    char buf[100];
    ssize_t ret;

    printf("[Reader] Trying to read (should block if empty)...\n");
    ret = read(fd, buf, sizeof(buf)-1);
    if (ret > 0) {
        buf[ret] = '\0';
        printf("[Reader] Read %zd bytes: \"%s\"\n", ret, buf);
    } else {
        perror("[Reader] Read failed");
    }
    return NULL;
}

int main() {
    int fd;
    pthread_t tid;
    char write_buf[] = "Delayed Data";

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    /* Start a reader thread. It should block because buffer is empty. */
    pthread_create(&tid, NULL, reader_thread, &fd);

    printf("[Main] Sleeping for 2 seconds to simulate delay...\n");
    sleep(2);

    printf("[Main] Writing data now...\n");
    write(fd, write_buf, strlen(write_buf));

    pthread_join(tid, NULL);
    close(fd);
    return 0;
}
