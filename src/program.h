#include <efi.h>
#include <efilib.h>

void* alloc(uint64_t amount)
{
    __asm__ volatile ("movq $0, %%rdi; movq %0, %%rsi; int $0x80" :  : "r"(amount) : "%rdi", "%rsi");
    uint64_t address = 0;
    __asm__ volatile ("movq %%rax, %0" : "=r"(address));
    return (void*)address;
}