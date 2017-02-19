#!/bin/bash
cd build
cmake -G "Unix Makefiles" ..
make
cd ..
echo "The executable is placed in the bin folder"