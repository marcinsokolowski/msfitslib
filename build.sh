#!/bin/bash

PROGRAM_NAME=msfitslib

if [ $PAWSEY_CLUSTER = "setonix" ]; then
   echo "Loading modules for PAWSEY_CLUSTER = $PAWSEY_CLUSTER"
   
   module load cfitsio/4.1.0
   module load libnova/0.15.0-hpvrrfr
   module load fftw/3.3.10
else
   echo "Loading modules for PAWSEY_CLUSTER = $PAWSEY_CLUSTER (!= setonix)"

   module purge   
   module load cascadelake
   module load gcc/8.3.0
   module load fftw/3.3.8 
   module load libnova/0.15.0
   module load cfitsio/3.48
   module load cmake/3.18.0
fi

#
cmake_options=""
if [[ -n "$4" && "$4" != "-" ]]; then
   cmake_options="$4"
fi

if [ $PAWSEY_CLUSTER = "mwa" ]; then
    echo "Building msfitslib on Garrawarla..."
    module use /software/projects/mwavcs/msok/modulefiles/
    module load cascadelake/1.0 slurm/20.11.9 gcc/8.3.0 cfitsio/4.3.1 libnova/devel fftw/3.3.8

    
    if [[ -n "$1" && $1 = 'group' ]]; then
#        MODULEFILE_DIR=/astro/mwavcs/pacer_blink/software/sles12sp5/modulefiles/$PROGRAM_NAME
#        INSTALL_DIR=/astro/mwavcs/pacer_blink/software/sles12sp5/development/$PROGRAM_NAME
        INSTALL_DIR=/software/projects/mwavcs/$USER/$PROGRAM_NAME
        MODULEFILE_DIR=/software/projects/mwavcs/$USER/modulefiles/$PROGRAM_NAME
    elif [[ -n "$1" && $1 = 'test' ]]; then
        INSTALL_DIR=`pwd`/build
        MODULEFILE_DIR=$INSTALL_DIR/modulefiles/$PROGRAM_NAME
    else
        INSTALL_DIR=/software/projects/mwavcs/$USER/$PROGRAM_NAME
        MODULEFILE_DIR=/software/projects/mwavcs/$USER/modulefiles/$PROGRAM_NAME
        
        if [[ -n "$2" && "$2" != "-" ]]; then
            INSTALL_DIR=$2
            MODULEFILE_DIR=$INSTALL_DIR/modulefiles/$PROGRAM_NAME
        fi
        if [[ -n "$3" && "$3" != "-" ]]; then
            MODULEFILE_DIR=$3
        fi
    fi
else
    if [ $PAWSEY_CLUSTER = "setonix" ]; then
       echo "Building msfitslib on Setonix ..."
       
       # read script parameters
       if [ $# -eq 1 ] && [ $1 = 'group' ]; then
           INSTALL_DIR=/software/projects/director2183/setonix/software/$PROGRAM_NAME
           MODULEFILE_DIR=/software/projects/director2183/setonix/modules/$PROGRAM_NAME
           echo "Group installation at $INSTALL_DIR" 
       else
           INSTALL_DIR=/software/projects/director2183/msok/setonix/software/$PROGRAM_NAME
           MODULEFILE_DIR=/software/projects/director2183/msok/setonix/modules/$PROGRAM_NAME
           echo "User installation at $INSTALL_DIR"
       fi

    else
       echo "Building msfitslib on Topaz..."

       # read script parameters
       if [ $# -eq 1 ] && [ $1 = 'group' ]; then
           INSTALL_DIR=/group/director2183/software/centos7.6/development/$PROGRAM_NAME
           MODULEFILE_DIR=/group/director2183/software/centos7.6/modulefiles/$PROGRAM_NAME
           echo "Group installation at $INSTALL_DIR" 
       else
           INSTALL_DIR=/group/director2183/$USER/software/centos7.6/development/$PROGRAM_NAME
           MODULEFILE_DIR=/group/director2183/$USER/software/centos7.6/modulefiles/$PROGRAM_NAME
           echo "User installation at $INSTALL_DIR"
       fi
    fi
fi


mkdir -p build/ 
cd build
echo "cmake ..  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ${cmake_options}"
sleep 1
cmake ..  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ${cmake_options}
make VERBOSE=1
make INSTALL_DIR=$INSTALL_DIR install

# create modulefile
if [ $PAWSEY_CLUSTER = "setonix" ]; then
   LOAD_MODULES="load('cascadelake');load('slurm/20.02.3');load('gcc/8.3.0');load('fftw/3.3.8');load('libnova/0.15.0');load('cfitsio/3.48')"
else
   LOAD_MODULES="load('cascadelake/1.0');load('slurm/20.11.9');load('gcc/8.3.0');load('fftw/3.3.8');load('libnova/devel');load('cfitsio/4.3.1')"
fi   
LIBDIR=lib
if [ $PAWSEY_CLUSTER = "setonix" ]; then
   LOAD_MODULES="load('cfitsio/4.0.0');load('libnova/0.15.0-l354muq');load('fftw/3.3.9')"
   LIBDIR=lib64
fi   

mkdir -p $MODULEFILE_DIR
echo "
local root_dir = '$INSTALL_DIR'

$LOAD_MODULES

if (mode() ~= 'whatis') then
prepend_path('MSFITSLIB_DIR', root_dir )
prepend_path('PATH', root_dir .. '/bin')
prepend_path('PATH', root_dir .. '/scripts')
prepend_path('CPATH', root_dir .. '/include')
prepend_path('LD_LIBRARY_PATH', root_dir .. '/$LIBDIR')
end

" > $MODULEFILE_DIR/devel.lua


echo "Build of $PROGRAM_NAME finished at:"
date
echo "Installed $PROGRAM_NAME in $INSTALL_DIR and module in $MODULEFILE_DIR/devel.lua"

