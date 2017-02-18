Pendulum 
==========================

Pendulum is a fork of BareMetal OS, replacing the problematic parts of the operating system with more reasonable ones.
Pendulum runs on the ext2 file system instead of the very limited and difficult to work with BMFS.
Pendulum's interface has also been improved, commands are more remiscent of Unix commands instead of with DOS-like commands in BareMetal.
Pendulum's boot strategy has been significantly improved. The kernel can now be installed as a regular file in the file system, and the build system will create code to load the kernel correctly.
Pendulum has a redesigned build system based on simple makefiles, making compilation as simple as typing make.


Prerequisites
-------------

In order to build Pendulum, the following host software is required:
	make - The Pendulum build system uses make
	yasm - Most of Pendulum is built with yasm
	nasm - unfortunately, nasm is still required to build part of the bootloader, PURE64, due to features which yasm does not yet support
	gcc (or another C compiler) - Although not required for building Pendulum itself, some host utilities (e.g. bootmap) are written in C
	Linux - Required for running bootmap (needed to create bootable image)


Building the source code
--------------------------

	make


Creating disk image
----------------------------

	make ext2


Test the image with Qemu
--------------------------

	make run


Using Pendulum 
--------------------

Once Pendulum finishes booting, you will presented with a prompt ('>'), at this prompt, you can type the following built-in commands:
	help - displays a list of available built-in commands
	ver - Display Pendulum version
	ls - list contents of current directory
	clear - clear screen
	stat <file> - display information about file 
	cat <file> - display contents of text file
	reboot - reboot computer

If a command is not recognized as a built-in command, Pendulum will search the current directory for a file with the given name and attempt to execute it
