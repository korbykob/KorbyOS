#include <efi.h>
#include <efilib.h>

void _start()
{
    uint64_t number = 0;
    __asm__ volatile ("movq %0, %%rdi; int $0x80" :  : "r"(number) : "%rdi");
    __asm__ volatile ("movq %%rax, %0" : "=r"(number));
    // number should be 1234 because syscall 0 returns 1234 in rax
}