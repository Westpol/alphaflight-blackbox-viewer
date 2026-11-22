#!/bin/bash

set -e

echo "Setting up AlphaFlight Blackbox Viewer..."

# Create directories if they don't exist
mkdir -p bin build

# Configure CMake
cmake -S . -B build

echo ""
echo "Setup complete!"
echo "Run './compile.sh' to compile the project."
