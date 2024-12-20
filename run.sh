#!/bin/bash
set -e
clear
mkdir -p bin
gcc -Ignu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -mgeneral-regs-only -c src/main.c -o bin/main.o
ld -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -Tgnu-efi/gnuefi/elf_x86_64_efi.lds -z noexecstack gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o bin/main.o -o bin/main.so -lgnuefi -lefi
objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 bin/main.so bin/main.efi
uefi-run -b OVMF-pure-efi.fd -d -f src/font.psf -f src/wallpaper.bmp bin/main.efi -- -enable-kvm -m 16G -cpu host -monitor stdio
echo