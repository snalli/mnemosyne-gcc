#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>


#define PCMDISK_CRASH                 32000
#define PCMDISK_CRASH_RESET           32001
#define PCMDISK_SET_PCM_BANDWIDTH     32002
#define PCMDISK_GET_PCM_BANDWIDTH     32003
#define PCMDISK_SET_DRAM_BANDWIDTH    32004
#define PCMDISK_GET_DRAM_BANDWIDTH    32005
#define PCMDISK_RESET_STATISTICS      32006
#define PCMDISK_PRINT_STATISTICS      32007



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


int
set_pcm_bw(int fd, int val)
{
	int r;

	if (val >=0) {
		printf("Set PCM bandwidth: %d\n", val);
		if ((r = ioctl(fd, PCMDISK_SET_PCM_BANDWIDTH, val)) < 0) {
			goto err;
		}
	}

	return 0;
err:
	return -1;
}

int
set_dram_bw(int fd, int val)
{
	int r;

	if (val >=0) {
		printf("Set DRAM bandwidth: %d\n", val);
		if ((r = ioctl(fd, PCMDISK_SET_DRAM_BANDWIDTH, val)) < 0) {
			goto err;
		}
	}

	return 0;
err:
	return -1;
}


int
reset_statistics(int fd)
{
	int r;

	printf("Reset statistics\n");
	if ((r = ioctl(fd, PCMDISK_RESET_STATISTICS, 0)) < 0) {
		goto err;
	}

	return 0;
err:
	return -1;
}


int
print_statistics(int fd)
{
	int r;

	printf("Print statistics...check output with dmesg\n");
	if ((r = ioctl(fd, PCMDISK_PRINT_STATISTICS, 0)) < 0) {
		goto err;
	}

	return 0;
err:
	return -1;
}



int
print_config(FILE *fout, int fd)
{
	int pcm_bw;
	int dram_bw;

	if ((pcm_bw = ioctl(fd, PCMDISK_GET_PCM_BANDWIDTH)) < 0) {
		goto err;
	}
	if ((dram_bw = ioctl(fd, PCMDISK_GET_DRAM_BANDWIDTH)) < 0) { 
		goto err;
	}
	
	fprintf(fout, "PCMDISK CONFIGURATION\n");
	fprintf(fout, "==================================\n");
	fprintf(fout, "PCM bandwidth         %d (MB/s)\n", pcm_bw);
	fprintf(fout, "DRAM bandwidth        %d (MB/s)\n", dram_bw);

	return 0;
err:
	return -1;
}

static
void usage(FILE *fout, char *name) 
{
	fprintf(fout, "usage:\n");
	fprintf(fout, "\n");
	fprintf(fout, "Options\n");
	fprintf(fout, "  %s  %s\n", "--crash       ", "Crash PCM-disk");
	fprintf(fout, "  %s  %s\n", "--reset       ", "Reset PCM-disk");
	fprintf(fout, "  %s  %s\n", "--print-config", "Print PCM-disk configuration");
	fprintf(fout, "  %s  %s\n", "--set-pcm-bw  ", "Set PCM bandwidth");
	fprintf(fout, "  %s  %s\n", "--set-dram-bw ", "Set DRAM bandwidth");
	fprintf(fout, "  %s  %s\n", "--reset-stat  ", "Reset statistics");
	fprintf(fout, "  %s  %s\n", "--print-stat  ", "Print statistics");
	exit(1);
}

int
main(int argc, char **argv)
{
	int  fd;
	int  r;
	int  i;
	int  c;
	int  val;
	char *prog_name = "userctl";

	r = fd = open("/dev/pcm0-ctl", O_RDWR);
	if (r <0) {
		printf("%s\n", strerror(errno));
		goto err;
	}

	while (1) {
		static struct option long_options[] = {
			{"reset",  no_argument, 0, 'R'},
			{"crash",  no_argument, 0, 'C'},
			{"print-config", no_argument, 0, 'f'},
			{"set-pcm-bw", required_argument, 0, 'p'},
			{"set-dram-bw", required_argument, 0, 'd'},
			{"reset-stat", no_argument, 0, 'r'},
			{"print-stat", no_argument, 0, 's'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "RCfp:d:rs",
		                 long_options, &option_index);
     
		/* Detect the end of the options. */
		if (c == -1)
			break;
     
		switch (c) {
			case 'C':
				printf("Crash PCM disk.\n");
				r = ioctl(fd, 32000);
				break;
			case 'R':
				printf("Reset PCM disk.\n");
				r = ioctl(fd, 32001);
				break;
			case 'p':
				val = atoi(optarg);
				if (set_pcm_bw(fd, val) < 0) {
					goto err;
				}
				break;
			case 'd':
				val = atoi(optarg);
				if (set_dram_bw(fd, val) < 0) {
					goto err;
				}
				break;
			case 'f':
				if ((r=print_config(stderr, fd)) < 0) {
					goto err;
				}
				break;
			case 'w':
				write_fs();
				break;
			case 'r':
				reset_statistics(fd);
				break;
			case 's':
				print_statistics(fd);
				break;
			case '?':
				/* getopt_long already printed an error message. */
				usage(stderr, prog_name);
				break;
 
		}
	}

	close(fd);
	return 0;

err:
	return r;
}	
