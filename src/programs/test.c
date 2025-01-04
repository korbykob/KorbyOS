#include <efi.h>
#include <efilib.h>

void _start()
{
    uint64_t number = 0;
    __asm__ volatile ("movq %0, %%rdi; movq $1000000, %%rsi; int $0x80" :  : "r"(number) : "%rdi");
    __asm__ volatile ("movq %%rax, %0" : "=r"(number));
    uint8_t* address = (uint8_t*)number;
    for (size_t i = 0; i < 1000000; i++)
    {
        address[i] = 0xFF;
    }
}