#!/bin/bash

module purge
module load cascadelake
module load gcc/8.3.0
module load fftw/3.3.8 
module load libnova/0.15.0
module load cfitsio/3.48
module load cmake/3.18.0

PROGRAM_NAME=msfitslib
if [ $PAWSEY_CLUSTER = "mwa" ]; then
    echo "Building cotter_wsclean on Garrawarla..."

    
    if [[ -n "$1" && $1 = 'group' ]]; then
        MODULEFILE_DIR=/astro/mwavcs/pacer_blink/software/sles12sp5/modulefiles/$PROGRAM_NAME
        INSTALL_DIR=/astro/mwavcs/pacer_blink/software/sles12sp5/development/$PROGRAM_NAME
    elif [[ -n "$1" && $1 = 'test' ]]; then
        INSTALL_DIR=`pwd`/build
        MODULEFILE_DIR=$INSTALL_DIR/modulefiles/$PROGRAM_NAME
    else
        INSTALL_DIR=/astro/mwavcs/pacer_blink/$USER/software/$PROGRAM_NAME
        MODULEFILE_DIR=/astro/mwavcs/pacer_blink/$USER/software/modulefiles/$PROGRAM_NAME
        
        if [[ -n "$2" && "$2" != "-" ]]; then
            INSTALL_DIR=$2
            MODULEFILE_DIR=$INSTALL_DIR/modulefiles/$PROGRAM_NAME
        fi
        if [[ -n "$3" && "$3" != "-" ]]; then
            MODULEFILE_DIR=$3
        fi
    fi
else
    echo "Building cotter_wsclean on Topaz..."

    # read script parameters
    if [ $# -eq 1 ] && [[ -n $1 && "$1" != "-" ]] && [ $1 = 'group' ]; then
        INSTALL_DIR=/group/director2183/software/centos7.6/development/$PROGRAM_NAME
        MODULEFILE_DIR=/group/director2183/software/centos7.6/modulefiles/$PROGRAM_NAME
        echo "Group installation at $INSTALL_DIR" 
    else
        INSTALL_DIR=/group/director2183/$USER/software/centos7.6/development/$PROGRAM_NAME
        MODULEFILE_DIR=/group/director2183/$USER/software/centos7.6/modulefiles/$PROGRAM_NAME
        echo "User installation at $INSTALL_DIR"
    fi
fi


mkdir -p build/ 
cd build
cmake ..  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
make VERBOSE=1
make INSTALL_DIR=$INSTALL_DIR install

# create modulefile
mkdir -p $MODULEFILE_DIR
echo "
local root_dir = '$INSTALL_DIR'

load('cascadelake')
load('slurm/20.02.3')
load('gcc/8.3.0')
load('fftw/3.3.8')
load('libnova/0.15.0')
load('cfitsio/3.48')

if (mode() ~= 'whatis') then
prepend_path('MSFITSLIB_DIR', root_dir )
prepend_path('PATH', root_dir .. '/bin')
prepend_path('PATH', root_dir .. '/scripts')
prepend_path('CPATH', root_dir .. '/include')
prepend_path('LD_LIBRARY_PATH', root_dir .. '/lib')
end

" > $MODULEFILE_DIR/devel.lua


echo "Build of $PROGRAM_NAME finished at:"
date
echo "Installed $PROGRAM_NAME in $INSTALL_DIR and module in $MODULEFILE_DIR/devel.lua"

