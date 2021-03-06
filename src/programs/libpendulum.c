// =============================================================================
// BareMetal -- a 64-bit OS written in Assembly for x86-64 systems
// Copyright (C) 2008-2016 Return Infinity -- see LICENSE.TXT
//
// The Pendulum C library code.
//
// Version 2.0
//
// This allows for a C/C++ program to access OS functions available in Pendulum
//
//
// Linux compile:
//
// Compile:
// gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone -o libBareMetal.o libBareMetal.c
// gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone -o yourapp.o yourapp.c
// Link:
// ld -T app.ld -o yourapp.app yourapp.o libBareMetal.o
//
// =============================================================================

#include "libpendulum.h"

/*
	For the record, the asm constraints here mean the following:
	* S - rsi
	* c - rcx
	* a - rax
	* D - rdi
	* d - rdx
*/

void b_output(const char *str)
{
	asm volatile ("call *0x00100010" : : "S"(str)); // Make sure source register (RSI) has the string address (str)
}

void b_output_chars(const char *str, unsigned long nbr)
{
	asm volatile ("call *0x00100018" : : "S"(str), "c"(nbr));
}


unsigned long b_input(unsigned char *str, unsigned long nbr)
{
	unsigned long len;
	asm volatile ("call *0x00100020" : "=c" (len) : "c"(nbr), "D"(str));
	return len;
}

unsigned char b_input_key(void)
{
	unsigned char chr;
	asm volatile ("call *0x00100028" : "=a" (chr));
	return chr;
}


unsigned long b_smp_enqueue(void *ptr, unsigned long var)
{
	unsigned long tlong;
	asm volatile ("call *0x00100030" : "=a"(tlong) : "a"(ptr), "S"(var));
	return tlong;
}

unsigned long b_smp_dequeue(unsigned long *var)
{
	unsigned long tlong;
	asm volatile ("call *0x00100038" : "=a"(tlong), "=D"(*(var)));
	return tlong;
}

void b_smp_run(unsigned long ptr, unsigned long var)
{
	asm volatile ("call *0x00100040" : : "a"(ptr), "D"(var));
}

void b_smp_wait(void)
{
	asm volatile ("call *0x00100048");
}


unsigned long b_mem_allocate(unsigned long *mem, unsigned long nbr)
{
	unsigned long tlong;
	asm volatile ("call *0x00100050" : "=a"(*(mem)), "=c"(tlong) : "c"(nbr));
	return tlong;
}

unsigned long b_mem_release(unsigned long *mem, unsigned long nbr)
{
	unsigned long tlong;
	asm volatile ("call *0x00100058" : "=c"(tlong) : "a"(*(mem)), "c"(nbr));
	return tlong;
}


void b_ethernet_tx(void *mem, unsigned long len)
{
	asm volatile ("call *0x00100060" : : "S"(mem), "c"(len));
}

unsigned long b_ethernet_rx(void *mem)
{
	unsigned long tlong;
	asm volatile ("call *0x00100068" : "=c"(tlong) : "D"(mem));
	return tlong;
}

unsigned long b_file_get_inode(unsigned long inode, struct kern_ext2_inode *buf)
{
	unsigned long tlong;
	asm volatile ("call *0x00100070" : "=a"(tlong) : "S"(inode),"D"(buf));
	return tlong;
}

unsigned long b_file_read(void *buf, unsigned long sector, struct kern_ext2_inode *i)
{
	unsigned long tlong;
	asm volatile ("call *0x00100088" : "=a"(tlong) : "d"(buf),"S"(sector),"D"(i));
	return tlong;
}

unsigned long b_file_open(unsigned long inode)
{
	unsigned long tlong;
	asm volatile ("call *0x00100078" : "=a"(tlong) : "S"(inode));
	return tlong;
}

unsigned long b_file_search(const char *fname, unsigned long rel_inode)
{
	unsigned long tlong;
	asm volatile ("call *0x00100078" : "=a"(tlong) : "a"(fname), "D"(rel_inode));
	return tlong;
}

//unsigned long b_file_close(unsigned long handle)
//{
//	unsigned long tlong = 0;
//	asm volatile ("call *0x00100080" : : "a"(handle));
//	return tlong;
//}

unsigned long b_file_write(unsigned long handle, const void *buf, unsigned int count)
{
	unsigned long tlong;
	asm volatile ("call *0x00100090" : "=c"(tlong) : "a"(handle), "S"(buf), "c"(count));
	return tlong;
}

/*
unsigned long b_file_create(const char *name, unsigned long size)
{
	unsigned long tlong;
	asm volatile ("call *0x001000F8" : : "S"(name), "c"(size));
	return tlong;
}

unsigned long b_file_delete(const unsigned char *name)
{
	unsigned long tlong;
	asm volatile ("call *0x00100108" : : "S"(name));
	return tlong;
}

unsigned long b_file_query(const unsigned char *name)
{
	unsigned long tlong;
	asm volatile ("call *0x00100118" : : "S"(name));
	return tlong;
}
*/


unsigned long b_system_config(unsigned long function, unsigned long var)
{
	unsigned long tlong;
	asm volatile ("call *0x001000B8" : "=a"(tlong) : "d"(function), "a"(var));
	return tlong;
}

void b_system_misc(unsigned long function, void* var1, void* var2)
{
	asm volatile ("call *0x001000C0" : : "d"(function), "a"(var1), "c"(var2));
}



// =============================================================================
// EOF
