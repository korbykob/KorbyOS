#!/bin/bash
set -e
clear
nasm -f elf64 main.asm -o mainasm.o
gcc -I/mnt/LinuxStuff/gnu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c main.c -o main.o
ld -shared -Bsymbolic -L/mnt/LinuxStuff/gnu-efi/x86_64/lib -L/mnt/LinuxStuff/gnu-efi/x86_64/gnuefi -T/mnt/LinuxStuff/gnu-efi/gnuefi/elf_x86_64_efi.lds -z noexecstack /mnt/LinuxStuff/gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o main.o mainasm.o -o main.so -lgnuefi -lefi
objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 main.so main.efi
uefi-run -b OVMF-pure-efi.fd -d -f font.psf main.efi -- -m 16G -smp 8