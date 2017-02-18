;crt.s v1
;
;This is the C runtime for programs that do not need the C library. It 
;makes arguments available and aligns the stack, then calls main directly
;
;This is the first version
;Todo: Make environment variables available

section .text
	global _start
	extern main

	_start:
		xor		rbp,rbp			;AMD64 ABI Requirement
		pop		rdi				;Get argument count
		mov		rsi,rsp			;Get argument array
		and		rsp,-16			;Align stack pointer
		call	main			;Execute C code

								;End Program
		mov 	rax,60			;SYS_exit
		syscall					;

		ret						;Never Reached
