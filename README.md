# msfitslib 
Common library with several C++ base classes (same as in mscommonlib) and also classes for reading and manipulating FITS files (same as msfitslib_core), which might be useful of required by other projects by Marcin Sokolowski

# required :
   sudo apt-get install libnova-dev fftw2 fftw-dev fftw3-dev libhdf5-dev libcfitsio-dev


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

# possible problems :

   May need addition of -ldl to all target_link_libraries (if does not link) - requires to repeat steps: cmake ../;make 
