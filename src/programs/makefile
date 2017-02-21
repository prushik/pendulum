CC=gcc
LD=ld
AS=yasm
ASFLAGS=-felf64
CFLAGS=-fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone
LDFLAGS=-T app.ld

all: mkdir putpixel
	cp $^ ../../root/

mkdir: mkdir.o libpendulum.o
	$(LD) $(LDFLAGS) $^ -o $@

putpixel: putpixel.o libpendulum.o
	$(LD) $(LDFLAGS) $^ -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $<

%.o: %.c
	$(CC) -c $(CFLAGS) $<
