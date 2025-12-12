#!/bin/bash
set -e
clear
mkdir -p bin
gcc -Ignu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/main.c -o bin/main.o
ld -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -Tgnu-efi/gnuefi/elf_x86_64_efi.lds -znoexecstack gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o bin/main.o -o bin/main.so -lgnuefi -lefi
objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 bin/main.so bin/main.efi
mkdir -p bin/system
nasm -f bin src/system/smp.asm -o bin/system/smp.boot
mkdir -p bin/programs/test
nasm -f elf64 src/programs/test/program.asm -o bin/programs/test/programasm.o
gcc -Ignu-efi/inc -fpie -fno-zero-initialized-in-bss -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/programs/test/program.c -o bin/programs/test/program.o
ld -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -znoexecstack -o bin/programs/test/program.bin --oformat binary bin/programs/test/programasm.o bin/programs/test/program.o -lgnuefi -lefi
mkdir -p bin/programs/desktop
nasm -f elf64 src/programs/desktop/program.asm -o bin/programs/desktop/programasm.o
gcc -Ignu-efi/inc -fpie -fno-zero-initialized-in-bss -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/programs/desktop/program.c -o bin/programs/desktop/program.o
ld -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -znoexecstack -o bin/programs/desktop/program.bin --oformat binary bin/programs/desktop/programasm.o bin/programs/desktop/program.o -lgnuefi -lefi
mkdir -p bin/programs/desktop/taskbar/terminal
nasm -f elf64 src/programs/desktop/taskbar/terminal/program.asm -o bin/programs/desktop/taskbar/terminal/programasm.o
gcc -Ignu-efi/inc -fpie -fno-zero-initialized-in-bss -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/programs/desktop/taskbar/terminal/program.c -o bin/programs/desktop/taskbar/terminal/program.o
ld -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -znoexecstack -o bin/programs/desktop/taskbar/terminal/program.bin --oformat binary bin/programs/desktop/taskbar/terminal/programasm.o bin/programs/desktop/taskbar/terminal/program.o -lgnuefi -lefi
mkdir -p bin/programs/desktop/taskbar/rendering
nasm -f elf64 src/programs/desktop/taskbar/rendering/program.asm -o bin/programs/desktop/taskbar/rendering/programasm.o
gcc -Ignu-efi/inc -fpie -fno-zero-initialized-in-bss -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/programs/desktop/taskbar/rendering/program.c -o bin/programs/desktop/taskbar/rendering/program.o
ld -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -znoexecstack -o bin/programs/desktop/taskbar/rendering/program.bin --oformat binary bin/programs/desktop/taskbar/rendering/programasm.o bin/programs/desktop/taskbar/rendering/program.o -lgnuefi -lefi
uefi-run -b OVMF-pure-efi.fd -d -s 20 \
-f bin/system/smp.boot:system/smp.boot \
-f src/fonts/font.psf:fonts/font.psf \
-f bin/programs/test/program.bin:programs/test/program.bin \
-f bin/programs/desktop/program.bin:programs/desktop/program.bin \
-f src/programs/desktop/wallpaper.bmp:programs/desktop/wallpaper.bmp \
-f bin/programs/desktop/taskbar/terminal/program.bin:programs/desktop/taskbar/terminal/program.bin \
-f src/programs/desktop/taskbar/terminal/program.bmp:programs/desktop/taskbar/terminal/program.bmp \
-f bin/programs/desktop/taskbar/rendering/program.bin:programs/desktop/taskbar/rendering/program.bin \
-f src/programs/desktop/taskbar/rendering/program.bmp:programs/desktop/taskbar/rendering/program.bmp \
-f src/programs/desktop/taskbar/rendering/wall.bmp:programs/desktop/taskbar/rendering/wall.bmp \
-f src/programs/desktop/taskbar/rendering/sprite.bmp:programs/desktop/taskbar/rendering/sprite.bmp \
bin/main.efi -- -enable-kvm -m 16G -cpu host -smp 4 -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -nic user,model=e1000e -serial null -serial null -serial stdio -display sdl