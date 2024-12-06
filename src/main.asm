bits 64

global inb
global outb
global loadIdt
global interrupts
global pitHandler
global cascadeHandler
extern pit

inb:
    mov dx, [rsp + 8]
    in al, dx
    ret

outb:
    mov dx, [rsp + 8]
    jmp $
    mov al, [rsp + 16]
    out dx, al
    ret

loadIdt:
    mov rdx, [rsp + 8]
    lidt [rdx]
    sti
    ret

interrupts:
    sti
    ret

pitHandler:
    call pit
    iretq

cascadeHandler:
    iretq