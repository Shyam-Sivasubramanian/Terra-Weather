#!/bin/bash
cd "/mnt/c/Users/shyam/Documents/code/Terra Weather/Terra-Weather"

cd build
make clean
make -j$(nproc)
