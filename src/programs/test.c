void _start()
{
    __asm__ volatile ("mov %0, %%rax; int 0x80" : : "r" (1234) : "%rax");
}
