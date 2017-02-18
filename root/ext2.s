; =============================================================================
; Pendulum -- a 64-bit exokernel OS
; Copyright (C) 2016 BetterOS.org -- see LICENSE.TXT
;
; EXT2 Functions
; =============================================================================

struc ext2_superblock
	.n_inodes			resd 1
	.n_blocks			resd 1
	.n_reserved_block	resd 1
	.n_unalloc_blocks	resd 1
	.n_unalloc_inodes	resd 1
	.superuser_block	resd 1
	.log_block_size		resd 1
	.log_fragment_size	resd 1
	.n_blocks_group		resd 1
	.n_frgments_group	resd 1
	.n_inodes_group		resd 1
	.last_mount			resd 1
	.last_write			resd 1

	.n_mounts			resw 1
	.n_max_mounts		resw 1
	.signature			resw 1
	.state				resw 1
	.error_proc			resw 1
	.version_minor		resw 1

	.check_time			resd 1
	.max_check_time		resd 1
	.os_created			resd 1
	.version_major		resd 1

	.uid_reserved		resw 1
	.gid_reserved		resw 1
endstruc

struc ext2_descriptor
	.block_use_bitmap	resd 1
	.inode_use_bitmap	resd 1
	.inode_table		resd 1
	.n_blocks			resw 1
	.n_inodes			resw 1
	.n_directories		resw 1
endstruc

struc ext2_inode
	.type				resw 1
	.uid				resw 1
	.l_size				resd 1
	.a_time				resd 1
	.c_time				resd 1
	.m_time				resd 1
	.d_time				resd 1
	.gid				resw 1
	.n_links			resw 1
	.n_sectors			resd 1
	.flags				resd 1
	.os_reserved1		resd 1
	.block_pointer		resd 12
	.single_indirect	resd 1
	.double_indirect	resd 1
	.triple_indirect	resd 1
	.generation			resd 1
	.xattr				resd 1
	.h_size				resd 1
	.frag_block			resd 1
	.os_reserved2		resd 3
endstruc

struc ext2_directory
	.inode				resd 1
	.size				resw 1
	.name_size			resb 1
	.type				resb 1
	.name				resb 1 ; this is actually ext2_directory.name_size bytes wide
endstruc

blocksize		dd 1024		; blocksize in byte
blocksectors	dd 2		; blocksize in sectors
desc_table		dd 0		; start of block group descriptor table
pwd				dd 2		; inode # of current directory

superblock: istruc ext2_superblock
iend
;resb 940
resb 428

descriptor: istruc ext2_descriptor
iend
resb 480

inode: istruc ext2_inode
iend
resb 384

align 16
db 'DEBUG: EXT2     '
align 16

ext2_blocksize:
	mov	eax, [superblock+ext2_superblock.log_block_size]
	mov	edx, 1024
	mov	ecx, eax
	sal	edx, cl
	mov	eax, edx
	ret;

; -----------------------------------------------------------------------------
; Initialize the EXT2 driver
init_ext2:
	push rdi
	push rdx
	push rcx
	push rax

	mov byte [pwd], 2

	cmp byte [os_DiskEnabled], 0x01
	jne init_ext2_nodisk

	; read superblock
	mov rax, 2			;Superblock starts at 1024
	mov rcx, 1			;and spans 1024 bytes (sort of... only 84-128 are used)
	;we only read 512 cause we only need 84 bytes
	xor edx,edx
	lea rdi,[superblock]
	call readsectors

	; below is modified compiled code
	; from:
	;	int descriptor_table = (4%(2<<blocksize))+4;
	mov	eax, DWORD [superblock+ext2_superblock.log_block_size]
	mov	edx, 2		;1024 bytes in sectors
	mov	ecx, eax
	sal	edx, cl
	mov [blocksectors], edx		;remember sectors per block
	mov	eax, edx
	sub	eax, 1
	and	eax, 4		;2048 in sectors
	add	eax, 4		;2048 in sectors
	; eax should contain the start of the descriptor table
	mov	[desc_table], eax

;	mov eax,[superblock+ext2_superblock.n_blocks]
	; Here we neeed to sanity check (correct number of inodes and blocks, no fs errors, no compression/type/journal)

init_ext2_nodisk:

	pop rax
	pop rcx
	pop rdx
	pop rdi
	ret
; -----------------------------------------------------------------------------


show_super:
	push rsi
	push rax
	push rdx
	mov rsi,[blocksize]
	call write_decimal
	mov rsi,[blocksectors]
	call write_decimal
	mov rsi,[superblock+ext2_superblock.n_inodes]
	call write_decimal
	mov rsi,[superblock+ext2_superblock.n_blocks]
	call write_decimal
	mov rsi,[superblock+ext2_superblock.n_blocks_group]
	call write_decimal
	mov rsi,[superblock+ext2_superblock.n_inodes_group]
	call write_decimal
	pop rdx
	pop rax
	pop rsi
	ret

