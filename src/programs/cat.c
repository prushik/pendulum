// Hello World C Test Program (v1.1, September 7 2010)
// Written by Ian Seyler
//
// BareMetal compile:
//
// GCC (Tested with 4.5.0)
// gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone -o helloc.o helloc.c
// gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone -o libpendulum.o libpendulum.c
// ld -T app.ld -o helloc.app helloc.o libpendulum.o


#include "libpendulum.h"

int main()
{
	int argc = b_system_config(1,0);
	char *argv[argc];
	int i;
	for (i=0;i<argc;i++)
		argv[i] = b_system_config(2, i);

	b_output_chars("Opening file: ", 14);
	b_output(argv[1]);

	struct kern_ext2_inode fi;

	int fd = b_file_search(argv[1], 2); // 2 is the ext2 root
	b_file_get_inode(fd, &fi);

	char secs = '0'+fi.n_sectors;
	b_output_chars("\n\n\nfile sectors: ", 17);
	b_output_chars(&secs, 1);
	b_output_chars("\n", 1);

	char buffer[2097152];
	int len;
//	do
//	{
//		len = b_file_read(fd,buffer,2097151);
//		b_output_chars(buffer,len);
//		b_file_seek(fd,len,1);
//	}
//	while (len);

	b_file_close(fd);

//	if (len==512)
//		b_output("READ 2mb\n");

//	b_output(argv[0]);
	b_output("\n");
	return 0;
}
