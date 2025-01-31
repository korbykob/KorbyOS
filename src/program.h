#include "shared.h"

void quit(uint64_t id)
{
    __asm__ volatile ("movq $0, %%rdi; movq %0, %%rsi; int $0x80" :  : "r"(id) : "%rdi", "%rsi");
}

void* allocate(uint64_t amount)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $1, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=r"(result) : "r"(amount) : "%rdi", "%rsi");
    return (void*)result;
}

void unallocate(void* pointer, uint64_t amount)
{
    __asm__ volatile ("movq $2, %%rdi; movq %0, %%rsi; movq %1, %%rdx; int $0x80" :  : "r"((uint64_t)pointer), "r"(amount) : "%rdi", "%rsi", "%rdx");
}

Window* allocateWindow(uint32_t width, uint32_t height, CHAR16* title, EFI_GRAPHICS_OUTPUT_BLT_PIXEL* icon)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $3, %%rdi; movq %1, %%rsi; movq %2, %%rdx; movq %3, %%rcx; movq %4, %%r8; int $0x80; movq %%rax, %0" : "=r"(result) : "r"((uint64_t)width), "r"((uint64_t)height), "r"((uint64_t)title), "r"((uint64_t)icon) : "%rdi", "%rsi", "%rdx", "%rcx");
    return (Window*)result;
}

Window* allocateFullscreenWindow()
{
    uint64_t result = 0;
    __asm__ volatile ("movq $4, %%rdi; int $0x80; movq %%rax, %0" : "=r"(result) :  : "%rdi");
    return (Window*)result;
}

void unallocateWindow(Window* window)
{
    __asm__ volatile ("movq $5, %%rdi; movq %0, %%rsi; int $0x80" :  : "r"((uint64_t)window) : "%rdi", "%rsi");
}

void* createFile(const CHAR16* name, uint64_t size)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $6, %%rdi; movq %1, %%rsi; movq %2, %%rdx; int $0x80; movq %%rax, %0" : "=r"(result) : "r"((uint64_t)name), "r"(size) : "%rdi", "%rsi", "%rdx");
    return (void*)result;
}

BOOLEAN readFile(const CHAR16* name, uint8_t** data, uint64_t* size)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $7, %%rdi; movq %1, %%rsi; movq %2, %%rdx; movq %3, %%rcx; int $0x80; movq %%rax, %0" : "=r"(result) : "r"((uint64_t)name), "r"((uint64_t)data), "r"((uint64_t)size) : "%rdi", "%rsi", "%rdx", "%rcx");
    return (BOOLEAN*)result;
}

void deleteFile(const CHAR16* name)
{
    __asm__ volatile ("movq $8, %%rdi; movq %0, %%rsi; int $0x80" :  : "r"((uint64_t)name) : "%rdi", "%rsi");
}