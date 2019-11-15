cmake_minimum_required(VERSION 3.9)
project(msfitslib VERSION 1.0.1 DESCRIPTION "msfitslib description")
include(GNUInstallDirs)
add_library(msfitslib SHARED 
     baseanal.cpp basedefines.cpp basestring.cpp basestructs.cpp ccdrowcollist.cpp cexcp.cpp cfgfile.cpp cmncfg.cpp cmn_tmpl.cpp laplace_info.cpp mybaseserver.cpp mybits.cpp mycmnglobals.cpp mydate2.cpp 
     mydate.cpp myenv.cpp myfastlongtab.cpp myfile.cpp myfract.cpp myframeslist.cpp myhisto2D.cpp myhisto.cpp myinifile.cpp mykeytab.cpp mylibloader.cpp mylock.cpp mymatrix.cpp mymsgqueue.cpp mynet.cpp 
     myparser.cpp mypipe.cpp mypixellist.cpp mypoints.cpp myprogress.cpp myranges.cpp myrequestdef.cpp mysafekeytab.cpp myshmem.cpp mysignal.cpp mystring.cpp mystrtable.cpp mytrace.cpp mytxtclient.cpp 
     mytxtinetserver.cpp mytxtserver.cpp  myutil.cpp myvalcounter.cpp paramtab.cpp random.cpp tab2Ddesc.cpp templ.cpp 
     gendistr.cpp calcrot.cpp mathfunc.cpp myfits.cpp mygraphcut.cpp vector.cpp
     ccddriver_interface.cpp
     bg_bedlam.cpp  bg_fits.cpp  bg_globals.cpp  bg_stat.cpp         bg_units.cpp  calsol_values.cpp weather_station.cpp
     bg_array.cpp  bg_date.cpp    bg_geo.cpp   bg_norm.cpp     bg_total_power.cpp  bg_vis.cpp    cvalue_vector.cpp  libnova_interface.cpp  
     spectrometer.cpp
           )

add_definitions(-D_UNIX -D_ENABLE_PROFILER_ -D_ENABLE_TRACING_ -D_ENABLE_VISUALIZATION_ -D_MONTE_CARLO_ -D_NO_ROOT_ )
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


# EXACUTABLES :
find_library(CFITSIO_LIB cfitsio)
find_library(LIBNOVA_LIB nova)
find_path(FITSIO_INCLUDE_DIR NAMES fitsio.h)
find_library(FFTW3_LIB fftw3 REQUIRED)
include_directories(${FITSIO_INCLUDE_DIR})

# You need to tell CMake where to find the ROOT installation. This can be done in a number of ways:
#   - ROOT built with classic configure/make use the provided $ROOTSYS/etc/cmake/FindROOT.cmake
#   - ROOT built with CMake. Add in CMAKE_PREFIX_PATH the installation prefix for ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
find_package(ROOT REQUIRED COMPONENTS RIO Net)

#---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
# include(${ROOT_USE_FILE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


add_executable(nan_test nan_test.cpp)
add_executable(libtest  libtest.cpp)
target_link_libraries(libtest msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB})
add_executable(radec2azh radec2azh.cpp)
target_link_libraries(radec2azh msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB})
add_executable(sid2ux sid2ux.cpp)
target_link_libraries(sid2ux msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB})
add_executable(ux2sid   ux2sid.cpp) 
target_link_libraries(ux2sid msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB})
add_executable(ux2sid_file   ux2sid_file.cpp)
target_link_libraries(ux2sid_file msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB})

