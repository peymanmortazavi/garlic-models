#!/bin/bash

# Check if conan is installed.
if ! [ -x "$(command -v conan)" ]; then
  echo 'Conan is not installed.' >&2
  exit 1
fi

mkdir build
cd build
conan install ..
cmake ..
make
