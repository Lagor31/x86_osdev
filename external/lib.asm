[bits 32]

global _system_call         ; Allows the C code to link to this
_system_call:
    push  ebp         ; Save the stack-frame base pointer (of the calling function).
    mov   ebp, esp    ; Set the stack-frame base pointer to be the current
                        ; location on the stack.
    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    int 31
    pop ebp
ret 

