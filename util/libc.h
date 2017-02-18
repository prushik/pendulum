//If no standard C library is to be used, include only linux kernel headers
#ifndef HAVE_LIBC
	//sys/syscall.h is a glibc header, so it needs to be replaced somehow
	#include <sys/syscall.h>		//Syscall numbers
	#include <asm/unistd.h>
	#include <asm/fcntl.h>
	#include <asm/termios.h>
#endif
//If we want to use a standard C library, use it's headers
#ifdef HAVE_LIBC
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <fcntl.h>
#endif

#include <sys/types.h>				//uint64_t
#include <sys/socket.h>				//Socket related constants
#include <sys/un.h>					//Unix domain constants
#include <netinet/in.h>				//Socket related constants
#include <sys/stat.h>				//stat structure
#include <sys/mman.h>				//mmap constants

//Although PATH_MAX has its limitations, it can still be useful
#ifdef __linux__
	#include <linux/limits.h>		//PATH_MAX
#else
	#define PATH_MAX 4096
#endif


//Use the preprocessor to change libc functions to syscalls through <arch>-syscalls.s
#ifndef HAVE_LIBC
	uint64_t syscall0(int);
	uint64_t syscall1(int, ...);
	uint64_t syscall2(int, ...);
	uint64_t syscall3(int, ...);
	uint64_t syscall4(int, ...);
	uint64_t syscall5(int, ...);
	uint64_t syscall6(int, ...);

	#define open(x,y,z)					syscall3(SYS_open, x, y, z)
	#define close(x)					syscall3(SYS_close, x)
	#define read(x,y,z)					syscall3(SYS_read, x, y, z)
	#define write(x,y,z)				syscall3(SYS_write, x, y, z)
	#define lseek(x,y,z)				syscall3(SYS_lseek, x, y, z)
	#define fstat(x,y)					syscall2(SYS_fstat, x, y)
	#define fadvise64(w,x,y,z)			syscall4(SYS_fadvise64, w, x, y, z);
	#define chdir(x)					syscall1(SYS_chdir, x)
	#define fork()						syscall0(SYS_fork)
	#define exit(x)						syscall1(SYS_exit, x)
	#define sbrk(x)						(void *)(syscall1(SYS_brk, syscall1(SYS_brk, 0) + x) - x)
	#define brk(x)						(((void *)syscall1(SYS_brk, x) == x)-1)
	#define mmap(a,b,c,x,y,z)			syscall6(SYS_mmap, a, b, c, x, y, z)
	#define uname(x)					syscall1(SYS_uname, x)
	#define sethostname(x,y)			syscall1(SYS_sethostname, x, y)
	#define socket(x,y,z)				syscall3(SYS_socket, x, y, z)
	#define bind(x,y,z)					syscall3(SYS_bind, x, y, z)
	#define listen(x,y)					syscall2(SYS_listen, x, y)
	#define accept(x,y,z)				syscall3(SYS_accept, x, y, z)
	#define connect(x,y,z)				syscall3(SYS_connect, x, y, z)
	#define ioctl(x,y,z)				syscall3(SYS_ioctl, x, y, z)
	#define getuid()					syscall0(SYS_getuid)
	#define getgid()					syscall0(SYS_getgid)
	#define chown(x,y,z)				syscall3(SYS_chown, x, y, z)
	#define fchown(x,y,z)				syscall3(SYS_fchown, x, y, z)
	#define inotify_init()				syscall0(SYS_inotify_init)
	#define inotify_add_watch(x,y,z)	syscall3(SYS_inotify_add_watch, x, y, z)
	#define htons(x)					((uint16_t)((((x) >> 8) & 0xffu) | (((x) & 0xffu) << 8)))
	#define setsid()		syscall0(SYS_setsid)
	#define dup2(x,y)		syscall2(SYS_dup2,x,y)
	#define execve(x,y,z)	syscall3(SYS_execve,x,y,z)
	#define poll(x,y,z)		syscall3(SYS_poll,x,y,z)


	#define EOF (-1)
#endif

