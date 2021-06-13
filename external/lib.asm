[bits 32]

global _system_call         ; Allows the C code to link to this
_system_call:
    pop eax
    int 0x31
ret 