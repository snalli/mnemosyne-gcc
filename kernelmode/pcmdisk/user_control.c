#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


void write_fs()
{
	int i;
	int fd;
	char buf[1024];

	for (i=0; i<1024; i++) {
		buf[i] = 'c';
	}
	fd = open("/mnt/pcmfs/test", O_CREAT| O_RDWR, S_IRWXU);
	if (fd<0) {
		return;
	}
	while (1) {
		write(fd, buf, 1024);
		fsync(fd);
	}
}


void main(int argc, char **argv)
{
	int fd;
	int r;
	int i;

	if (argc < 2) {
		return;
	}

	fd = open("/dev/pcm0", O_RDWR);
	switch(argv[1][0]) {
		case 'c':
			printf("Crash PCM disk.\n");
			r = ioctl(fd, 32000);
			break;
		case 'r':
			printf("Reset PCM disk.\n");
			r = ioctl(fd, 32001);
			break;
		case 'w':
			write_fs();
			break;
	}
	if (r==-1) {
		printf("%s\n", strerror(errno));
	}
	close(fd);
}	
