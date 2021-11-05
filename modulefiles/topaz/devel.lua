help("Sets up the paths you need to use msfitslib")
local version = '1.0'
local build_options = 'default'
local tool = 'msfitslib'
local root_dir = '/group/director2183/msok/software/centos7.6/development/msfitslib'
whatis( [[The GNU Scientific Library (GSL) is a numerical library for C and C++
programmers. It is free software under the GNU General Public License.

The library provides a wide range of mathematical routines such as random
number generators, special functions and least-squares fitting. There are over
1000 functions in total with an extensive test suite.

For further information see https://www.gnu.org/software/gsl/]] )
whatis( [[Compiled with gcc/8.3.0]] )
whatis( [[Compiled for broadwell]] )
whatis( [[Compiled for cascadelake]] )
conflict("sandybridge")
conflict("ivybridge")
conflict("haswell")
conflict("knl")
conflict("gcc/7.2.0")
conflict("gcc/9.2.0")
conflict("gcc/10.2.0")
conflict("gcc/11.1.0")
conflict("intel/19.0.5")
conflict("pgi/19.7")
conflict("clang/9.0.0")
load("libnova/0.15.0")

if (mode() ~= "whatis") then
setenv("MAALI_COTTERWSCLEAN_HOME",root_dir)
prepend_path("LD_LIBRARY_PATH",root_dir .. "/lib")
prepend_path("PATH",root_dir .. "/bin")
end
