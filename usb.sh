#!/bin/bash
set -e
sudo mkdir -p staging
sudo mount /dev/sdc1 staging
sudo mkdir -p staging/efi/boot
sudo cp bin/main.efi staging/efi/boot/bootx64.efi
sudo mkdir -p staging/system
sudo cp bin/system/smp.bin staging/system/smp.bin
sudo mkdir -p staging/wallpapers
sudo cp src/wallpapers/wallpaper.bmp staging/wallpapers/wallpaper.bmp
sudo mkdir -p staging/fonts
sudo cp src/fonts/font.psf staging/fonts/font.psf
sudo mkdir -p staging/programs/rendering
sudo cp src/programs/rendering/program.bmp staging/programs/rendering/program.bmp
sudo cp bin/programs/rendering/program.bin staging/programs/rendering/program.bin
sudo cp src/programs/rendering/wall.bmp staging/programs/rendering/wall.bmp
sudo cp src/programs/rendering/barrel.bmp staging/programs/rendering/barrel.bmp
sudo umount staging
sudo rm -r staging