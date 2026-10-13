#!/bin/bash
# Compiles the Swift helpers into tools/test/bin (macOS only, needs Xcode's swiftc).
set -e
cd "$(dirname "$0")"
mkdir -p bin
for tool in keys winid strip; do
  swiftc -O -o "bin/$tool" "$tool.swift"
done
echo "built: $(ls bin | tr '\n' ' ')"
