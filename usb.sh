#!/bin/bash
set -e
sudo mount /dev/sdc1 /mnt/iso
sudo cp main.efi /mnt/iso/EFI/Boot/bootx64.efi
sudo cp font.psf /mnt/iso/font.psf
sudo umount /dev/sdc1