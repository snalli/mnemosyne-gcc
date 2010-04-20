#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "file.h"

/** 
 * \brief Creates a file of a given size.
 */
int 
m_create_file(char *file, ssize_t size)
{
	int      fd;
	ssize_t  roundup_size;
	char     buf[1]; 
	
	fd = open(file, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		return fd;
	}
	lseek(fd, size, SEEK_SET);
	write(fd, buf, 1);
	fsync(fd); /* make sure the file metadata is synced */

	return fd;
}
