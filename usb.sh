#!/bin/bash
set -e
sudo mount /dev/sdc1 /mnt/iso
sudo cp bin/main.efi /mnt/iso/EFI/Boot/bootx64.efi
sudo cp src/font.psf /mnt/iso/font.psf
sudo umount /dev/sdc1