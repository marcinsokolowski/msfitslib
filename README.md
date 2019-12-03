# mscommonlib
Common library with several C++ base classes, which might be useful of required by other projects by Marcin Sokolowski

# required :
   sudo apt-get install libnova-dev libnova fftw2 fftw-dev libnova-0.16-0 libnova-dev fftw3-dev libhdf5-dev


CERN root :   
   https://root.cern.ch/building-root
   https://root.cern.ch/downloading-root

   Can be removed by editing CMakeList.txt and comment out lines :
   list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
   and
   find_package(ROOT REQUIRED COMPONENTS RIO Net)


# installation:

mkdir build

cd build

cmake ..

make

sudo make install
