#!/bin/bash

module load fftw/3.3.8 
module load libnova/0.15.0

mkdir -p build/ lib/ bin/
cd build
cmake ..
make

cp libmsfitslib.so* ../lib/

cp ux2sid ../bin/
cp ux2sid_file ../bin/
cp sid2ux ../bin/
cp radec2azh ../bin/
cp avg_images ../bin/

# cp libmscommonlib.so* ../lib/


