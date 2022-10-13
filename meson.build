# Meson build script from MSFITSLIB
project('msfitslib', 'cpp', version: '1.0.1',
         default_options : ['cpp_std=c++11',])

# Basic setup
cc  = meson.get_compiler('cpp')
builddir = meson.project_build_root()

comp_flags = ['-D_UNIX','-D_ENABLE_PROFILER_', '-D_ENABLE_TRACING_', 
              '-D_ENABLE_VISUALIZATION_', '-D_MONTE_CARLO_', '-D_NO_ROOT_']
add_project_arguments(comp_flags, language : 'cpp')

# Load list of files to compile into shared object
# Note: Meson does not allow * wildcard (deliberately)
# Instead, we use file generated from command: ls src/*.cpp > meson_srclist.txt
fs = import('fs') 
srcs = fs.read('meson_srclist.txt').strip().split('\n')

# Dependencies: FFTW3, CFITSIO, LIBNOVA, LDL, PTHREAD
fftw3_dep = dependency('fftw3')
cfitsio_dep = dependency('cfitsio')
libnova_dep = cc.find_library('libnova', dirs : '/usr/lib/x86_64-linux-gnu') 
ldl_dep     = cc.find_library('dl') 
lpthread_dep = cc.find_library('pthread')

# Compile shared object
msfitslib = library('msfitslib', srcs,              
                     include_directories: 'src',
                     dependencies: [fftw3_dep, cfitsio_dep, libnova_dep, ldl_dep, lpthread_dep],
                     install: true
                    )

# Install headers
install_headers('src/array_config_common.h', 'src/basestring.h', 'src/cvalue_vector.h',
                'src/libnova_interface.h',
                'src/bg_fits.h', 'src/bg_array.h','src/bg_globals.h', 'src/bg_date.h', 
                'src/bg_defines.h', 'src/bg_total_power.h', 
                'src/mystring.h', 'src/myfile.h', 'src/mytypes.h', 'src/basedefines.h',
                'src/mystrtable.h', 'src/mylock.h', 'src/mypipe.h', 'src/mydate.h')

# Create pkg-config file                    
pkg = import('pkgconfig')
pkg.generate(msfitslib)


# Compile apps
apps = [
    'avg_images', 
    'calcfits_bg', 
    'doy2local',
    'libtest',
    'main_fft_file',
    'nan_test',
    'radec2azh',
    'running_median',
    'sid2ux',
    'ux2sid',
    'ux2sid_file',
]

foreach app : apps
  exe = executable(app, 'apps'/app+'.cpp',
                include_directories: 'src',
                dependencies: [fftw3_dep, cfitsio_dep, libnova_dep, ldl_dep, lpthread_dep],
                link_with: msfitslib,
                install: true
                )
endforeach


