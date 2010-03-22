/**
 * \file main_user.c
 *
 * \brief The user mode front-end to the kernel SCM mappings subsystem.
 *
 * Controls the kernel SCM mappings subsystem through the procfs interface.
 * The idea is that to survive reboots you have to save the mappings in a 
 * file and then load the mappings from that file. If you had persistent 
 * memory then this step wouldn't be necessary. Then you command the SCM 
 * subsystem to reconstruct the address_space page mappings. This is the 
 * boostrapping step which would be necessary even if you had persistent 
 * memory since the page mappings live in volatile memory.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "scmmap-userkernel.h"

#define PROG_NAME              "scmmap_user"
#define PROCFS_SCMMAP_MAPPING  "/proc/scmmap/mapping"
#define PROCFS_SCMMAP_CONTROL  "/proc/scmmap/control"

enum {
	CMD_UNKNOWN = -1,
	CMD_LOAD_MAPPINGS = 0,
	CMD_SAVE_MAPPINGS,
	CMD_LOAD_MAPPINGS_TEST,
	CMD_PRINT_MAPPINGS,
	CMD_EXECUTE_CONTROL_COMMAND,
};


int load_mappings_test()
{
	int                    rv = 0;
	int                    i;
	int                    fd_sm;
	int                    count;
	struct scmpage_mapping mapping;
	
	fd_sm = open(PROCFS_SCMMAP_MAPPING, O_WRONLY);

	for (i=0; i<1024; i++) {
		mapping.dev = 0;
		mapping.ino = 0;
		mapping.ino_gen = 0;
		mapping.offset = 0;
		mapping.pfn = i;
		count = pwrite(fd_sm, &mapping, 1, i);
		if (count != 1) {
			fprintf(stderr, "Couldn't load mapping.\n");
			rv = count;
			goto out;
		}	
	}
	
	close(fd_sm);

out:
	return rv;

}


int load_mappings_from_file(char *filename)
{
	int                    rv = 0;
	int                    i;
	int                    fd_sm;
	int                    fd_in;
	int                    count;
	struct scmpage_mapping mapping;
	struct stat            tmp_stat;
	off_t                  offset;
	int                    num_mappings = 0;
	
	fd_in = open(filename, O_RDONLY);
	if (fd_in < 0) {
		rv = fd_in;
		goto out;
	}
	fd_sm = open(PROCFS_SCMMAP_MAPPING, O_WRONLY);
	if (fd_sm < 0) {
		rv = fd_sm;
		goto out;
	}
	fstat(fd_in, &tmp_stat);
	
	num_mappings = tmp_stat.st_size / sizeof(struct scmpage_mapping);
	for (i=0; i<num_mappings; i++) {
		offset = sizeof(struct scmpage_mapping) * i;
		count = pread(fd_in, &mapping, sizeof(struct scmpage_mapping), offset);
		if (count != sizeof(struct scmpage_mapping)) {
			goto err_read;
		}
		count = pwrite(fd_sm, &mapping, 1, 0xFFFFFFFF);
		if (count != 1) {
			goto err_write;
		}
	}

	close(fd_sm);
	close(fd_in);
	goto out;

err_write:
err_read:
	close(fd_sm);	
err_fd_sm:
	close(fd_in);
out:
	return rv;

}


int save_mappings_to_file(char *filename)
{
	int                    rv = 0;
	int                    i;
	int                    fd_sm;
	int                    fd_out;
	int                    count;
	struct scmpage_mapping mapping;
	struct stat            tmp_stat;
	off_t                  offset;
	int                    num_mappings = 0;
	
	fd_out = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 00666);
	if (fd_out < 0) {
		rv = fd_out;
		goto out;
	}
	fd_sm = open(PROCFS_SCMMAP_MAPPING, O_RDONLY);
	if (fd_out < 0) {
		rv = fd_sm;
		goto out;
	}
	fstat(fd_sm, &tmp_stat);
	
	num_mappings = tmp_stat.st_size;
	for (i=0; i<num_mappings; i++) {
		offset = sizeof(struct scmpage_mapping) * i;
		count = pread(fd_sm, &mapping, 1, i);
		if (count != 1) {
			goto err_read;
		}
		count = pwrite(fd_out, &mapping, sizeof(struct scmpage_mapping), offset);
		if (count != sizeof(struct scmpage_mapping)) {
			goto err_write;
		}
	}

	close(fd_sm);
	close(fd_out);
	goto out;

err_write:
err_read:
	close(fd_sm);	
err_fd_sm:
	close(fd_out);
out:
	return rv;
}


void print_mapping(int idx, struct scmpage_mapping *mapping)
{
	printf("MAPPING: %d\n", idx);
	printf("  .dev      = 0x%lx\n", mapping->dev);
	printf("  .ino      = 0x%lx\n", mapping->ino);
	printf("  .ino_gen  = %lu\n", mapping->ino_gen);
	printf("  .offset   = %lu\n", mapping->offset);
	printf("  .pfn      = %lu\n", mapping->pfn);

}


int print_mappings(char *filename)
{
	int                    rv = 0;
	int                    i;
	int                    fd;
	int                    count;
	struct scmpage_mapping mapping;
	struct stat            tmp_stat;
	off_t                  offset;
	int                    num_mappings = 0;
	
	if (filename) {
		fd = open(filename, O_RDONLY);
	} else {
		fd = open(PROCFS_SCMMAP_MAPPING, O_RDONLY);
	}
	if (fd < 0) {
		rv = fd;
		goto out;
	}
	fstat(fd, &tmp_stat);
	num_mappings = tmp_stat.st_size;

	for (i=0; i<num_mappings; i++) {
		if (filename) {
			offset = sizeof(struct scmpage_mapping) * i;
			count = pread(fd, &mapping, sizeof(struct scmpage_mapping), offset);
			if (count != sizeof(struct scmpage_mapping)) {
				goto err_read;
			}
		} else {
			offset = i;
			count = pread(fd, &mapping, 1, offset);
			if (count != 1) {
				goto err_read;
			}
		}
		print_mapping(i, &mapping);
	}

	close(fd);
	goto out;

err_read:
	close(fd);	
out:
	return rv;
}


int execute_control_command(char *cmd_str)
{
	struct scmmap_control_msg control_msg;
	int                       fd;
	int                       rv = 0;

	if (strcmp(cmd_str, SCMMAP_CMD_RECONSTRUCT_PAGES) == 0) {
		strcpy(control_msg.cmd, cmd_str);
		control_msg.payload_size = 0;
		control_msg.payload_buf = NULL;
	} else {
		rv = -1; /* unknown command */
		goto out;
	}

	fd = open(PROCFS_SCMMAP_CONTROL, O_WRONLY);
	if (fd < 0) {
		rv = fd;
		goto out;
	}
	pwrite(fd, &control_msg, sizeof(struct scmmap_control_msg), 0);
	close(fd);
