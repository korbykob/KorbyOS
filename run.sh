#!/bin/bash
set -e
clear
nasm -f elf64 src/main.asm -o bin/mainasm.o
gcc -I/mnt/LinuxStuff/gnu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/main.c -o bin/main.o
ld -shared -Bsymbolic -L/mnt/LinuxStuff/gnu-efi/x86_64/lib -L/mnt/LinuxStuff/gnu-efi/x86_64/gnuefi -T/mnt/LinuxStuff/gnu-efi/gnuefi/elf_x86_64_efi.lds -z noexecstack /mnt/LinuxStuff/gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o bin/main.o bin/mainasm.o -o bin/main.so -lgnuefi -lefi
objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 bin/main.so bin/main.efi
uefi-run -b OVMF-pure-efi.fd -d -f src/font.psf bin/main.efi -- -m 16G -smp 8 -monitor stdio
echo