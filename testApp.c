#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define DEVICE_PATH "/dev/sevenseg"
#define LOOP_DELAY_SEC 1

int main(void)
{
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", DEVICE_PATH, strerror(errno));
        return EXIT_FAILURE;
    }

    for (int digit = 0; digit <= 9; ++digit) {
        char write_buf = '0' + digit;
        ssize_t ret;

        /* Write the new digit */
        ret = write(fd, &write_buf, 1);
        if (ret < 0) {
            fprintf(stderr, "Write failed at digit %d: %s\n", digit, strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }

        /* Reset file position for reading */
        lseek(fd, 0, SEEK_SET);

        /* Read back one byte (the digit character) */
        char read_buf;
        ret = read(fd, &read_buf, 1);
        if (ret < 0) {
            fprintf(stderr, "Read failed after write: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }

        /* Convert ASCII to integer */
        int read_digit = read_buf - '0';
        printf("Displayed (read back): %d\n", read_digit);

        sleep(LOOP_DELAY_SEC);
        /* Reset again so next iteration can read fresh */
        lseek(fd, 0, SEEK_SET);
    }

    close(fd);
    return EXIT_SUCCESS;
}
