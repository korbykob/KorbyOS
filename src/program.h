#include "shared.h"

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

struct Window* allocateWindow(uint32_t width, uint32_t height, CHAR16* title)
{
    uint64_t address = 0;
    __asm__ volatile ("movq $2, %%rdi; movq %1, %%rsi; movq %2, %%rdx; movq %3, %%rcx; int $0x80; movq %%rax, %0" : "=r"(address) : "r"((uint64_t)width), "r"((uint64_t)height), "r"(title) : "%rdi", "%rsi", "%rdx", "%rcx");
    return (struct Window*)address;
}

struct Window* allocateFullscreenWindow()
{
    uint64_t address = 0;
    __asm__ volatile ("movq $3, %%rdi; int $0x80; movq %%rax, %0" : "=r"(address) :  : "%rdi");
    return (struct Window*)address;
}

void unallocateWindow(struct Window* window)
{
    __asm__ volatile ("movq $4, %%rdi; movq %0, %%rsi; int $0x80" :  : "r"(window) : "%rdi", "%rsi");
}