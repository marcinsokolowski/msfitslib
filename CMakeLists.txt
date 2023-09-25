cmake_minimum_required(VERSION 3.10.0)
project(msfitslib VERSION 1.0.1)
include(GNUInstallDirs)

file(GLOB msfitslib_sources "src/*.cpp")
file(GLOB msfitslib_headers "src/*.h")
add_library(msfitslib SHARED ${msfitslib_sources})

add_definitions(-D_UNIX -D_ENABLE_PROFILER_ -D_ENABLE_TRACING_ -D_ENABLE_VISUALIZATION_ -D_MONTE_CARLO_ -D_NO_ROOT_ -I${CMAKE_SOURCE_DIR}/src)

set_target_properties(msfitslib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${msfitslib_headers}")
 
configure_file(msfitslib.pc.in msfitslib.pc @ONLY)


install(TARGETS msfitslib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_BINARY_DIR}/msfitslib.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)


find_library(CFITSIO_LIB cfitsio HINTS ENV LD_LIBRARY_PATH)
find_library(FFTW3_LIB fftw3 HINTS ENV FFTW_LIB REQUIRED)
find_library(LIBNOVA_LIB nova PATHS ENV LD_LIBRARY_PATH)


# required to properly link the dynamic library :
target_link_libraries(msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

add_executable(nan_test apps/nan_test.cpp)
add_executable(libtest  apps/libtest.cpp)
target_link_libraries(libtest msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)
add_executable(radec2azh apps/radec2azh.cpp)
target_link_libraries(radec2azh msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)
add_executable(sid2ux apps/sid2ux.cpp)
target_link_libraries(sid2ux msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)
add_executable(ux2sid   apps/ux2sid.cpp) 
target_link_libraries(ux2sid msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)
add_executable(ux2sid_file   apps/ux2sid_file.cpp)
target_link_libraries(ux2sid_file msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)
add_executable(eda_spectrometer   apps/main_fft_file.cpp)
target_link_libraries(eda_spectrometer msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

# larger programs :
add_executable(avg_images  apps/avg_images.cpp)
target_link_libraries(avg_images msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

add_executable(homeopatic_avg  apps/homeopatic_avg.cpp)
target_link_libraries(homeopatic_avg msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

add_executable(image_differencer  apps/image_differencer.cpp)
target_link_libraries(image_differencer msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

add_executable(calcfits_bg  apps/calcfits_bg.cpp)
target_link_libraries(calcfits_bg msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

add_executable(dump_lc  apps/dump_lc/main.cpp apps/dump_lc/lc_table.cpp)
target_link_libraries(dump_lc msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

add_executable(noise_mapper  apps/noise_mapper.cpp)
target_link_libraries(noise_mapper msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

add_executable(date2date  apps/date2date.cpp)
target_link_libraries(date2date msfitslib ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

add_executable(print_sun  apps/print_sun.cpp)
target_link_libraries(print_sun msfitslib ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)


if(FRB_SEARCH)
   add_executable(create_dynaspec apps/frb_search/main_dynaspec.cpp apps/frb_search/mwa_fits.cpp apps/frb_search/sources.cpp apps/frb_search/dedisp_search.cpp apps/frb_search/myprogress.cpp)
   target_link_libraries(create_dynaspec msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

   add_executable(dynaspec_search apps/frb_search/main_test_dynaspec.cpp apps/frb_search/dedisp_search.cpp apps/frb_search/mwa_fits.cpp apps/frb_search/sources.cpp)
   target_link_libraries(dynaspec_search msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

   add_executable(calcstat apps/frb_search/calcstat.cpp apps/frb_search/dedisp_search.cpp apps/frb_search/mwa_fits.cpp apps/frb_search/sources.cpp)
   target_link_libraries(calcstat msfitslib ${CFITSIO_LIB} ${LIBNOVA_LIB} ${ROOT_LIBRARIES} ${FFTW3_LIB} -ldl -lpthread)

   # INSTALLATION:
   install(TARGETS noise_mapper calcfits_bg dump_lc avg_images homeopatic_avg image_differencer ux2sid_file ux2sid sid2ux radec2azh date2date create_dynaspec dynaspec_search RUNTIME DESTINATION bin)
else()
   # INSTALLATION:
   install(TARGETS noise_mapper calcfits_bg dump_lc avg_images homeopatic_avg image_differencer ux2sid_file ux2sid sid2ux radec2azh date2date RUNTIME DESTINATION bin)
endif()
