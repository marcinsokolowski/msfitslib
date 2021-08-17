cmake_minimum_required(VERSION 3.9)
project(msfitslib VERSION 1.0.1 DESCRIPTION "msfitslib description")
include(GNUInstallDirs)
add_library(msfitslib SHARED 
     src/baseanal.cpp src/basedefines.cpp src/basestring.cpp src/basestructs.cpp src/ccdrowcollist.cpp src/cexcp.cpp src/cfgfile.cpp src/cmncfg.cpp src/cmn_tmpl.cpp
     src/laplace_info.cpp src/mybaseserver.cpp src/mybits.cpp src/mycmnglobals.cpp src/mydate2.cpp src/mydate.cpp src/myenv.cpp src/myfastlongtab.cpp src/myfile.cpp src/myfract.cpp 
     src/myframeslist.cpp src/myhisto2D.cpp src/myhisto.cpp src/myinifile.cpp src/mykeytab.cpp src/mylibloader.cpp src/mylock.cpp src/mymatrix.cpp src/mymsgqueue.cpp src/mynet.cpp 
     src/myparser.cpp src/mypipe.cpp src/mypixellist.cpp src/mypoints.cpp src/myprogress.cpp src/myranges.cpp src/myrequestdef.cpp src/mysafekeytab.cpp src/myshmem.cpp src/mysignal.cpp 
     src/mystring.cpp src/mystrtable.cpp src/mytrace.cpp src/mytxtclient.cpp src/mytxtinetserver.cpp src/mytxtserver.cpp  src/myutil.cpp src/myvalcounter.cpp
     src/paramtab.cpp src/random.cpp src/tab2Ddesc.cpp src/templ.cpp  src/gendistr.cpp src/calcrot.cpp src/mathfunc.cpp src/myfits.cpp src/mygraphcut.cpp src/vector.cpp  src/ccddriver_interface.cpp
     src/bg_bedlam.cpp  src/bg_fits.cpp  src/bg_globals.cpp  src/bg_stat.cpp src/bg_units.cpp  src/calsol_values.cpp src/weather_station.cpp
     src/bg_array.cpp  src/bg_date.cpp    src/bg_geo.cpp   src/bg_norm.cpp   src/bg_total_power.cpp  src/bg_vis.cpp src/cvalue_vector.cpp  src/libnova_interface.cpp  
     src/spectrometer.cpp
           )

add_definitions(-D_UNIX -D_ENABLE_PROFILER_ -D_ENABLE_TRACING_ -D_ENABLE_VISUALIZATION_ -D_MONTE_CARLO_ -D_NO_ROOT_ -I../src/)
set_target_properties(msfitslib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER   "msfitslib.h;spectrometer.h;bg_date.h;bg_globals.h;cvalue_vector.h;calsol_values.h;myparser.h;mystring.h;mystrtable.h;mytypes.h;basedefines.h;basestring.h;myfile.h;basestructs.h")
configure_file(msfitslib.pc.in msfitslib.pc @ONLY)
target_include_directories(msfitslib PRIVATE .)
install(TARGETS msfitslib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_BINARY_DIR}/msfitslib.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

find_package(PkgConfig REQUIRED)

# EXACUTABLES :
#find_library(CFITSIO_LIB cfitsio)
pkg_search_module(CFITSIO_LIB REQUIRED cfitsio IMPORTED_TARGET)
include_directories(PkgConfig::CFITSIO_LIB)
link_libraries     (PkgConfig::CFITSIO_LIB)
#
# find_library(LIBNOVA_LIB nova)
# find_path(FITSIO_INCLUDE_DIR NAMES fitsio.h)

pkg_search_module(FFTW REQUIRED fftw3 IMPORTED_TARGET)
include_directories(PkgConfig::FFTW)
link_libraries     (PkgConfig::FFTW)
# find_library(FFTW3_LIB fftw3 HINTS ${FFTW_LIB} REQUIRED)
include_directories(${FITSIO_INCLUDE_DIR})

# EXACUTABLES :
# pkg_search_module(LIBNOVA_LIB REQUIRED libnova IMPORTED_TARGET)
# include_directories(PkgConfig::LIBNOVA_LIB)
# link_libraries     (PkgConfig::LIBNOVA_LIB)
find_library(LIBNOVA_LIB nova PATHS ENV LD_LIBRARY_PATH)


# You need to tell CMake where to find the ROOT installation. This can be done in a number of ways:
#   - ROOT built with classic configure/make use the provided $ROOTSYS/etc/cmake/FindROOT.cmake
#   - ROOT built with CMake. Add in CMAKE_PREFIX_PATH the installation prefix for ROOT
#list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
#find_package(ROOT REQUIRED COMPONENTS RIO Net)

#---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
# include(${ROOT_USE_FILE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


add_executable(nan_test src/nan_test.cpp)
add_executable(libtest  src/libtest.cpp)
target_link_libraries(libtest msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB})
add_executable(radec2azh src/radec2azh.cpp)
target_link_libraries(radec2azh msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB})
add_executable(sid2ux src/sid2ux.cpp)
target_link_libraries(sid2ux msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB})
add_executable(ux2sid   src/ux2sid.cpp) 
target_link_libraries(ux2sid msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB})
add_executable(ux2sid_file   src/ux2sid_file.cpp)
target_link_libraries(ux2sid_file msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB})

