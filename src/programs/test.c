signed long int rdx = 0;

void _start()
{
    __asm__ volatile ("movq %0, %%rdx; int $0x80" :  : "r"(rdx) : "%rdx");
    rdx++;
}
