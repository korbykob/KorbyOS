bits 64

global reload
extern completed

reload:
    push qword 0x08
    lea rax, [rel registers]
    push rax
    retfq

registers:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp completed