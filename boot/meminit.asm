[bits 32]            ;We're already in Protected Mode thanks to GRUB which enables A20 for us
[extern kernel_main] ; kernel entry point defined in the C file kernel.c
[extern kimage_end]  ; Address of the end of the kernel image in memory

; Needs to be writable (we're writing the page directory variables)
; the rest of the attributes can be the same as a .text default section
section .meminit  progbits  alloc   exec    write  align=16

;This was specified in the linker.ld as the entry point of the ELF Kernel file
;GRUB2 will drop us here
global _start
_start:

    ; We find the following values as given by GRUB2
    mov [_multiboot_info.magic], eax ; Multiboot2 Magic Number
    mov [_multiboot_info.addr], ebx ; Multiboot2 Info structure address (Physical)
    

    ; Setting Page Global Enabled (bit 8 of CR4):
    ; If set, address translations (PDE or PTE records) may be shared between address spaces.

    mov ecx, cr4
    or ecx, PGE_ENABLE
    mov cr4, ecx

    
    ; We need to setup a basic paging system so that when we call kernel_main, the Virtual addresses 
    ; are mapped to the right physical frames.
    ; The kernel will then overwrite this mapping with a more complete one.
    ; This mapping will map the first MiB of Physical RAM to the 1st MiB of Virtual Address Space
    ; and to the virtual address to which we decided to map the kernel (KERNEL_VIRTUAL_BASE = 0xC0000000)

    xor ecx, ecx

    ; We setup the first 4 Page Directory entries to point to our 4 Page Tables (ultimately mapping the VSpace to the PSpace)
    ; PageDir[768] corresponds to the address 0xC0000000

_setup_frames:
    xor edx,edx
    mov edx, ecx
    shl edx,12
    or edx, KPAGE_TABLE_MASK
    mov [BootPageTable + ecx * 4], edx
    cmp ecx, 1024
    jge _end_setup_frames
    inc ecx
    jmp _setup_frames

_end_setup_frames:
    mov edx, BootPageTable
    or edx, KPAGE_TABLE_MASK

    mov [BootPageDirectory + 0], edx
    mov [BootPageDirectory + 768 * 4 ], edx

    xor ecx, ecx

_setup_frames2:
    xor edx,edx
    mov edx, ecx
    add edx, 1024
    shl edx,12
    or edx, KPAGE_TABLE_MASK
    mov [BootPageTable2 + ecx * 4], edx
    cmp ecx, 1024
    jge _end_setup_frames2
    inc ecx
    jmp _setup_frames2

_end_setup_frames2:

    mov edx, BootPageTable2
    or edx, KPAGE_TABLE_MASK

    mov [BootPageDirectory + 4], edx
    mov [BootPageDirectory + 769 * 4 ], edx

    xor ecx, ecx

_setup_frames3:

    xor edx,edx
    mov edx, ecx
    add edx, 2048
    shl edx,12
    or edx, KPAGE_TABLE_MASK
    mov [BootPageTable3 + ecx * 4], edx
    cmp ecx, 1024
    jge _end_setup_frames3
    inc ecx
    jmp _setup_frames3

_end_setup_frames3:

    mov edx, BootPageTable3
    or edx, KPAGE_TABLE_MASK

    mov [BootPageDirectory + 8], edx
    mov [BootPageDirectory + 770 * 4], edx


    xor ecx, ecx

_setup_frames4:

    xor edx,edx
    mov edx, ecx
    add edx, 3072
    shl edx,12
    or edx, KPAGE_TABLE_MASK
    mov [BootPageTable4 + ecx * 4], edx
    cmp ecx, 1024
    jge _end_setup_frames4
    inc ecx
    jmp _setup_frames4

_end_setup_frames4:

    mov edx, BootPageTable4
    or edx, KPAGE_TABLE_MASK

    mov [BootPageDirectory + 12], edx
    mov [BootPageDirectory + 771 * 4], edx    
    xor ecx, ecx


_setup_frames5:

    xor edx,edx
    mov edx, ecx
    add edx, 4096
    shl edx,12
    or edx, KPAGE_TABLE_MASK
    mov [BootPageTable5 + ecx * 4], edx
    cmp ecx, 1024
    jge _end_setup_frames5
    inc ecx
    jmp _setup_frames5

_end_setup_frames5:

    mov edx, BootPageTable5
    or edx, KPAGE_TABLE_MASK

    mov [BootPageDirectory + 16], edx
    mov [BootPageDirectory + 772 * 4], edx    

    xor ecx, ecx



_setup_frames6:

    xor edx,edx
    mov edx, ecx
    add edx, 5120
    shl edx,12
    or edx, KPAGE_TABLE_MASK
    mov [BootPageTable6 + ecx * 4], edx
    cmp ecx, 1024
    jge _end_setup_frames6
    inc ecx
    jmp _setup_frames6

_end_setup_frames6:

    mov edx, BootPageTable6
    or edx, KPAGE_TABLE_MASK

    mov [BootPageDirectory + 20], edx
    mov [BootPageDirectory + 773 * 4], edx    

    xor ecx, ecx


_setup_frames7:

    xor edx,edx
    mov edx, ecx
    add edx, 6144
    shl edx,12
    or edx, KPAGE_TABLE_MASK
    mov [BootPageTable7 + ecx * 4], edx
    cmp ecx, 1024
    jge _end_setup_frames7
    inc ecx
    jmp _setup_frames7

_end_setup_frames7:

    mov edx, BootPageTable7
    or edx, KPAGE_TABLE_MASK

    mov [BootPageDirectory + 24], edx
    mov [BootPageDirectory + 774 * 4], edx    




	mov eax, BootPageDirectory 
	mov cr3, eax
    mov eax,cr0
	or eax, PG_ENABLE
	mov cr0,eax

    xor ecx, ecx

    ; At this point we select our kernel ESP by adding a fixed amount to the end of the kernel address
_setting_up_kernel_stack:

    ; GRUB2 puts the module in an unconvenient location that gets overwritten if we're not careful enough
    ; Our kernel free mem pointer must start at the end of the module

    ;mov edx, kimage_end

    mov ebx, [_multiboot_info.addr] ; ebx = Address of the module
    mov ebx, [ebx]                  ; The first 4 bytes are the length of the module
    mov ecx, [_multiboot_info.addr] 
    add ecx, ebx                    ; module_address + size = first free byte ptr
    add ecx, KERNEL_VIRTUAL_BASE

    add ecx, KSTACK_SIZE

    push ecx

    and ecx, 0x00000FFF ; Was it 4K aligned?
    cmp ecx, 0
    je _stack_ok

    pop ecx
    and ecx, 0xFFFFF000
    add ecx, 0x1000     ; We make it 4K aligned
    push ecx
    
_stack_ok:
    pop ecx
    mov [_stack_address], ecx
    mov esp, ecx
    xor ebp, ebp

    ; We restore the previously saved Multiboot2 data and we pass it on to the kernel main function
    mov eax, [_multiboot_info.magic]
    mov ebx, [_multiboot_info.addr]
    add ebx, KERNEL_VIRTUAL_BASE ; We need to convert to the Virtual Address in the kernel

    push ebx
    push eax
_call_main:
    call kernel_main
    ; Idling
_idle:
    hlt
    jmp _idle



KPAGE_TABLE_MASK equ 3
PGE_ENABLE equ 0x80
PG_ENABLE equ 0x80000000
KERNEL_VIRTUAL_BASE equ 0xC0000000
KSTACK_SIZE equ 0x1000 ; Number of bytes

; Space for our Paging Structures (their address needs to be aligned at 4K)
align 0x1000 ; align to 4KB, the size of a page
BootPageDirectory:
    times 1024 dd 0

align 0x1000 ; align to 4KB, the size of a page
BootPageTable:
    times 1024 dd 0

align 0x1000 ; align to 4KB, the size of a page
BootPageTable2:
    times 1024 dd 0

align 0x1000 ; align to 4KB, the size of a page
BootPageTable3:
    times 1024 dd 0    

align 0x1000 ; align to 4KB, the size of a page
BootPageTable4:
    times 1024 dd 0    

align 0x1000 ; align to 4KB, the size of a page
BootPageTable5:
    times 1024 dd 0   

align 0x1000 ; align to 4KB, the size of a page
BootPageTable6:
    times 1024 dd 0    

align 0x1000 ; align to 4KB, the size of a page
BootPageTable7:
    times 1024 dd 0   

align 4
global _stack_address
_stack_address:
dd 0

_multiboot_info:
.magic:
    dd  0
.addr:
    dd  0

 