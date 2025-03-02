#!/bin/bash
set -e
sudo mount /dev/sdc1 /mnt/iso
sudo mkdir -p /mnt/iso/efi/boot
sudo cp bin/main.efi /mnt/iso/efi/boot/bootx64.efi
sudo mkdir -p /mnt/iso/system
sudo cp bin/system/smp.bin /mnt/iso/system/smp.bin
sudo mkdir -p /mnt/iso/wallpapers
sudo cp src/wallpapers/wallpaper.bmp /mnt/iso/wallpapers/wallpaper.bmp
sudo mkdir -p /mnt/iso/fonts
sudo cp src/fonts/font.psf /mnt/iso/fonts/font.psf
sudo mkdir -p /mnt/iso/programs/rendering
sudo cp src/programs/rendering/program.bmp /mnt/iso/programs/rendering/program.bmp
sudo cp bin/programs/rendering/program.bin /mnt/iso/programs/rendering/program.bin
sudo cp src/programs/rendering/wall.bmp /mnt/iso/programs/rendering/wall.bmp
sudo umount /dev/sdc1