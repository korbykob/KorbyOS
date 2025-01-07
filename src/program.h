#include <efi.h>
#include <efilib.h>

void* alloc(uint64_t amount)
{
    uint64_t address = 0;
    __asm__ volatile ("movq $0, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=r"(address) : "r"(amount) : "%rdi", "%rsi");
    return (void*)address;
}