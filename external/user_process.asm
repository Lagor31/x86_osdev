VIDEO_MEMORY equ 0x400000 + 0xB8000
xor ecx, ecx
mov eax, 0x0f31
loop_start:
    sti
    mov DWORD [VIDEO_MEMORY + ecx], eax
    add ecx,2
    cmp ecx, 80
    jl loop_start
    xor ecx, ecx
    inc eax
    mov edx, eax
    and edx, 0xFF
    cmp edx, 0x39
    jl loop_start    
    mov eax, 0x0f31
    jmp loop_start
ret