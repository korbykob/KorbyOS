#include "../../program.h"

uint8_t syscall = 0;

void initialiseDesktopCalls()
{
    syscall = getRegisteredSyscall(L"desktop");
}

Window* allocateWindow(uint32_t width, uint32_t height, const CHAR16* title, const CHAR16* icon)
{
    uint64_t result = 0;
    __asm__ volatile ("movq %1, %%rdi; movq $0, %%rsi; movq %2, %%rdx; movq %3, %%rcx; movq %4, %%r8; movq %5, %%r9; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)syscall), "g"((uint64_t)width), "g"((uint64_t)height), "g"((uint64_t)title), "g"((uint64_t)icon) : "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9");
    return (Window*)result;
}

Window* allocateFullscreenWindow(const CHAR16* icon)
{
    uint64_t result = 0;
    __asm__ volatile ("movq %1, %%rdi; movq $1, %%rsi; movq %2, %%rdx; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)syscall), "g"((uint64_t)icon) : "%rdi", "%rsi", "%rdx");
    return (Window*)result;
}

void unallocateWindow(Window* window)
{
    __asm__ volatile ("movq %0, %%rdi; movq $2, %%rsi; movq %1, %%rdx; int $0x80" : : "g"((uint64_t)syscall), "g"((uint64_t)window) : "%rdi", "%rsi", "%rdx");
}