;------------------------------------------------------------------------------
; IN:	rsi = inode number
; OUT:	rax = inode address
; 
;
os_ext2_find_inode:
	push rsi
	push rdx
	push rdi
	push rcx
	push r8

	; rax = group of inode = (inode_number-1) / inodes_per_group
	; rdx = inode in group = (inode_number-1) % inodes_per_group
	sub	rsi,1
	mov rax, rsi
	xor	edx, edx
	div DWORD [superblock+ext2_superblock.n_inodes_group];

	mov rsi,rdx ; save inodeN_in_group

	; rax = sector of group = group/16
	; rdx = group in sector = group%16
	; rsi = inode in group
	xor	edx, edx
	mov rdi,16
	div edi		; 16 is 512/32 = number of groups per sector

	add eax,DWORD[desc_table] ; rax = sector containing blockgroup descriptor

	; read sector containing group
	; rax = sector of n
	mov rcx,1		; one sector
	mov r8,rdx
	xor rdx,rdx		; disk 0
	lea rdi,[descriptor]
	call readsectors

	; rax = memory location of correct group descriptor = group in sector *32 + descriptor
	; rsi = inode in group
	mov eax,32 
	mul r8d

	add rax,descriptor

	; rax = sector of inode table
	mov r8,rsi
	mov rsi,[rax+ext2_descriptor.inode_table]
	xor rdx,rdx
	mov eax,[blocksectors]
	mul esi

	;need to divide inode in group by 4(inodes per sector)
	;then get that sector and use remainder for inode in sector
	;but as long as we only need inodes 1,2,3, and 4, we can skip this
	mov rcx,rax
	mov rax,r8
	mov rsi,4
	xor rdx,rdx
	div esi
	add rax,rcx
	mov r8,rdx
	; edx should be inode in sector

	mov rcx,1
	xor rdx,rdx
	lea rdi,[inode]
	call readsectors

	; multiply by size of inode struct to get the correct inode
	mov rdx,r8
	mov eax,128
	mul r8

	add rax,inode

;	push rsi
;	push rax
;	push rdx
;	mov rsi,rax
;	mov esi,DWORD[rax+ext2_inode.block_pointer]
;	call write_decimal
;	pop rdx
;	pop rax
;	pop rsi

	pop r8
	pop rcx
	pop rdi
	pop rdx
	pop rsi
	ret

;------------------------------------------------------------------------------
; read a block from an inode
; rdi = memory address of inode structure
; rsi = block within inode to read
; rdx = memory to store
os_ext2_inode_block_read:
	push rdi
	push rdx
	push rax
	push rsi
	push rcx
	push r8

	add rdi,ext2_inode.block_pointer

	mov r8,rdx

	mov rax,rsi
	shl rax,2

	add rdi,rax ; this should now be the correct pointer
	mov eax,DWORD[rdi]

	;eax should now contain the block address
	mov ecx,DWORD[blocksectors]
	mul ecx

	mov rdi,r8

	;eax should now contain the sector of the block
	;rcx should already be number of sectors
	xor rdx,rdx
	; rdi should already be set
	call readsectors

	pop r8
	pop rcx
	pop rsi
	pop rax
	pop rdx
	pop rdi
	ret


;------------------------------------------------------------------------------
; read a sector from an inode
; rdi = memory address of inode structure
; rsi = sector within inode to read
; rdx = memory to store
os_ext2_inode_sector_read:
	push rdi
	push rdx
	push rax
	push rsi
	push rcx
	push r8

	add rdi,ext2_inode.block_pointer

	mov r8,rdx

	mov rax,rsi
	xor rdx,rdx
	div DWORD[blocksectors]

	mov r9,rdx

	mov esi,4
	mul esi

	add rdi,rax ; this should now be the correct pointer (unless its >12)
	mov eax,DWORD[rdi]

	;eax should now contain the block address
	mov ecx,DWORD[blocksectors]
	mul ecx

	add rax, r9	; sector to read
	mov rcx, 1	; read 1 sector
	xor rdx,rdx
	mov rdi,r8	; memory to store
	call readsectors

	pop r8
	pop rcx
	pop rsi
	pop rax
	pop rdx
	pop rdi
	ret

;------------------------------------------------------------------------------
; read a file in its entirity
; rdi = memory address of inode structure
; rdx = memory to store
os_ext2_file_load:
	push rdi
	push rdx
	push rax
	push rsi
	push rcx
	push r8

	mov rcx,[rdi+ext2_inode.n_sectors]
	xor rsi,rsi ; start reading at sector 0

