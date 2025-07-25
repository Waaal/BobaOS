#!/bin/sh

# OS-Erkennung
OS_NAME="$(uname)"
SCRIPT_PLATFORM_DIR=""
CROSS_PREFIX=""

OS_STRING=""

case "$OS_NAME" in
    "Darwin")
        SCRIPT_PLATFORM_DIR="script/macos"
        OS_STRING='MacOS'
        ;;
    "Linux")
        SCRIPT_PLATFORM_DIR="script/linux"
        CROSS_PREFIX="$HOME/opt/cross64/bin/"
        OS_STRING="Linux"
        ;;
    *)
        echo "Unsupported OS: $OS_NAME"
        exit 1
        ;;
esac

echo "Detected platform: $OS_NAME"
echo "Using script directory: $SCRIPT_PLATFORM_DIR"
echo "Using cross compiler path: $CROSS_PREFIX x86_64-elf-gcc"

# CMake initialisieren, übergibt die Plattform-Variable
cmake -B build -S . -DSCRIPT_PLATFORM_DIR="$SCRIPT_PLATFORM_DIR" -DCROSS_PREFIX="$CROSS_PREFIX"

# Copy debug.sh and boot.sh
echo "Copy boot and debug to main directory..."

cp "$SCRIPT_PLATFORM_DIR/boot.sh" ./boot.sh
cp "$SCRIPT_PLATFORM_DIR/debug.sh" ./debug.sh

# Give execute rights
chmod +x ./boot.sh ./debug.sh

echo "Setup complete for $OS_STRING."

