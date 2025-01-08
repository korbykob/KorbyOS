#include <efi.h>
#include <efilib.h>

void* allocate(uint64_t amount)
{
    uint64_t address = 0;
    __asm__ volatile ("movq $0, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=r"(address) : "r"(amount) : "%rdi", "%rsi");
    return (void*)address;
}

void unallocate(void* pointer, uint64_t amount)
{
    __asm__ volatile ("movq $1, %%rdi; movq %0, %%rsi; movq %1, %%rdx; int $0x80" :  : "r"(pointer), "r"(amount) : "%rdi", "%rsi", "%rdx");
}