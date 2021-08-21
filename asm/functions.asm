[bits 32]
; Extra functions in assembly
; We put them in the .text section so that their addresses will be Kernel Virtual Addresses
section .text


; This will set up our new segment registers.  We need to do
; something special in order to set CS.  We do what is called
; a far jump.  A jump that includes a segment as well as an
; offset.  This is declared in C as 'extern void gdt_flush();'
global _gdt_flush         ; Allows the C code to link to this
extern _gp                ; Says that '_gp' is in another file
_gdt_flush:
    lgdt [_gp]            ; Load the GDT with our '_gp' which is a special pointer
    mov ax, 0x10          ; 0x10 is the offset in the GDT to our data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:flush        ; 0x08 is the offset to our code segment: Far jump!
flush:
ret                       ; Returns back to the C code!


global _tss_flush         ; Allows our C code to call tss_flush().
_tss_flush:
   mov ax, 0x2B           ; Load the index of our TSS structure - The index is
                          ; 0x28, as it is the 5th selector and each is 8 bytes
                          ; long, but we set the bottom two bits (making 0x2B)
                          ; so that it has an RPL of 3, not zero.
   ltr ax                 ; Load 0x2B into the task state register.
ret

; void _loadPageDirectory(uint32_t *pdPhysicalAddress)
; Just moves the supplied parameter in cr3 to switch page mapping
global _loadPageDirectory
_loadPageDirectory:
    push ebp
    mov ebp,esp
    mov eax, [esp + 8]
    mov cr3, eax
    mov esp,ebp
    pop ebp
ret

global _spin_lock
_spin_lock:

    push  ebp         ; Save the stack-frame base pointer (of the calling function).
    mov   ebp, esp    ; Set the stack-frame base pointer to be the current
                        ; location on the stack.
    mov ebx, [ebp + 8]

_try_spin_lock:
    mov eax, 1
    xchg eax, [ebx]
    cmp eax, 0
    jne _try_spin_lock
    
    ;mov   esp, ebp    ; Put the stack pointer back where it was when this function
                    ; was called.
    pop   ebp         ; Restore the calling function's stack frame.
ret                 ; Return to the calling function.


global _test_spin_lock
_test_spin_lock:

    push  ebp         ; Save the stack-frame base pointer (of the calling function).
    mov   ebp, esp    ; Set the stack-frame base pointer to be the current
                        ; location on the stack.
    mov ebx, [ebp + 8]

_test_try_spin_lock:
    mov eax, 1
    xchg eax, [ebx]
    
            
    pop   ebp         ; Restore the calling function's stack frame.
ret                 ; Return to the calling function.

global _free_lock
_free_lock:
    push  ebp         ; Save the stack-frame base pointer (of the calling function).
    mov   ebp, esp    ; Set the stack-frame base pointer to be the current
                        ; location on the stack.
    mov ebx, [ebp + 8]
    mov [ebx], DWORD 0
    pop ebp
    ;mov   esp, ebp    ; Put the stack pointer back where it was when this function
                    ; was called.
ret




global _switch_to_thread
extern current_thread
extern tss

THREAD_ESP equ 0
PAGE_DIR equ 8
THREAD_TSS equ 4
KERNEL_CS equ 0x10

_switch_to_thread:
    ;Save previous task's state
 
    ;Notes:
    ;  For cdecl; EAX, ECX, and EDX are already saved by the caller and don't need to be saved again
    ;  EIP is already saved on the stack by the caller's "CALL" instruction
    ;  The task isn't able to change CR3 so it doesn't need to be saved
    ;  Segment registers are constants (while running kernel code) so they don't need to be saved
    
    mov edx, [esp]     ; ret addr
    mov ecx, [esp + 4]    ;next proc 

    pushf

    push cs                 
    push edx

    push ebx
    push esi
    push edi
    push ebp

    push ds
 
    mov edi, [current_thread]   ;edi = address of the previous task's "thread control block"
    ;esp offset in the Proc struct
    mov [edi + THREAD_ESP],esp         ;Save ESP for previous task's kernel stack in the thread's TCB
    
    ;Load next task's state 
    mov esi,ecx                 ;esi = address of the next task's "thread control block" (parameter passed on stack)
    mov [current_thread],esi      ;Current task's TCB is the next task TCB
 
    mov esp,[esi + THREAD_ESP]         ;Load ESP for next task's kernel stack from the thread's TCB
    mov eax,[esi + PAGE_DIR]         ;eax = address of page directory for next task
    mov ebx,[esi + THREAD_TSS]        ;ebx = address for the top of the next task's kernel stack
    mov [tss + 4],ebx            ;Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)
    mov DWORD [tss], KERNEL_CS
    mov ecx,cr3                   ;ecx = previous task's virtual address space
 
    cmp eax,ecx                   ;Does the virtual address space need to being changed?
    je .done_paging                   ; no, virtual address space is the same, so don't reload it and cause TLB flushes
    mov cr3,eax                   ; yes, load the next task's virtual address space

.done_paging:

    pop  ebx
    mov ds, bx
    mov es, bx
    mov gs, bx
    mov fs, bx

    pop ebp
    pop edi
    pop esi
    pop ebx 
    
iret



global _syscall
_syscall:
    push  ebp         ; Save the stack-frame base pointer (of the calling function).
    mov   ebp, esp    ; Set the stack-frame base pointer to be the current
                        ; location on the stack.
    mov eax, [ebp + 8]
    int 31

    pop ebp
ret


global _setreg
_setreg:
    push  ebp         ; Save the stack-frame base pointer (of the calling function).
    mov   ebp, esp    ; Set the stack-frame base pointer to be the current
                        ; location on the stack.

    cmp DWORD [ebp + 8], 0
    je c_eax
    cmp DWORD [ebp + 8], 1
    je c_ebx
    cmp DWORD [ebp + 8], 2
    je c_ecx
    cmp DWORD [ebp + 8], 3
    je c_edx

    c_eax:
    mov eax,DWORD [ebp + 12]
    jmp end
    c_ebx:
    mov ebx,DWORD [ebp + 12]
    jmp end
    c_ecx:
    mov ecx,DWORD [ebp + 12]
    jmp end
    c_edx:
    mov edx,DWORD [ebp + 12]
    jmp end
  end:

    pop ebp
ret



section .files  progbits  alloc   exec    write  align=32
incbin 'external/user_program'