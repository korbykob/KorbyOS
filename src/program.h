#include "shared.h"

uint64_t execute(const CHAR16* file)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $0, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=g"(result) : "g"(file) : "%rdi", "%rsi");
    return result;
}

void quit()
{
    __asm__ volatile ("movq $1, %%rdi; int $0x80" : : : "%rdi");
}

void* allocate(uint64_t amount)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $2, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=g"(result) : "g"(amount) : "%rdi", "%rsi");
    return (void*)result;
}

void unallocate(void* pointer)
{
    __asm__ volatile ("movq $3, %%rdi; movq %0, %%rsi; int $0x80" : : "g"((uint64_t)pointer) : "%rdi", "%rsi");
}

void* writeFile(const CHAR16* name, uint64_t size)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $4, %%rdi; movq %1, %%rsi; movq %2, %%rdx; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)name), "g"(size) : "%rdi", "%rsi", "%rdx");
    return (void*)result;
}

uint8_t* readFile(const CHAR16* name, uint64_t* size)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $5, %%rdi; movq %1, %%rsi; movq %2, %%rdx; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)name), "g"((uint64_t)size) : "%rdi", "%rsi", "%rdx");
    return (uint8_t*)result;
}

BOOLEAN checkFile(const CHAR16* name)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $6, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)name) : "%rdi", "%rsi");
    return (BOOLEAN)result;
}

BOOLEAN checkFolder(const CHAR16* name)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $7, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)name) : "%rdi", "%rsi");
    return (BOOLEAN)result;
}

void deleteFile(const CHAR16* name)
{
    __asm__ volatile ("movq $8, %%rdi; movq %0, %%rsi; int $0x80" : : "g"((uint64_t)name) : "%rdi", "%rsi");
}

File** getFiles(const CHAR16* root, uint64_t* count, BOOLEAN recursive)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $9, %%rdi; movq %1, %%rsi; movq %2, %%rdx; movq %3, %%rcx; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)root), "g"((uint64_t)count), "g"((uint64_t)recursive) : "%rdi", "%rsi", "%rdx", "%rcx");
    return (File**)result;
}

uint64_t getCores()
{
    uint64_t result = 0;
    __asm__ volatile ("movq $10, %%rdi; int $0x80; movq %%rax, %0" : "=g"(result) : : "%rdi");
    return result;
}

PointerArray* addKeyCall(void (*keyCall)(uint8_t scancode, BOOLEAN pressed))
{
    uint64_t result = 0;
    __asm__ volatile ("movq $11, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)keyCall) : "%rdi", "%rsi");
    return (PointerArray*)result;
}

void removeKeyCall(PointerArray* keyCall)
{
    __asm__ volatile ("movq $12, %%rdi; movq %0, %%rsi; int $0x80" : : "g"((uint64_t)keyCall) : "%rdi", "%rsi");
}

PointerArray* addMoveCall(void (*moveCall)(int16_t x, int16_t y))
{
    uint64_t result = 0;
    __asm__ volatile ("movq $13, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)moveCall) : "%rdi", "%rsi");
    return (PointerArray*)result;
}

void removeMoveCall(PointerArray* moveCall)
{
    __asm__ volatile ("movq $14, %%rdi; movq %0, %%rsi; int $0x80" : : "g"((uint64_t)moveCall) : "%rdi", "%rsi");
}

PointerArray* addClickCall(void (*clickCall)(BOOLEAN left, BOOLEAN pressed))
{
    uint64_t result = 0;
    __asm__ volatile ("movq $15, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)clickCall) : "%rdi", "%rsi");
    return (PointerArray*)result;
}

void removeClickCall(PointerArray* clickCall)
{
    __asm__ volatile ("movq $16, %%rdi; movq %0, %%rsi; int $0x80" : : "g"((uint64_t)clickCall) : "%rdi", "%rsi");
}

uint8_t registerSyscallHandler(const CHAR16* name, uint64_t (*syscallHandler)(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4))
{
    uint64_t result = 0;
    __asm__ volatile ("movq $17, %%rdi; movq %1, %%rsi; movq %2, %%rdx; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)name), "g"((uint64_t)syscallHandler) : "%rdi", "%rsi", "%rdx");
    return (uint8_t)result;
}

uint8_t getRegisteredSyscall(const CHAR16* name)
{
    uint64_t result = 0;
    __asm__ volatile ("movq $18, %%rdi; movq %1, %%rsi; int $0x80; movq %%rax, %0" : "=g"(result) : "g"((uint64_t)name) : "%rdi", "%rsi");
    return (uint8_t)result;
}

void unregisterSyscallHandler(uint8_t id)
{
    __asm__ volatile ("movq $19, %%rdi; movq %0, %%rsi; int $0x80" : : "g"((uint64_t)id) : "%rdi", "%rsi");
}

void registerTerminal(void (*function)(const CHAR16* message))
{
    __asm__ volatile ("movq $20, %%rdi; movq %0, %%rsi; int $0x80" : : "g"((uint64_t)function) : "%rdi", "%rsi");
}

void print(const CHAR16* message)
{
    __asm__ volatile ("movq $21, %%rdi; movq %0, %%rsi; int $0x80" : : "g"((uint64_t)message) : "%rdi", "%rsi");
}

void unregisterTerminal()
{
    __asm__ volatile ("movq $22, %%rdi; int $0x80" : : : "%rdi");
}

void getDisplayInfo(Display* display)
{
    __asm__ volatile ("movq $23, %%rdi; movq %0, %%rsi; int $0x80" : : "g"((uint64_t)display) : "%rdi", "%rsi");
}

void waitForProgram(uint64_t pid, BOOLEAN* done)
{
    __asm__ volatile ("movq $24, %%rdi; movq %0, %%rsi; movq %1, %%rdx; int $0x80" : : "g"(pid), "g"((uint64_t)done) : "%rdi", "%rsi", "%rdx");
}
