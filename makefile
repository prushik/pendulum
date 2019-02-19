#AS ?= nasm
AS ?= yasm
ASFLAGS ?=
SUDO ?= sudo 

all: src/pure64/pure64 src/kernel/kernel64

src/kernel/kernel64:
	make -C src/kernel

src/pure64/pure64:
	make -C src/pure64

util/bootmap:
	make -C util

programs:
	make -C src/programs

run: bin/ext2.img
	qemu-system-x86_64 -vga std -smp 8 -m 256 -drive id=disk,file=bin/ext2.img,if=none,format=raw -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0 -name "PendulumOS" -net nic,model=i82551

ext2: src/kernel/kernel64 src/pure64/pure64 src/pure64/ext2_mbr util/bootmap
	dd if=/dev/zero of=bin/ext2.img bs=1M count=256
	$(SUDO) mkfs.ext2 -r 0 bin/ext2.img
	dd if=src/pure64/ext2_mbr of=bin/ext2.img bs=512 conv=notrunc
	cat src/pure64/pure64 src/kernel/kernel64 > software
	mkdir -p bin/mp
	$(SUDO) mount -o loop bin/ext2.img bin/mp
	$(SUDO) mv software bin/mp/kernel
	$(SUDO) util/bootmap bin/mp/kernel map
	$(SUDO) cp -L root/* bin/mp/
	$(SUDO) umount bin/mp
	$(SUDO) chmod 666 map
	dd if=map of=bin/ext2.img bs=512 seek=1 conv=notrunc

clean:
	make -C src/kernel clean
	make -C src/pure64 clean
	rm map

%.o: %.s
	$(AS) $(ASFLAGS) %<
