org 0xF000
bits 16

mov eax, cr0
and eax, 01111111111111111111111111111111b
mov cr0, eax
mov edi, 0x1000
mov cr3, edi
mov eax, cr4
or eax, 100000b
mov cr4, eax
mov ecx, 0xC0000080
rdmsr
or eax, 100000000b
wrmsr
mov eax, cr0
or eax, 10000000000000000000000000000001b
mov cr0, eax
lgdt [gdtr]
jmp 0x8:boot

gdt:
    dq 0x0000000000000000
    dq 0x00209A0000000000
    dq 0x0000920000000000

gdtr:
    dw $ - gdt - 1
    dq gdt

bits 64

boot:
    cli
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rdi, [0x5000]
    mov cr3, rdi
    mov rax, cr0
    and ax, 0xFFF
    or ax, 0x2
    mov cr0, rax
    mov rax, cr4
    or ax, 11000000000b
    mov cr4, rax
    mov rax, 0
    mov al, [0xEFFF]
    mov rdi, rax
    inc rdi
    mov rbx, 8
    mul rbx
    add rax, 0x5008
    mov rsp, [rax]
    mov rbp, rsp
    inc byte[0xEFFF]
    mov qword[rax], 0

ready:
    cmp qword[rax], 0
    je ready
    push rax
    push rdi
    call [rax]
    pop rdi
    pop rax
    mov qword[rax], 0
    jmp ready