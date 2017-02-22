// =============================================================================
// Pendulum -- a 64-bit OS written in Assembly for x86-64 systems
// Copyright (C) 2017 BetterOS.org -- see LICENSE.TXT
//
// The Pendulum C library header.
//
// Version 1.0
//
// This allows for a C program to access OS functions available in Pendulum
// =============================================================================
#ifndef _PENDULUM_H
#define _PENDULUM_H 1

struct kern_ext2_inode
{
	unsigned short	type;
	unsigned short	uid;
	unsigned int	l_size,
					a_time,
					c_time,
					m_time,
					d_time;
	unsigned short	gid;
	unsigned short	n_links;
	unsigned int	n_sectors;
	unsigned int	flags;
	unsigned int	os_reserved1;
	unsigned int	block_pointer[12];
	unsigned int	single_indirect;
	unsigned int	double_indirect;
	unsigned int	triple_indirect;
	unsigned int	generation;
	unsigned int	xattr;
	unsigned int	h_size;
	unsigned int	frag_block;
	unsigned int	os_reserved2[3];
};

void b_output(const char *str);
void b_output_chars(const char *str, unsigned long nbr);

unsigned long b_input(unsigned char *str, unsigned long nbr);
unsigned char b_input_key(void);

unsigned long b_smp_enqueue(void *ptr, unsigned long var);
unsigned long b_smp_dequeue(unsigned long *var);
void b_smp_run(unsigned long ptr, unsigned long var);
void b_smp_wait(void);

unsigned long b_mem_allocate(unsigned long *mem, unsigned long nbr);
unsigned long b_mem_release(unsigned long *mem, unsigned long nbr);

void b_ethernet_tx(void *mem, unsigned long len);
unsigned long b_ethernet_rx(void *mem);

unsigned long b_file_get_inode(unsigned long inode, struct kern_ext2_inode *buf);
//unsigned long b_file_get_inode(unsigned long inode, const unsigned char *name);
unsigned long b_file_open(unsigned long inode);
unsigned long b_file_close(unsigned long handle);
unsigned long b_file_read(unsigned long handle, void *buf, unsigned int count);
unsigned long b_file_write(unsigned long handle, const void *buf, unsigned int count);

/*
unsigned long b_file_seek(unsigned long handle, unsigned int offset, unsigned int whence);
unsigned long b_file_query(const unsigned char *name);
unsigned long b_file_create(const char *name, unsigned long size);
unsigned long b_file_delete(const unsigned char *name);
*/

unsigned long b_system_config(unsigned long function, unsigned long var);
void b_system_misc(unsigned long function, void* var1, void* var2);


// Index for b_system_config calls
#define timecounter 0
#define config_argc 1
#define config_argv 2
#define networkcallback_get 3
#define networkcallback_set 4
#define clockcallback_get 5
#define clockcallback_set 6
#define statusbar 10


// Index for b_system_misc calls
#define smp_get_id 1
#define smp_lock 2
#define smp_unlock 3
#define debug_dump_mem 4
#define debug_dump_rax 5
#define get_argc 6
#define get_argv 7

#endif
