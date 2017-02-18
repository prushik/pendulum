; =============================================================================
; BareMetal -- a 64-bit OS written in Assembly for x86-64 systems
; Copyright (C) 2008-2016 Return Infinity -- see LICENSE.TXT
;
; System Call Section -- Accessible to user programs
; =============================================================================

align 16
db 'DEBUG: SYSCALLS '
align 16


%include "syscalls/debug.s"
%include "syscalls/ethernet.s"
%include "syscalls/file.s"
%include "syscalls/input.s"
%include "syscalls/memory.s"
%include "syscalls/misc.s"
%include "syscalls/screen.s"
%include "syscalls/smp.s"
%include "syscalls/string.s"


; =============================================================================
; EOF
