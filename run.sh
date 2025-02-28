#!/bin/bash
set -e
clear
mkdir -p bin
nasm -f elf64 src/main.asm -o bin/mainasm.o
gcc -Ignu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/main.c -o bin/main.o
ld -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -Tgnu-efi/gnuefi/elf_x86_64_efi.lds -znoexecstack gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o bin/mainasm.o bin/main.o -o bin/main.so -lgnuefi -lefi
objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 bin/main.so bin/main.efi
mkdir -p bin/system
nasm -f bin src/system/smp.asm -o bin/system/smp.bin
mkdir -p bin/programs/test
nasm -f elf64 src/programs/test/test.asm -o bin/programs/test/testasm.o
gcc -Ignu-efi/inc -fno-zero-initialized-in-bss -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/programs/test/test.c -o bin/programs/test/test.o
ld -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -o bin/programs/test/test.bin -Ttext 0x0 --oformat binary bin/programs/test/testasm.o bin/programs/test/test.o -lgnuefi -lefi
uefi-run -b OVMF-pure-efi.fd -d -f bin/system/smp.bin:system/smp.bin -f src/wallpapers/wallpaper.bmp:wallpapers/wallpaper.bmp -f src/fonts/font.psf:fonts/font.psf -f src/programs/test/test.bmp:programs/test/test.bmp -f bin/programs/test/test.bin:programs/test/test.bin -f src/programs/test/wall.bmp:programs/test/wall.bmp bin/main.efi -- -accel kvm -m 16G -cpu host -smp 4 -monitor stdio
echo