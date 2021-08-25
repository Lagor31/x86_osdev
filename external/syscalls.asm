[bits 32]

global _system_call0     ; Allows the C code to link to this
_system_call0:
    push  ebp         ; Save the stack-frame base pointer (of the calling function).
    mov   ebp, esp    ; Set the stack-frame base pointer to be the current
                        ; location on the stack.
    mov eax, [ebp + 8]
    int 31
    pop ebp
ret    

global _system_call1     ; Allows the C code to link to this
_system_call1:
    push  ebp         ; Save the stack-frame base pointer (of the calling function).
    mov   ebp, esp    ; Set the stack-frame base pointer to be the current
                        ; location on the stack.
    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    int 31
    pop ebp
ret 

global _system_call2     ; Allows the C code to link to this
_system_call2:
    push  ebp         ; Save the stack-frame base pointer (of the calling function).
    mov   ebp, esp    ; Set the stack-frame base pointer to be the current
                        ; location on the stack.
    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    mov ecx, [ebp + 16]

    int 31
    pop ebp
ret 

global _system_call3     ; Allows the C code to link to this
_system_call3:
    push  ebp         ; Save the stack-frame base pointer (of the calling function).
    mov   ebp, esp    ; Set the stack-frame base pointer to be the current
                        ; location on the stack.
    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    mov ecx, [ebp + 16]
    mov edx, [ebp + 20]

    int 31
    pop ebp
ret 


