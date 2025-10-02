#!/bin/bash
set -e
sudo mkdir -p staging
sudo mount /dev/sdb1 staging
sudo mkdir -p staging/efi/boot
sudo cp bin/main.efi staging/efi/boot/bootx64.efi
sudo mkdir -p staging/system
sudo cp bin/system/smp.boot staging/system/smp.boot
sudo mkdir -p staging/fonts
sudo cp src/fonts/font.psf staging/fonts/font.psf
sudo mkdir -p staging/programs/test
sudo cp bin/programs/test/program.bin staging/programs/test/program.bin
sudo mkdir -p staging/programs/desktop
sudo cp bin/programs/desktop/program.bin staging/programs/desktop/program.bin
sudo cp src/programs/desktop/wallpaper.bmp staging/programs/desktop/wallpaper.bmp
sudo mkdir -p staging/programs/desktop/taskbar/terminal
sudo cp bin/programs/desktop/taskbar/terminal/program.bin staging/programs/desktop/taskbar/terminal/program.bin
sudo cp src/programs/desktop/taskbar/terminal/program.bmp staging/programs/desktop/taskbar/terminal/program.bmp
sudo mkdir -p staging/programs/desktop/taskbar/rendering
sudo cp bin/programs/desktop/taskbar/rendering/program.bin staging/programs/desktop/taskbar/rendering/program.bin
sudo cp src/programs/desktop/taskbar/rendering/program.bmp staging/programs/desktop/taskbar/rendering/program.bmp
sudo cp src/programs/desktop/taskbar/rendering/wall.bmp staging/programs/desktop/taskbar/rendering/wall.bmp
sudo cp src/programs/desktop/taskbar/rendering/sprite.bmp staging/programs/desktop/taskbar/rendering/sprite.bmp
sudo umount staging
sudo rm -r staging