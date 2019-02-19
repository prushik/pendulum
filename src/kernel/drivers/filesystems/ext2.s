; =============================================================================
; Pendulum -- a 64-bit exokernel OS
; Copyright (C) 2016-2019 BetterOS.org -- see LICENSE.TXT
;
; EXT2 Functions
; =============================================================================

align 16
db 'DEBUG: EXT2     '
align 16

ext2_blocksize:
	mov	eax, [fs_superblock+ext2_superblock.log_block_size]
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

	mov byte [fs_pwd], 2

	cmp byte [os_DiskEnabled], 0x01
	jne init_ext2_nodisk

	; read superblock
	mov rax, 2			;Superblock starts at 1024
	mov rcx, 1			;and spans 1024 bytes (sort of... only 84-128 are used)
	;we only read 512 cause we only need 84 bytes
	xor edx,edx
	lea rdi,[fs_superblock]
	call readsectors

	; below is modified compiled code
	; from:
	;	int descriptor_table = (4%(2<<blocksize))+4;
	mov	eax, DWORD [fs_superblock+ext2_superblock.log_block_size]
	mov	edx, 2		;1024 bytes in sectors
	mov	ecx, eax
	sal	edx, cl
	mov [fs_blocksectors], edx		;remember sectors per block
	mov	eax, edx
	sub	eax, 1
	and	eax, 4		;2048 in sectors
	add	eax, 4		;2048 in sectors
	; eax should contain the start of the descriptor table
	mov	[fs_desc_table], eax

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
	mov rsi,[fs_blocksize]
	call write_decimal
	mov rsi,[fs_blocksectors]
	call write_decimal
	mov rsi,[fs_superblock+ext2_superblock.n_inodes]
	call write_decimal
	mov rsi,[fs_superblock+ext2_superblock.n_blocks]
	call write_decimal
	mov rsi,[fs_superblock+ext2_superblock.n_blocks_group]
	call write_decimal
	mov rsi,[fs_superblock+ext2_superblock.n_inodes_group]
	call write_decimal
	pop rdx
	pop rax
	pop rsi
	ret

; IN:	rdi = inode number
; OUT:	rax = group number of inode
;		rdx = inode offset in group
os_ext2_inode_groups:
	; rax = group of inode = (inode_number-1) / inodes_per_group
	; rdx = inode in group = (inode_number-1) % inodes_per_group
	sub rdi,1	; inode numbers start at 1
	mov rax,rdi
	xor edx,edx
	div DWORD [fs_superblock+ext2_superblock.n_inodes_group];
	ret

; IN:	rax = group number
; OUT:	rax = sector number of group
;		rdx = group offset in sector
os_ext2_group_sectors:
	; rax = sector of group = group/16
	; rdx = group in sector = group%16
	xor	edx, edx
	mov rdi,16
	div edi		; 16 is 512/32 = number of groups per sector
	add eax,DWORD[fs_desc_table] ; rax = sector containing blockgroup descriptor
	ret

;------------------------------------------------------------------------------
; IN:	rdi = inode number
;		rsi = memory address to store inode struct
; OUT:	rax = inode address
;
os_ext2_read_inode:
	push rsi
	push rdx
	push rdi
	push rcx
	push r8
	push r9

	mov r9,rsi

	; rax = group of inode
	; rdx = inode in group
	call os_ext2_inode_groups

	mov rsi,rdx ; save inodeN_in_group

	; rax = sector of group
	; rdx = group in sector
	; rsi = inode in group
	call os_ext2_group_sectors

	mov r8,rdx		;save group in sector

	; read sector containing group
	; rax = sector of n
	mov rcx,1		; one sector
	xor rdx,rdx		; disk 0
	lea rdi,[fs_descriptor]
	call readsectors	;readsectors(rax sector_no,rcx n_sectors,rdx disk_no,rdi *buffer)

	; rax = memory location of correct group descriptor = group in sector *32 + descriptor
	; rsi = inode in group
	shl r8,5
	mov rax,r8

	add rax,fs_descriptor

	; rax = sector of inode table
	mov r8,rsi
	mov rsi,[rax+ext2_descriptor.inode_table]
	xor rdx,rdx
	mov eax,[fs_blocksectors]
	mul esi

	;need to divide inode in group by 4(inodes per sector)
	;then get that sector and use remainder for inode in sector
	mov rcx,rax
	mov rax,r8
	mov rsi,4
	xor rdx,rdx
	div esi
	add rax,rcx
	mov r8,rdx
	; rdx should be inode in sector

	mov rcx,1
	xor rdx,rdx
	lea rdi,[fs_pwd_inode]
	call readsectors

	; multiply by size of inode struct to get the correct inode
	shl r8,7
	mov rax,r8

	add rax,fs_pwd_inode

	; copy to user supplied struct
	mov rdi,r9
	mov rsi,rax
	mov rdx,128
	call os_memcpy

	pop r9
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
	mov ecx,DWORD[fs_blocksectors]
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

; read a sector from an indirect block
; rdi = address of indirect block
; rsi = sector within indirect to read
; rdx = memory to store
os_ext2_indirect_sector_read:

