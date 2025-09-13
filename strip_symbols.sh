#!/bin/bash

# Script to strip debug symbols from executables

# Check for Windows executable
WIN_EXE="bin/BotMasterXL.exe"
LINUX_EXE="bin/BotMasterXL"

echo "Stripping debug symbols from executables..."

if [ -f "$WIN_EXE" ]; then
    echo "Original Windows executable size:"
    ls -lh "$WIN_EXE"
    echo "Stripping Windows executable..."
    x86_64-w64-mingw32-strip --strip-debug "$WIN_EXE"
    echo "New Windows executable size:"
    ls -lh "$WIN_EXE"
    echo ""
fi

if [ -f "$LINUX_EXE" ]; then
    echo "Original Linux executable size:"
    ls -lh "$LINUX_EXE"
    echo "Stripping Linux executable..."
    strip --strip-debug "$LINUX_EXE"
    echo "New Linux executable size:"
    ls -lh "$LINUX_EXE"
    echo ""
fi

if [ ! -f "$WIN_EXE" ] && [ ! -f "$LINUX_EXE" ]; then
    echo "No executables found to strip!"
    exit 1
fi

echo "Debug symbols stripped successfully!"