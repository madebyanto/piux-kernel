#!/bin/bash

echo "Piux Build & Boot Script"
echo "This script speeds up the process of cleaning, compiling, creating images and starting Piux in VM"
echo "v1.0.0"

DISK="disk.img"
ISO="piux.iso"

if [ ! -d "build" ]; then
    mkdir -p build
fi

while true; do
    echo ""
    echo "Select an option:"
    echo "  1) Compile from scratch"
    echo "  2) Boot from an existing disk (VM)"
    echo "  3) Compile from scratch and start from VM disk"
    echo "  4) Exit"
    echo -n "> "
    read -r choice

    case "$choice" in
        1)
            echo "[*] Cleaning..."
            make clean
            echo "[*] Compiling..."
            make
            ;;
        2)
            if [ ! -f "$DISK" ] || [ ! -f "$ISO" ]; then
                echo "[!] Missing $DISK or $ISO. Compile first."
                continue
            fi
            qemu-system-i386 -cdrom "$ISO" -hda "$DISK"
            ;;
        3)
            echo "[*] Cleaning..."
            make clean
            echo "[*] Compiling..."
            make

            echo "[*] Creating disk image..."
            dd if=/dev/zero of="$DISK" bs=1M count=100
            mke2fs "$DISK"

            echo "[*] Starting VM..."
            qemu-system-i386 -cdrom "$ISO" -hda "$DISK"
            ;;
        4)
            echo "Bye!"
            exit 0
            ;;
        *)
            echo "[!] Invalid choice"
            ;;
    esac
done