;	mov rax,rdi
;	mov edi,DWORD[blocksectors]
;	div edi

	mov r8,rdx ; save memory location

	; divide sector by blocks per sector = block in indirect
	mov rax,rsi
	xor rdx,rdx
	div DWORD[fs_blocksectors]

	mov r9,rdx ; save sector in block

	; mutiply block by 4 (sizeof direct block pointer)
	mov esi,4
	mul esi

	;probably we need to actually read the block

	; add to memory address storing indirect block
	add rdi,rax ; this should now be the correct pointer (unless its >12+blocksize/32)
	mov eax,DWORD[rdi]

	;eax should now contain the block address
	mov ecx,DWORD[fs_blocksectors]
	mul ecx

	add rax, r9	; sector to read
	mov rcx, 1	; read 1 sector
	xor rdx,rdx
	mov rdi,r8	; memory to store
	call readsectors

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
	push r9

	add rdi,ext2_inode.block_pointer

	mov r8,rdx

	mov rax,rsi
	xor rdx,rdx
	div DWORD[fs_blocksectors]

	mov r9,rdx

	mov esi,4
	mul esi

	add rdi,rax ; this should now be the correct pointer (unless its >12)
	mov eax,DWORD[rdi]

	;eax should now contain the block address
	mov ecx,DWORD[fs_blocksectors]
	mul ecx

	add rax, r9	; sector to read
	mov rcx, 1	; read 1 sector
	xor rdx,rdx
	mov rdi,r8	; memory to store
	call readsectors

	pop r9
	pop r8
	pop rcx
	pop rsi
	pop rax
	pop rdx
	pop rdi
	ret


;------------------------------------------------------------------------------
; read a sector from an inode
; rax = block address of indirect block
; rsi = block number to get inside indirect
; out: rax = block address of target
os_ext2_indirect_get_block:
	push rsi
	push rdi
	push rcx

	;multiply by 4 because a block address is 32 bit
	sal rsi,2

	;get sector of block
	mov ecx,DWORD[fs_blocksectors]
	mul ecx

	;rax contains sector to read
	mov ecx,DWORD[fs_blocksectors]; read the whole block
	xor rdx,rdx	;	disk 0
	lea rdi,[fs_misc+512] ; memory to store
	call readsectors ; get the block

	lea rax,[fs_misc+512]
	add rax,rsi ; arriving at the address of the target block
	mov eax,DWORD[rax];

	pop rcx
	pop rdi
	pop rsi

;------------------------------------------------------------------------------
; read a sector from an inode
; rdi = memory address of inode structure
; rsi = sector within inode to read
; rdx = memory to store
os_ext2_get_block:
	push rdi
	push rdx
	push rax
	push rsi
	push rcx
	push r8
	push r9

	add rdi,ext2_inode.block_pointer

	mov r8,rdx

	mov rax,rsi
	xor rdx,rdx
	div DWORD[fs_blocksectors]

	mov r9,rdx ; sector in block

	cmp rax,12
	jae .block_is_indirect

.block_is_direct:
	sal rax,2 ;multiply by 4 (4bytes)

	add rdi,rax ; this should now be the correct pointer (unless its >12)
	mov eax,DWORD[rdi]
	jmp .block_is_address

.block_is_indirect:

	sub rax,12

;	mov esi,DWORD[fs_blocksize] ; blocksize
;	sar rsi,5

;	cmp rax,rsi
;	jae .block_is_double_indirect

	mov rsi,rax
	add rdi,48
	mov eax,DWORD[rdi] ; should be block pointer 12 (single indirect)
	call os_ext2_indirect_get_block
	jmp .block_is_address

.block_is_double_indirect:

	sub rax,rsi ;rsi contains the number of block pointers per indirect block

	; need to divide rax by (blocksize/4) and keep remainder

	mov rsi,rax
	add rdi,52
	mov eax,DWORD[rdi] ; should be block pointer 12+1 (double indirect)
	call os_ext2_indirect_get_block


	jmp .block_is_address


.block_is_address:
	;eax should now contain the block address
	;multiply by sectors per block to get sector of block
	mov ecx,DWORD[fs_blocksectors]
	mul ecx

	add rax, r9	; sector to read
	mov ecx, 1	; read 1 sector
	xor rdx,rdx ; disk 0
	mov rdi,r8	; memory to store
	call readsectors

	pop r9
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

	mov rcx,[rdi+ext2_inode.n_sectors]
	mov rsi,rcx
	xor rsi,rsi ; start reading at sector 0

.load_loop:

	call os_ext2_get_block

	add rdx,512	; advance memory pointer to next location
	inc rsi		; advance sector number
	cmp rsi,rcx ; make sure we still have data to read
	jl .load_loop; read more if not finished

.load_done:
	pop rcx
	pop rsi
	pop rax
	pop rdx
	pop rdi
	ret

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
	lea rdx,[fs_directory]
	call os_ext2_file_load

	lea rbx,[fs_directory]
	mov ecx,[rdi+ext2_inode.l_size]
	add rbx,rcx ; end address

	lea rdx,[fs_directory] ; start address

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
	lea rdx,[fs_directory]
	call os_ext2_inode_block_read

	lea rbx,[fs_directory]
	mov ecx,[rdi+ext2_inode.l_size]
	add rbx,rcx ; end address

	lea rdx,[fs_directory] ; start address

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
