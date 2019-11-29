# mscommonlib
Common library with several C++ base classes, which might be useful of required by other projects by Marcin Sokolowski

# required :

libnova : sudo apt-get install libnova-dev libnova

CERN root :   
   https://root.cern.ch/building-root
   https://root.cern.ch/downloading-root

   Can be removed by editing CMakeList.txt and comment out lines :
   # list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
   and
   # find_package(ROOT REQUIRED COMPONENTS RIO Net)


# installation:
mkdir build
cd build
cmake ..
make
sudo make install
