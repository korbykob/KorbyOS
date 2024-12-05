bits 64

global interrupts
global pitHandler
global cascadeHandler
extern pit

interrupts:
    sti
    ret

pitHandler:
    call pit
    iretq

cascadeHandler:
    iretq