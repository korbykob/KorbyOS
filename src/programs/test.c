void _start()
{
    __asm__ volatile ("mov %0, %%rbx" : : "r" (1234) : "%rbx");
}
