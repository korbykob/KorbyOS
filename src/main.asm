bits 64

global syscallHandler
extern syscallHandle

syscallHandler:
    call syscallHandle
    iretq