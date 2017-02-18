#AS=nasm
AS=yasm
ASFLAGS=

all: ext2_mbr pure64

# yasm complains about segment registers in 64-bit mode. Not sure if it affects result
pure64: pure64.s
	nasm pure64.s -o pure64

pxestart:
	$(AS) bootsectors/pxestart.s -o pxestart

# deprecated
#bmfs_mbr:
#	$(AS) bootsectors/bmfs_mbr.s -o bmfs_mbr

ext2_mbr: bootsectors/ext2_mbr.s
	$(AS) bootsectors/ext2_mbr.s -o ext2_mbr

clean:
	rm ext2_mbr pure64

%.o: %.s
	$(AS) $(ASFLAGS) %<
