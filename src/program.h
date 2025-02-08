#include "shared.h"

void quit(uint64_t pid)
{
    __asm__ volatile ("movq $0, %%rdi; movq %0, %%rsi; int $0x80" :  : "r"(pid) : "%rdi", "%rsi");
}

void* allocate(uint64_t amount)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $1, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=r"(result) : "r"(amount) : "%rdi", "%rsi");
    return (void*)result;
}

void unallocate(void* pointer)
{
    __asm__ volatile ("movq $2, %%rdi; movq %0, %%rsi; int $0x80" :  : "r"((uint64_t)pointer) : "%rdi", "%rsi");
}

Window* allocateWindow(uint32_t width, uint32_t height, const CHAR16* title, const CHAR16* icon)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $3, %%rdi; movq %1, %%rsi; movq %2, %%rdx; movq %3, %%rcx; movq %4, %%r8; int $0x80; movq %%rax, %0" : "=r"(result) : "r"((uint64_t)width), "r"((uint64_t)height), "r"((uint64_t)title), "r"((uint64_t)icon) : "%rdi", "%rsi", "%rdx", "%rcx");
    return (Window*)result;
}

Window* allocateFullscreenWindow(const CHAR16* icon)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $4, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=r"(result) : "r"((uint64_t)icon) : "%rdi", "%rsi");
    return (Window*)result;
}

void unallocateWindow(Window* window)
{
    __asm__ volatile ("movq $5, %%rdi; movq %0, %%rsi; int $0x80" :  : "r"((uint64_t)window) : "%rdi", "%rsi");
}

void* writeFile(const CHAR16* name, uint64_t size)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $6, %%rdi; movq %1, %%rsi; movq %2, %%rdx; int $0x80; movq %%rax, %0" : "=r"(result) : "r"((uint64_t)name), "r"(size) : "%rdi", "%rsi", "%rdx");
    return (void*)result;
}

uint8_t* readFile(const CHAR16* name, uint64_t* size)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $7, %%rdi; movq %1, %%rsi; movq %2, %%rdx; int $0x80; movq %%rax, %0" : "=r"(result) : "r"((uint64_t)name), "r"((uint64_t)size) : "%rdi", "%rsi", "%rdx");
    return (uint8_t*)result;
}

void deleteFile(const CHAR16* name)
{
    __asm__ volatile ("movq $8, %%rdi; movq %0, %%rsi; int $0x80" :  : "r"((uint64_t)name) : "%rdi", "%rsi");
}