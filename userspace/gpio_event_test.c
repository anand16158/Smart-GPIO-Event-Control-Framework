#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#define DEV_NODE "/dev/gpio_event"

int main(void)
{
    int fd;
    struct pollfd pfd;
    char val;

    fd = open(DEV_NODE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("gpio_event_test: opened %s\n", DEV_NODE);

    /* Turn ON output GPIO */
    val = '1';
    write(fd, &val, 1);
    printf("Output GPIO set HIGH\n");

    sleep(1);

    /* Turn OFF output GPIO */
    val = '0';
    write(fd, &val, 1);
    printf("Output GPIO set LOW\n");

    /* Prepare poll */
    pfd.fd = fd;
    pfd.events = POLLIN;

    printf("Waiting for GPIO interrupt event...\n");

    while (1) {
        int ret = poll(&pfd, 1, -1);  /* block forever */

        if (ret < 0) {
            perror("poll");
            break;
        }

        if (pfd.revents & POLLIN) {
            lseek(fd, 0, SEEK_SET);  /* reset file offset */

            if (read(fd, &val, 1) == 1) {
                printf("GPIO interrupt received (value=%c)\n", val);
            }
        }
    }

    close(fd);
    return 0;
}