load_loop:

	call os_ext2_inode_sector_read

	add rdx,512	; advance memory pointer to next location
	inc rsi		; advance sector number
	cmp rsi,rcx ; make sure we still have data to read
	jl load_loop; read more if not finished

load_done:
	pop r8
	pop rcx
	pop rsi
	pop rax
	pop rdx
	pop rdi
	ret

;------------------------------------------------------------------------------
; read data from an inode
; rdi = memory address of inode structure
; rsi = offset of file to read
; rdx = memory to store
; rcx = bytes to read
os_ext2_inode_data_read:
	push rdi
	push rdx
	push rax
	push rsi
	push rcx
	push r8

	mov r8,rdx
	mov r9,rdi

	mov rax,rsi
	xor rdx,rdx
	div DWORD [blocksize] ;divide offset by blocksize to get start block
	; rdx = start offset in block | rax = start block of inode

	mov r10,rdx
	mov rsi,rax

	lea rdx,[directory]
	call os_ext2_inode_block_read

	add rdx,r10 ; now rdx should be the place where we wanted data to start
	mov rdi,rsi
	mov rsi,rdx
	mov rdx,[blocksize]
	sub rdx,r10
	call os_memcpy ; copy the data to caller supplied buffer

	add r10,rax ; r10 is now the next location to read to!

	inc rdi ; next block

;	sub rcx
	jz ri_done

	

	mov eax,4
	mul esi

	add rdi,rax ; this should now be the correct pointer
	mov eax,DWORD[rdi]

	;eax should now contain the block address
	mov ecx,DWORD[blocksectors]
	mul ecx

	mov rdi,r8

	;eax should now contain the sector of the block
	;rcx should already be number of sectors
	xor rdx,rdx
	; rdi should already be set
	call readsectors

ri_done:
	pop r8
	pop rcx
	pop rsi
	pop rax
	pop rdx
	pop rdi
	ret


;------------------------------------------------------------------------------
; basically read the whole file. this is needed for loading executables
; rdi = inode in memory
; rdx = location to store
;
os_ext2_load_file:
	mov rcx,[rdi+ext2_inode.n_sectors]
	mov rbx,[rdi+ext2_inode.block_pointer]


;------------------------------------------------------------------------------
; rdi = memory location of inode
; reads a directory
; only if all dir is in one block
os_ext2_list_directory:
	push rax
	push rsi
	push rdx
	push rcx

	mov rsi,0
	lea rdx,[directory]
	call os_ext2_inode_block_read

	lea rbx,[directory]
	mov ecx,[rdi+ext2_inode.l_size]
	add rbx,rcx ; end address

	lea rdx,[directory] ; start address

.dir_list_loop:
	mov rsi,rdx
	mov rax,rdx
	xor rcx,rcx
	mov	cl, BYTE [rsi+ext2_directory.name_size]
	add rsi,ext2_directory.name
	call os_output_chars

	call os_print_newline

	xor rdx,rdx
	mov dx, WORD [rax+ext2_directory.size]
	add rax, rdx
	mov rdx, rax

	cmp rbx,rdx
	jg .dir_list_loop

	pop rcx
	pop rdx
	pop rsi
	pop rax
	ret

;------------------------------------------------------------------------------
; rdi = memory location of inode (dir to search)
; rax = addr of file/dir name to find
; out: rax = inode # or 0 if not found
; reads a directory
; only if all dir is in one block
os_ext2_search_directory:
	push rsi
	push rdx
	push rcx

	mov r8,rax ; save fname

	mov rsi,0
	lea rdx,[directory]
	call os_ext2_inode_block_read

	lea rbx,[directory]
	mov ecx,[rdi+ext2_inode.l_size]
	add rbx,rcx ; end address

	lea rdx,[directory] ; start address

.dir_search_loop:
	mov rdi,rdx
	mov rax,rdx
	xor rcx,rcx
	mov	cl, BYTE [rdi+ext2_directory.name_size]
	add rdi,ext2_directory.name

	mov rsi,r8
	call os_string_compare_part
	jnc .continue
	mov eax,DWORD[rax+ext2_directory.inode]
	jmp .found
.continue:

	xor rdx,rdx
	mov dx, WORD [rax+ext2_directory.size]
	add rax, rdx
	mov rdx, rax

	cmp rbx,rdx
	jg .dir_search_loop
	jmp .notfound

.notfound:
	mov rax,0 ; not found

.found:

	pop rcx
	pop rdx
	pop rsi
	ret



