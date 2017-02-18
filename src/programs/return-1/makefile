PREFIX?=/usr/local

Return: main.c math.s amd64_syscalls.s crt.s
	gcc main.c -c -mtune=core2 -msse -msse2 -msse3 -msse4.1 -O3
	yasm -felf64 math.s
	yasm -felf64 crt.s
	yasm -felf64 amd64_syscalls.s
	ld main.o crt.o math.o amd64_syscalls.o -O3 -o $@
	strip $@

Return_debug: main.c math.s amd64_syscalls.s crt.s
	gcc main.c -c -mtune=core2 -g
	yasm -felf64 math.s
	yasm -felf64 crt.s
	yasm -felf64 amd64_syscalls.s
	ld main.o crt.o math.o amd64_syscalls.o -o $@

Return_libc: main.c amd64_syscalls.s
	yasm -felf64 amd64_syscalls.s
	gcc main.c -mtune=core2 -msse -msse2 -msse3 -msse4.1 -D"sgn(x)=x<0?-1:1" amd64_syscalls.o -static -O3 -lm -o $@
	strip $@

install: Return
	install -D Return $(PREFIX)/games/

clean:
	rm Return
	rm *.o
