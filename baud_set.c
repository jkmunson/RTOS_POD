#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <asm/termios.h>
//#include <sys/ioctl.h>

int main(int argc, char* argv[]) {

    if (argc != 3) {
        printf("%s device speed\n\nSet speed for a serial device.\nFor instance:\n    %s /dev/ttyUSB0 75000\n", argv[0], argv[0]);                   
        return -1;
    }

    int fd = open(argv[1], O_RDONLY);

    int speed = atoi(argv[2]);

    struct termios2 tio;
    ioctl(fd, TCGETS2, &tio);
    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER;
    tio.c_ispeed = speed;
    tio.c_ospeed = speed;
    int r = ioctl(fd, TCSETS2, &tio);
    close(fd);

    if (r == 0) {
        printf("Changed successfully.\n");
    } else {
        perror("ioctl");
    }
}
