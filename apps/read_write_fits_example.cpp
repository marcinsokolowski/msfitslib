// program averages few FITS images of the same sizes 

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#include <bg_globals.h>
#include "bg_fits.h"
#include <mystring.h>

#include <vector>
using namespace std;

void usage()
{
   printf("read_write_fits_example in.fits out.fits\n\n\n");
   exit(0);
}

int main(int argc,char* argv[])
{
  string in_fits = "in.fits";
  if( argc >= 2 ){
     in_fits = argv[1];
  }
  string out_fits="out.fits";
  if( argc >= 3 ){
     out_fits = argv[2];
  }  

  printf("Reading fits file %s ...\n",in_fits.c_str());
  CBgFits fits;
  if( fits.ReadFits( in_fits.c_str()  , 0, 1, 1 ) ){
     printf("ERROR : could not read first fits file %s on the list\n",in_fits.c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",in_fits.c_str());
  }
  
  if( fits.WriteFits( out_fits.c_str() ) ){
     printf("ERROR : could not write average fits file %s\n",out_fits.c_str());
     exit(-1);
  }  
  printf("OK : output fits file %s written ok\n",out_fits.c_str());
}

