#!/bin/bash
set -e
clear
mkdir -p bin
mkdir -p bin/programs
nasm -f elf64 src/main.asm -o bin/mainasm.o
gcc -Ignu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/main.c -o bin/main.o
ld -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -Tgnu-efi/gnuefi/elf_x86_64_efi.lds -znoexecstack gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o bin/mainasm.o bin/main.o -o bin/main.so -lgnuefi -lefi
objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 bin/main.so bin/main.efi
nasm -f elf64 src/programs/test.asm -o bin/programs/testasm.o
gcc -Ignu-efi/inc -fno-zero-initialized-in-bss -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/programs/test.c -o bin/programs/test.o
ld -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -o bin/programs/test.bin -Ttext 0x0 --oformat binary bin/programs/testasm.o bin/programs/test.o -lgnuefi -lefi
uefi-run -s 20 -b OVMF-pure-efi.fd -d -f src/wallpapers/wallpaper.bmp:wallpapers/wallpaper.bmp -f src/fonts/font.psf:fonts/font.psf -f src/programs/test.bmp:programs/test/test.bmp -f bin/programs/test.bin:programs/test/test.bin bin/main.efi -- -enable-kvm -m 16G -cpu host -monitor stdio
echo