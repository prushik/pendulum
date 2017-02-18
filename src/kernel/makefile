#AS=nasm
AS=yasm
ASFLAGS=

all: kernel64

kernel64: kernel64.s cli.s drivers.s font.s init.s interrupt.s syscalls.s sysvar.s
	$(AS) kernel64.s -o kernel64

clean:
	rm kernel64

%.o: %.s
	$(AS) $(ASFLAGS) %<
