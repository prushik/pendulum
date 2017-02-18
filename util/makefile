CC=gcc
AS=yasm
ASFLAGS=-felf64
CFLAGS=-fno-stack-protector
LDFLAGS=-nostartfiles -nostdlib 

all: bootmap

fibmap: amd64_syscalls.o crt.o fibmap.o
	$(CC) $(LDFLAGS) $^ -o $@

bootmap: amd64_syscalls.o crt.o bootmap.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $<

%.o: %.c
	$(CC) -c $(CFLAGS) $<
