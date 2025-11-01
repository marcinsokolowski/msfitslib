// program averages few FITS images of the same sizes 

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#include <bg_globals.h>
#include "bg_fits.h"
#include <mystring.h>

#include <vector>
#include <memory>
using namespace std;

void usage()
{
   printf("read_write_fits_example in.fits out.fits\n\n\n");
   exit(0);
}

int main(int argc,char* argv[])
{
  string in_fits = "start_time_1508442485_coarse_109_real.fits";
  if( argc >= 2 ){
     in_fits = argv[1];
  }
  
  string szOutDir="cube/";
  
  int size = 512;
  CBgFits image(size,size);
  
  fitsfile *fptr;
  int status = 0;
  fits_open_file(&fptr, in_fits.c_str(), READONLY, &status);
  if (status) fits_report_error(stderr, status);
  
  int n_channels=32;
  int n_times=50;
  int n_total=n_channels*n_times;
  long firstpixel[2] = {1, 1};
  int image_type = TFLOAT;
  
  vector<CBgFits*> image_list;
  for(int ch=0;ch<n_channels;ch++){
     image_list.push_back( new CBgFits(size,size) );
  }
  
  for(int i=0;i<n_total;i++){
     int channel = (i % n_channels);
     int timestep = (i / n_channels);
  
     fits_movabs_hdu(fptr, i+1, NULL, &status); 
     if (status) fits_report_error(stderr, status);     
     
     int hdutype;
     fits_get_hdu_type(fptr, &hdutype, &status);
     if (status) fits_report_error(stderr, status);

     if (hdutype == IMAGE_HDU) {
         // Read image data
         printf("Reading image %d\n",i);
         CBgFits* ptr = image_list[channel];
         int fits_read_ret = fits_read_pix(fptr, image_type, firstpixel, size*size, NULL, ptr->get_data(), NULL, &status);
         
         char outfile[256];
         sprintf(outfile,"%s/timestep%05d_channel%05d.fits",szOutDir.c_str(),channel,timestep);
         image.WriteFits(outfile);

         if( channel == 31 ){                     
            sprintf(outfile,"%s/timestep%05d.fits",szOutDir.c_str(),timestep);
            printf("DEBUG : before writting FITS %s\n",outfile);
            CBgFits::WriteMultiImageFits( image_list, outfile );            
            
            // re-create all objects (as something is not ok otherwise) :
            for(int ch=0;ch<n_channels;ch++){
               delete image_list[ch];
               image_list.push_back( new CBgFits(size,size) );
            }
         }
     }else if (hdutype == ASCII_TBL || hdutype == BINARY_TBL) {         
        // Read table data
        printf("Reading table %d\n",i);
     }
  }
  
  fits_close_file(fptr, &status);
  if (status) fits_report_error(stderr, status);

// re-create all objects (as something is not ok otherwise) :
  for(int ch=0;ch<n_channels;ch++){
     delete image_list[ch];
  }
}

