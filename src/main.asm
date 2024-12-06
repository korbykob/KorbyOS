bits 64

global inb
global outb
global loadIdt
global pitHandler
global cascadeHandler
extern pit

inb:
    mov rdx, rdi
    in al, dx
    ret

outb:
    mov rdx, rdi
    mov rax, rsi
    out dx, al
    ret

loadIdt:
    lidt [rdi]
    sti
    ret

pitHandler:
    call pit
    iretq

cascadeHandler:
    iretq