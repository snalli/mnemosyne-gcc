/*!
 * \file
 * A self-contained program which prints the names of ELF sections per a given ELF filename.
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
        int fd;
        Elf *e;
        char *name, *p, pc[4*sizeof(char)];
        Elf_Scn *scn;
        Elf_Data *data;
        GElf_Shdr shdr;
        size_t n, shstrndx, sz;
		Elf64_Ehdr *ehdr;

        if (argc != 2)
                errx(EX_USAGE, "usage: %s file-name", "test");

        if (elf_version(EV_CURRENT) == EV_NONE)
                errx(EX_SOFTWARE, "ELF library initialization failed: %s",
                    elf_errmsg(-1));

        if ((fd = open(argv[1], O_RDONLY, 0)) < 0)
                err(EX_NOINPUT, "open \%s\" failed", argv[1]);

        if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
                errx(EX_SOFTWARE, "elf_begin() failed: %s.",
                    elf_errmsg(-1));

        if (elf_kind(e) != ELF_K_ELF)
                errx(EX_DATAERR, "%s is not an ELF object.", argv[1]);

	    ehdr = elf64_getehdr(e);
        //if (elf_getshstrndx(e, &shstrndx) == 0)
        //        errx(EX_SOFTWARE, "getshstrndx() failed: %s.", elf_errmsg(-1));
		shstrndx = ehdr->e_shstrndx;

        scn = NULL;
        while ((scn = elf_nextscn(e, scn)) != NULL) {
                if (gelf_getshdr(scn, &shdr) != &shdr)
                        errx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));

                if ((name = elf_strptr(e, shstrndx, shdr.sh_name)) == NULL)
                        errx(EX_SOFTWARE, "elf_strptr() failed: %s.", elf_errmsg(-1));

                (void) printf("Section %-4.4jd %s %x %d\n", (uintmax_t) elf_ndxscn(scn),
                    name, shdr.sh_addr, shdr.sh_size);
        }

        if ((scn = elf_getscn(e, shstrndx)) == NULL)
                errx(EX_SOFTWARE, "getscn() failed: %s.",
                    elf_errmsg(-1));

        if (gelf_getshdr(scn, &shdr) != &shdr)
                errx(EX_SOFTWARE, "getshdr(shstrndx) failed: %s.",
                    elf_errmsg(-1));

        (void) printf(".shstrab: size=%jd\n", (uintmax_t) shdr.sh_size);

        data = NULL; n = 0;
        while (n < shdr.sh_size && (data = elf_getdata(scn, data)) != NULL) {
                p = (char *) data->d_buf;
                while (p < (char *) data->d_buf + data->d_size) {
                        n++; p++;
                        (void) putchar((n % 16) ? ' ' : '\n');
                }
        }
        (void) putchar('\n');

        (void) elf_end(e);
        (void) close(fd);
        exit(EX_OK);
}
