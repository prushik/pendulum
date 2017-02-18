#include "libc.h"
#include <linux/fs.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void hexstr(char *str, uint64_t num, uint8_t len)
{
	char ascii_hex[] = "0123456789abcdef";
	int i;
	for (i=len-1;i>0;i--)
	{
		str[i]=ascii_hex[num&0x0F];
		num = num>>4;
	}
}

int main(int argc, char **argv)
{
	struct stat si;
	int fd, nr_blocks, block_size, i, ret, errno;
	if (argc != 2)
	{
		char buffer[16];
		hexstr(buffer,0x012345679abcdef,16);
		write(1,buffer,16);
		write(1,"\n",1);
		write(2,"Usage:\n fibmap -k <kernel>\n",27);
		exit(EXIT_FAILURE);
	}
	fd = open(argv[1], O_RDONLY, 0);
	if (fd < 0)
	{
		write(2,"Cannot open file. Does it exist?\n",33);
		exit(EXIT_FAILURE);
	}
	ret = ioctl(fd, FIGETBSZ, &block_size);
	if (ret < 0)
	{
		write(2,"Failed to get blocksize. That's weird.\n",39);
		close(fd);
		exit(EXIT_FAILURE);
	}
	ret = fstat(fd, &si);
	if (ret < 0)
	{
		write(2,"Cannot stat file. Does it exist?\n",33);
		close(fd);
		exit(EXIT_FAILURE);
	}
	nr_blocks = (si.st_size + block_size - 1) / block_size;
//	printf("%s: %d %d-byte blocks\n", argv[1], nr_blocks, block_size);
	for (i=0; i < nr_blocks; i++)
	{
		int block = i;
		ret = ioctl(fd, FIBMAP, &block);
		if (ret < 0)
		{
			write(2,"FIBMAP ioctl failed. Permission denied or FIBMAP not supported.\n",64);
			close(fd);
			exit(EXIT_FAILURE);
		}
		char buffer[16];
		hexstr(buffer,block,16);
		write(1,buffer,16);
		write(1,"\n",1);
	}
	close(fd);
	return 0;
}
