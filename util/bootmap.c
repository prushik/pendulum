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
	if (argc != 3)
	{
		write(2,"Usage:\n bootmap <kernel> <output>\n",34);
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

	int outfd = open(argv[2],O_WRONLY|O_CREAT,0);

	uint32_t block_sectors = block_size/512, n_sectors, sector, next_sector, base_sector;

	int block = 0;
	ret = ioctl(fd, FIBMAP, &block);
	if (ret < 0)
	{
		write(2,"FIBMAP ioctl failed. Permission denied or FIBMAP not supported.\n",64);
		close(fd);
		exit(EXIT_FAILURE);
	}

	base_sector = block * block_sectors;
	next_sector = base_sector+block_sectors;
	n_sectors = block_sectors;

	for (i=1; i < nr_blocks; i++)
	{
		block = i;
		ret = ioctl(fd, FIBMAP, &block);

		sector = block * block_sectors;

		char buffer[16];

		if (sector==next_sector)
			n_sectors+=block_sectors;
		else
		{
			write(outfd,&n_sectors,sizeof(n_sectors));
			write(outfd,&base_sector,sizeof(base_sector));
			hexstr(buffer,n_sectors,16);
			write(1,&buffer[8],8);
			write(1,"\t",1);
			hexstr(buffer,base_sector,16);
			write(1,buffer,16);
			write(1,"\n",1);
			n_sectors=block_sectors;
			base_sector=sector;
		}
		next_sector = sector+block_sectors;
	}

	write(outfd,&n_sectors,sizeof(n_sectors));
	write(outfd,&base_sector,sizeof(base_sector));

	char buffer[16];

	hexstr(buffer,n_sectors,16);
	write(1,&buffer[8],8);
	write(1,"\t",1);
	hexstr(buffer,next_sector-n_sectors,16);
	write(1,buffer,16);
	write(1,"\n",1);
	n_sectors=block_sectors;

	close(outfd);
	close(fd);
	exit(0);
	return 0;
}
