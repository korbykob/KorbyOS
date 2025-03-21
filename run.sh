#!/bin/bash
set -e
clear
mkdir -p bin
gcc -Ignu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/main.c -o bin/main.o
ld -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -Tgnu-efi/gnuefi/elf_x86_64_efi.lds -znoexecstack gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o bin/main.o -o bin/main.so -lgnuefi -lefi
objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 bin/main.so bin/main.efi
mkdir -p bin/system
nasm -f bin src/system/smp.asm -o bin/system/smp.bin
mkdir -p bin/programs/desktop
nasm -f elf64 src/programs/desktop/program.asm -o bin/programs/desktop/programasm.o
gcc -Ignu-efi/inc -fno-zero-initialized-in-bss -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/programs/desktop/program.c -o bin/programs/desktop/program.o
ld -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -o bin/programs/desktop/program.bin -Ttext 0x0 --oformat binary bin/programs/desktop/programasm.o bin/programs/desktop/program.o -lgnuefi -lefi
mkdir -p bin/programs/rendering
nasm -f elf64 src/programs/rendering/program.asm -o bin/programs/rendering/programasm.o
gcc -Ignu-efi/inc -fno-zero-initialized-in-bss -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/programs/rendering/program.c -o bin/programs/rendering/program.o
ld -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -o bin/programs/rendering/program.bin -Ttext 0x0 --oformat binary bin/programs/rendering/programasm.o bin/programs/rendering/program.o -lgnuefi -lefi
uefi-run -b OVMF-pure-efi.fd -d -s 20 \
-f bin/system/smp.bin:system/smp.bin \
-f src/fonts/font.psf:fonts/font.psf \
-f bin/programs/desktop/program.bin:programs/desktop/program.bin \
-f src/programs/desktop/wallpaper.bmp:programs/desktop/wallpaper.bmp \
-f bin/programs/rendering/program.bin:programs/rendering/program.bin \
-f src/programs/rendering/program.bmp:programs/rendering/program.bmp \
-f src/programs/rendering/wall.bmp:programs/rendering/wall.bmp \
-f src/programs/rendering/sprite.bmp:programs/rendering/sprite.bmp \
bin/main.efi -- -accel kvm -m 16G -cpu host -smp 4 -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -serial null -serial null -serial stdio