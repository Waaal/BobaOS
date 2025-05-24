#!/bin/bash
sudo mount -t vfat cmake-build-debug/os.bin /mnt/d/
sudo cp assets/* /mnt/d
sudo umount /mnt/d