; =============================================================================
; BareMetal -- a 64-bit OS written in Assembly for x86-64 systems
; Copyright (C) 2008-2016 Return Infinity -- see LICENSE.TXT
;
; Initialization Includes
; =============================================================================

align 16
db 'DEBUG: INIT     '
align 16


%include "init/64.s"
%include "init/hdd.s"
%include "init/net.s"
%include "init/pci.s"


; =============================================================================
; EOF
