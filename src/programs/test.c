void _start()
{
    __asm__ volatile ("mov %0, %%rax" : : "r" (1234) : "%rax");
}
