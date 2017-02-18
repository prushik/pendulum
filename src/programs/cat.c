// Hello World C Test Program (v1.1, September 7 2010)
// Written by Ian Seyler
//
// BareMetal compile:
//
// GCC (Tested with 4.5.0)
// gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone -o helloc.o helloc.c
// gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone -o libBareMetal.o libBareMetal.c
// ld -T app.ld -o helloc.app helloc.o libBareMetal.o
//
// Clang (Tested with 2.7)
// clang -c -mno-red-zone -o libBareMetal.o libBareMetal.c
// clang -c -mno-red-zone -o helloc.o helloc.c
// ld -T app.ld -o helloc.app helloc.o libBareMetal.o


#include "libBareMetal.h"

int main()
{
	int argc = b_system_config(1,0);
	char *argv[argc];
	int i;
	for (i=0;i<argc;i++)
		argv[i] = b_system_config(2, i);

	int fd = b_file_open(argv[1]);

	char buffer[2097152];
	int len;
//	do
//	{
		len = b_file_read(fd,buffer,2097151);
//		b_output_chars(buffer,len);
//		b_file_seek(fd,len,1);
//	}
//	while (len);

	b_file_close(fd);

	if (len==512)
		b_output("READ 2mb\n");

//	b_output(argv[0]);
	b_output("\n");
	return 0;
}
