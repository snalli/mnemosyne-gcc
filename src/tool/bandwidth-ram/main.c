/*============================================================================
  bandwidth 0.16, a benchmark to estimate memory transfer bandwidth.
  Copyright (C) 2005,2007-2009 by Zack T Smith.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  The author may be reached at fbui@comcast.net.
 *===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#if !defined(__CYGWIN__)
#include <linux/fb.h>
#endif

#define VERSION "0.16"

#ifndef bool
typedef char bool;
enum { true = 1, false = 0 };
#endif

extern int Reader (void *ptr, unsigned long loops, unsigned long size);
extern int ReaderSSE2 (void *ptr, unsigned long loops, unsigned long size);
extern int Writer (void *ptr, unsigned long loops, unsigned long size, unsigned long value);
extern int WriterSSE2 (void *ptr, unsigned long loops, unsigned long size, unsigned long value);
extern int my_bzero (unsigned char *ptr, unsigned long length);
extern int my_bzeroSSE2 (unsigned char *ptr, unsigned long length);

#define FBLOOPS_R 400
#define FBLOOPS_W 800
#if __x86_64__
#define FB_SIZE (1<<18)
#endif

static float cpu_speed = 0.0;

static long usec_per_test = 15000000;

//----------------------------------------------------------------------------
// Name:	mytime
// Purpose:	Reports time in microseconds.
//----------------------------------------------------------------------------
unsigned long mytime ()
{
	struct timeval tv;
	struct timezone tz;
	bzero (&tz, sizeof(struct timezone));
	gettimeofday (&tv, &tz);
	return 1000000 * tv.tv_sec + tv.tv_usec;
}

//----------------------------------------------------------------------------
// Name:	seq_write
// Purpose:	Performs sequential write on chunk of memory of specified size.
//----------------------------------------------------------------------------
void
seq_write (unsigned long size, bool use_sse2)
{
	unsigned char *chunk;
	unsigned long loops, total_count=0;
#if __x86_64__
	unsigned long value = 0x1234567689abcdef;
#else
	unsigned long value = 0x12345678;
#endif
	double rate;
	unsigned long diff=0, t0;
	unsigned long tmp;
	char size_str[30];

	chunk = malloc (size+16);
	if (!chunk) {
		perror("malloc");
	}
	bzero (chunk, size);
	tmp = (unsigned long) chunk;
	if (tmp & 7) {
		tmp -= (tmp & 7);
		tmp += 8;
		chunk = (unsigned char*) tmp;
	}

	//-------------------------------------------------
	if (size < 1024)
		sprintf (size_str, "%lu, ", size);
	else
	if (size < (1<<20))
		sprintf (size_str, "%lukB, ", size/1024);
	else
		sprintf (size_str, "%luMB, ", size/(1<<20));

	if (use_sse2) 
		printf ("Sequential write (128-bit (bypasses caches), size=%lu, ", size);
	else
		printf ("Sequential write (%d-bit, size=%lu, ", 
#if __x86_64__
	64
#else
	32
#endif
			,size);

	loops = (1<<30) / size;// XX need to adjust for CPU MHz
	printf ("loops = %lu) ", loops);
	fflush (stdout);

	bzero (chunk, size);

	t0 = mytime ();

	total_count += loops;

	if (use_sse2)
		WriterSSE2 (chunk, loops, size, value);
	else
		Writer (chunk, loops, size, value);
	diff = mytime () - t0;

	rate = size;		// bytes written.
	rate *= total_count;	// how many times written.
	rate /= diff;		// per usec.
	rate *= 1000000;	// per second.
	rate /= (1<<20);	// megs

	printf ("%g MB/sec \n", rate);
	free ((void*)chunk);
}


//----------------------------------------------------------------------------
// Name:	seq_read
// Purpose:	Performs sequential read on chunk of memory of specified size.
//----------------------------------------------------------------------------
void
seq_read (unsigned long size, bool use_sse2)
{
	unsigned long loops, total_count = 0;
	double rate;
	unsigned long t, t0, diff=0;
	unsigned char *chunk;
	unsigned long tmp;
	char size_str[30];

	//-------------------------------------------------

	chunk = malloc (size+32);
	if (!chunk) {
		perror("malloc");
	}
	bzero (chunk, size);
	tmp = (unsigned long) chunk;
	if (tmp & 15) {
		tmp -= (tmp & 15);
		tmp += 16;
		chunk = (unsigned char*) tmp;
	}

	if (size < 1024)
		sprintf (size_str, "%lu, ", size);
	else
	if (size < (1<<20))
		sprintf (size_str, "%lukB, ", size/1024);
	else
		sprintf (size_str, "%luMB, ", size/(1<<20));

	printf ("Sequential read (%d-bit, size=%lu, ", use_sse2 ? 128 :
#if __x86_64__
	64
#else
	32
#endif
		, size);

	loops = (1<<30) / size;	// XX need to adjust for CPU MHz
	printf ("loops = %lu) ", loops);
	fflush (stdout);

	t0 = mytime ();

	total_count += loops;

	if (use_sse2)
		ReaderSSE2 (chunk, loops, size);
	else
		Reader (chunk, loops, size);

	diff = mytime () - t0;

	rate = total_count;	// # times read.
	rate *= size;		// bytes read.
	rate /= diff;		// per usec.
	rate *= 1000000;	// per second.
	rate /= (1<<20);	// megs

	printf ("%g MB/sec \n", rate);
	free (chunk);
}


//----------------------------------------------------------------------------
// Name:	fb_readwrite
// Purpose:	Performs sequential read & write tests on framebuffer memory.
//----------------------------------------------------------------------------
#if !defined(__CYGWIN__)
void
fb_readwrite (bool use_sse2)
{

	unsigned long counter, total_count;
	unsigned long length;
	double rate;
	unsigned long diff, t0;
	static struct fb_fix_screeninfo fi;
	static struct fb_var_screeninfo vi;
	unsigned long *fb = NULL;
	unsigned long datum;
	int fd;
	register unsigned long foo;
#if __x86_64__
	unsigned long value = 0x1234567689abcdef;
#else
	unsigned long value = 0x12345678;
#endif

	//-------------------------------------------------

	fd = open ("/dev/fb0", O_RDWR);
	if (fd < 0) 
		fd = open ("/dev/fb/0", O_RDWR);
	if (fd < 0) {
		perror ("open");
		printf ("Can't open framebuffer.\n");
		exit (0);
	}

	if (ioctl (fd, FBIOGET_FSCREENINFO, &fi))
	{
		perror("ioctl");
		exit (1);
	}
	else
	if (ioctl (fd, FBIOGET_VSCREENINFO, &vi))
	{
		perror("ioctl");
		exit (1);
	}
	else
	{
		if (fi.visual != FB_VISUAL_TRUECOLOR &&
		    fi.visual != FB_VISUAL_DIRECTCOLOR )
		{
			fprintf (stderr, "Error: need direct/truecolor framebuffer device.\n");
			exit (1);
		}
		else
		{
			unsigned long fblen;

			printf ("Framebuffer resolution: %dx%d, %dbpp\n",
				vi.xres, vi.yres, vi.bits_per_pixel);

			fb = (unsigned long*) fi.smem_start;
			fblen = fi.smem_len;

			fb = mmap (fb, fblen,
				PROT_WRITE | PROT_READ,
				MAP_SHARED, fd, 0);
			if (fb == MAP_FAILED)
			{
				perror ("mmap");
				exit (1);
			}
		}
	}

	//-------------------
	// Use only the upper half of the display.
	//
#if __x86_64__
	length = FB_SIZE;
#else
	length = (vi.xres * vi.yres * vi.bits_per_pixel / 8) / 2;
#endif

	//-------------------
	// READ

	printf ("Framebuffer memory sequential read ");
	fflush (stdout);

	t0 = mytime ();
	diff = 0;

	total_count = FBLOOPS_R;

	if (use_sse2)
		ReaderSSE2 (fb, FBLOOPS_R, length);
	else
		Reader (fb, FBLOOPS_R, length);

	diff = mytime () - t0;

	rate = total_count;
	rate *= length;
	rate /= diff;
	rate *= 1000000;
	rate /= (1<<20);

	printf ("%g MB/sec \n", rate);

	//-------------------
	// WRITE

	printf ("Framebuffer memory sequential write ");
	fflush (stdout);

	t0 = mytime ();
	diff = 0;

	total_count = FBLOOPS_W;

	if (use_sse2) 
		WriterSSE2 (fb, FBLOOPS_W, length, value);
	else
		Writer (fb, FBLOOPS_W, length, value);

	diff = mytime () - t0;

	rate = total_count;
	rate *= length;
	rate /= diff;
	rate *= 1000000;
	rate /= (1<<20);

	printf ("%g MB/sec \n", rate);
}
#endif


//----------------------------------------------------------------------------
// Name:	library_test
// Purpose:	Performs C library tests (memset, memcpy).
//----------------------------------------------------------------------------
int
library_test () 
{
	double rate;
	char *a1, *a2;
	unsigned long t, t0;
	int i;

#define NT_SIZE (64*1024*1024)
#define NT_SIZE2 (100)

	a1 = malloc (NT_SIZE);
	if (!a1) {
		perror("malloc1");
		return -1;
	}
	a2 = malloc (NT_SIZE);
	if (!a2) {
		perror("malloc2");
		return -1;
	}

	//--------------------------------------
	t0 = mytime ();
	for (i=0; i<NT_SIZE2; i++) {
		memset (a1, i, NT_SIZE);
	}
	t = mytime ();
	rate = NT_SIZE;
	rate *= NT_SIZE2;
	rate /= (t-t0);
	rate *= 1000000;
	rate /= (1<<20);

	printf ("Library: ");
	fflush (stdout);

	printf ("memset %g MB/sec \n", rate);

	//--------------------------------------
	t0 = mytime ();
	for (i=0; i<NT_SIZE2; i++) {
		memcpy (a2, a1, NT_SIZE);
	}
	t = mytime ();
	rate = NT_SIZE;
	rate *= NT_SIZE2;
	rate /= (t-t0);
	rate *= 1000000;
	rate /= (1<<20);

	printf ("Library: ");
	fflush (stdout);

	printf ("memcpy %g MB/sec \n", rate);

	free (a1);
	free (a2);
}

//----------------------------------------------------------------------------
// Name:	test_my_bzero
// Purpose:	Performs test on my own SSE2 version of bzero().
//----------------------------------------------------------------------------
void
test_my_bzero ()
{
	char *foo = malloc (16 * 1024 * 1024);
	unsigned long t = mytime ();
	int i;

#define MY_BZERO_LOOPS (777)
	for (i=0 ; i < MY_BZERO_LOOPS; i++)
		my_bzeroSSE2 (foo, 16 * 1024 * 1024);
	t = mytime () - t;
	double d = 16 * 1024 * 1024;
	d /= t;
	d *= MY_BZERO_LOOPS;
	d *= 1000000;
	d /= (1 << 20);

	printf ("Performance of my_bzeroSSE2: %g MB/sec\n", d);
}


//----------------------------------------------------------------------------
// Name:	main
//----------------------------------------------------------------------------
int
main (int argc, char **argv)
{
	FILE *f; 
	int i;
	struct stat st;
	bool use_sse2 = true;

	--argc;
	++argv;

	printf ("This is bandwidth version %s.\n", VERSION);
	printf ("Copyright (C) 2005,2007-2009 by Zack T Smith.\n\n");
	printf ("This software is covered by the GNU Public License.\n");
	printf ("It is provided AS-IS, use at your own risk.\n");
	printf ("See the file COPYING for more information.\n");

	puts ("");
#if __x86_64__
	if (use_sse2)
		printf ("Using 64- and 128-bit data transfers.\n");
	else
		printf ("Using 64-bit data transfers.\n");
#else
	if (use_sse2)
		printf ("Using 32- and 128-bit data transfers.\n");
	else
		printf ("Compiled for 32-bit data transfers.\n");
#endif

	//------------------------------------------------------------
	// Attempt to provide information about the CPU.
	//
	if (!stat ("/proc/cpuinfo", &st)) {
		if (system ("grep MHz /proc/cpuinfo | uniq | sed \"s/[\\t\\n: a-zA-Z]//g\" > /tmp/bandw_tmp"))
			perror ("system");

		f = fopen ("/tmp/bandw_tmp", "r");
		if (f) {
			if (1 == fscanf (f, "%g", &cpu_speed)) {
				float theoretical_max;

				puts ("");
				printf ("CPU speed is %g MHz.\n", cpu_speed);

				// Estimate L1 max.
				theoretical_max = cpu_speed * 1000000.0 * 8.0;
				theoretical_max /= 1.E6;
				printf ("L1 theoretical max is %g MB/second (one 64-bit access/cycle).\n", theoretical_max);
			}
			fclose (f);
			unlink ("/tmp/bandw_tmp");
		}
	} else {
		printf ("CPU information is not available (/proc/cpuinfo).\n");
	}

	//------------------------------------------------------------
	// Sequential reads.
	//
	puts ("");
	for (i = 8; i <= 24; i++) {
		unsigned long chunk_size = 1 << i;
		seq_read (chunk_size, false);
	}

	//------------------------------------------------------------
	// Sequential writes.
	//
	puts ("");
	for (i = 8; i <= 24; i++) {
		unsigned long chunk_size = 1 << i;
		seq_write (chunk_size, false);
	}

	//------------------------------------------------------------
	// SSE2 sequential reads.
	//
	if (use_sse2) {
		puts ("");
		for (i = 8; i <= 24; i++) {
			unsigned long chunk_size = 1 << i;
			seq_read (chunk_size, true);
		}
	}

	//------------------------------------------------------------
	// SSE2 sequential writes that bypass the caches.
	//
	if (use_sse2) {
		puts ("");
		for (i = 8; i <= 24; i+=4) {
			unsigned long chunk_size = 1 << i;
			seq_write (chunk_size, true);
		}
	}
#if 0
#if !defined(__CYGWIN__)
	//------------------------------------------------------------
	// Framebuffer read & write.
	//
	puts ("");
	fb_readwrite (true);
#endif
#endif

	//------------------------------------------------------------
	// C library performance.
	//
	puts ("");
	library_test ();

	//------------------------------------------------------------
	// My SSE2 routines.
	//
	puts ("");
	test_my_bzero();

	return 0;
}