;------------------------------------------------------------------------------
; rdi = memory location of inode
; prints informations about a file
os_ext2_file_stat:
	push rdi
	push rsi
	push rax

	mov rsi,msg_stat_lsze
	call os_output
	mov esi,[rdi+ext2_inode.l_size]
	call write_decimal

	mov rsi,msg_stat_sect
	call os_output
	mov esi,[rdi+ext2_inode.n_sectors]
	call write_decimal

	mov rsi,msg_stat_strt
	call os_output
	mov esi,[rdi+ext2_inode.block_pointer]
	call write_decimal

	mov rsi,msg_stat_type
	call os_output

	xor rsi,rsi
	mov si,[rdi+ext2_inode.type]
	and si,0xF000
	cmp si,0x4000
	je stat_isdir

stat_isfile:
	mov rsi,msg_stat_type_file
	call os_output
	jmp stat_done

stat_isdir:
	mov rsi,msg_stat_type_dir
	call os_output

stat_done
	call os_print_newline

	pop rax
	pop rsi
	pop rdi
	ret



; -----------------------------------------------------------------------------
; os_file_open -- Open a file on disk
; IN:	RSI = File name (zero-terminated string)
; OUT:	RAX = File I/O handler, 0 on error
;	All other registers preserved
os_ext2_file_open:
	push rsi
	push rdx
	push rcx
	push rbx

	pop rbx
	pop rcx
	pop rdx
	pop rsi
	ret
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; os_file_close -- Close an open file
; IN:	RAX = File I/O handler
; OUT:	All registers preserved
os_ext2_file_close:
	push rsi
	push rax

	pop rax
	pop rsi
	ret
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; os_ext2_file_read -- Read a number of bytes from a file
; IN:	RAX = File I/O handler
;	RCX = Number of bytes to read (automatically rounded up to next 2MiB)
;	RDI = Destination memory address
; OUT:	RCX = Number of bytes read
;	All other registers preserved
os_ext2_file_read:
	push rdi
	push rsi
	push rdx
	push rcx
	push rbx
	push rax

	pop rax
	pop rbx
	pop rcx
	pop rdx
	pop rsi
	pop rdi
	ret
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; os_ext2_file_write -- Write a number of bytes to a file
; IN:	RAX = File I/O handler
;	RCX = Number of bytes to write
;	RSI = Source memory address
; OUT:	RCX = Number of bytes written
;	All other registers preserved
os_ext2_file_write:
	push rdi
	push rsi
	push rdx
	push rcx
	push rbx
	push rax


	pop rax
	pop rbx
	pop rcx
	pop rdx
	pop rsi
	pop rdi
	ret
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; os_ext2_file_seek -- Seek to position in a file
; IN:	RAX = File I/O handler
;	RCX = Number of bytes to offset from origin
;	RDX = Origin
; OUT:	All registers preserved
os_ext2_file_seek:
	; Is this an open file?

	ret
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; os_ext2_file_internal_query -- Search for a file name and return information
; IN:	RSI = Pointer to file name
; OUT:	RAX = Staring block number
;	RBX = Offset to entry
;	RCX = File size in bytes
;	RDX = Reserved blocks
;	Carry set if not found. If carry is set then ignore returned values
os_ext2_file_internal_query:
	push rdi

	pop rdi
	ret
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; os_ext2_file_query -- Search for a file name and return information
; IN:	RSI = Pointer to file name
; OUT:	RCX = File size in bytes
;	Carry set if not found. If carry is set then ignore returned values
os_ext2_file_query:
	push rdi

	pop rdi
	ret
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; os_ext2_file_create -- Create a file on the hard disk
; IN:	RSI = Pointer to file name, must be <= 32 characters
;	RCX = File size to reserve (rounded up to the nearest 2MiB)
; OUT:	Carry clear on success, set on failure
; Note:	This function pre-allocates all blocks required for the file
os_ext2_file_create:

	; Flush directory to disk

	ret
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; os_ext2_file_delete -- Delete a file from the hard disk
; IN:	RSI = File name to delete
; OUT:	Carry clear on success, set on failure
os_ext2_file_delete:
	push rdx
	push rcx
	push rbx
	push rax

	pop rax
	pop rbx
	pop rcx
	pop rdx
	ret
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; os_ext2_block_read -- Read a number of blocks into memory
; IN:	RAX = Starting block #
;	RCX = Number of blocks to read
;	RDI = Memory location to store blocks
; OUT:
os_ext2_block_read:

	ret
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; os_ext2_block_write -- Write a number of blocks to disk
; IN:	RAX = Starting block #
;	RCX = Number of blocks to write
;	RSI = Memory location of blocks to store
; OUT:
os_ext2_block_write:

	ret
; -----------------------------------------------------------------------------



	msg_stat_lsze:		db "Size: ",0
	msg_stat_sect:		db "Sectors: ",0
	msg_stat_strt:		db "Start Block: ",0
	msg_stat_type:		db "Type: ",0
	msg_stat_type_file:	db "file",0
	msg_stat_type_dir:	db "directory",0
; =============================================================================
; EOF