out:	
	return rv;
}


void usage()
{
	fprintf(stderr, "usage: %s <OPTIONS>\n", PROG_NAME);
	fprintf(stderr, "\n");
	fprintf(stderr, "About:\n");
	fprintf(stderr, "\tControls the SCM mapping kernel subsystem.\n");
	fprintf(stderr, "\t\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "\t-t            Load test mappings\n");
	fprintf(stderr, "\t-l -f <FILE>  Load mappings from file <FILE>\n");
	fprintf(stderr, "\t-s -f <FILE>  Save mappings to file <FILE>\n");
	fprintf(stderr, "\t-p            Print active mappings kept in kernel\n");
	fprintf(stderr, "\t-p -f <FILE>  Print mappings of file <FILE>\n");
	fprintf(stderr, "\t-c <CMD>      Send control command <CMD> to the SCM mapping kernel subsystem\n");
	fprintf(stderr, "\t\n");
	fprintf(stderr, "Control commands:\n");
	fprintf(stderr, "\t%-14sReconstruct pages\n", SCMMAP_CMD_RECONSTRUCT_PAGES);
}


int
main(int argc, char *argv[])
{
	int         cmd = CMD_UNKNOWN;
	int         c;
	int         ret;
	int         errflg = 0;
	char        *file = NULL;
	char        *control_command = NULL;
	extern char *optarg;

	while ((c = getopt(argc, argv, ":ltspc:f:")) != -1) {
		switch(c) {
			case 'l':
				cmd = CMD_LOAD_MAPPINGS;
				break;
			case 's':
				cmd = CMD_SAVE_MAPPINGS;
				break;
			case 't':
				cmd = CMD_LOAD_MAPPINGS_TEST;
				break;
			case 'p':
				cmd = CMD_PRINT_MAPPINGS;
				break;
			case 'c':
				cmd = CMD_EXECUTE_CONTROL_COMMAND;
				control_command = optarg;
				break;
			case 'f':
				file = optarg;
				break;
        	case ':':       
				fprintf(stderr,
				        "Option -%c requires an operand\n", optopt);
				errflg++;
				break;
			case '?':
				fprintf(stderr,
				        "Unrecognized option: -%c\n", optopt);
				errflg++;
		}
	}

	if (errflg) {
		goto err_usage;
	}

	switch(cmd) {
		case CMD_UNKNOWN:
			goto err_usage;
		case CMD_LOAD_MAPPINGS:
			if (file == NULL) goto err_usage;
			ret = load_mappings_from_file(file);	
			return ret;
		case CMD_LOAD_MAPPINGS_TEST:
			ret = load_mappings_test();	
			return ret;
		case CMD_SAVE_MAPPINGS:
			if (file == NULL) goto err_usage;
			ret = save_mappings_to_file(file);
			return ret;
		case CMD_PRINT_MAPPINGS:
			ret = print_mappings(file);
			return ret;
		case CMD_EXECUTE_CONTROL_COMMAND:
			ret = execute_control_command(control_command);
			return ret;
	}

err_usage:
	usage();
	exit(2);
}
