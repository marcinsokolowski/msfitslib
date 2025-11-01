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

string szOutDir="cube/";

void usage()
{
   printf("mecube2cubes.cpp in.fits -d OUTDIR/\n\n\n");
   exit(0);
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "hd:";
   int opt;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
//      printf("opt = %c (%s)\n",opt,optarg);   
      switch (opt) {
         case 'h':
            // antenna1 = atol( optarg );
            usage();
            break;

         case 'd':
            if( optarg ){
               szOutDir = optarg;
            }
            break;

         default:   
            fprintf(stderr,"Unknown option %c\n",opt);
            usage();
      }
   }
}

void print_parameters()
{
    printf("############################################################################################\n");
    printf("PARAMETERS :\n");
    printf("############################################################################################\n");
    printf("Outdir = %s\n",szOutDir.c_str());
    printf("############################################################################################\n");
}


int main(int argc,char* argv[])
{
  string in_fits = "start_time_1508442485_coarse_109_real.fits";
  if( argc >= 2 ){
     in_fits = argv[1];
  }
  parse_cmdline(argc,argv);
  print_parameters();
  
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

  char outfile[256];  
  for(int timestep=0;timestep<n_times;timestep++){
     for(int channel=0;channel<n_channels;channel++){
        int i = timestep*n_channels + channel;
        //  for(int i=0;i<n_total;i++){
        // int channel = (i % n_channels);
        // int timestep = (i / n_channels);
  
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
         
            sprintf(outfile,"%s/timestep%05d_channel%05d.fits",szOutDir.c_str(),timestep,channel);
            ptr->WriteFits(outfile);
        }else if (hdutype == ASCII_TBL || hdutype == BINARY_TBL) {         
           // Read table data
           printf("Reading table %d\n",i);
        }
     }   
     
     sprintf(outfile,"%s/timestep%05d.fits",szOutDir.c_str(),timestep);
     printf("DEBUG : before writting FITS %s\n",outfile);
     CBgFits::WriteMultiImageFits( image_list, outfile );

     // re-create all objects (as something is not ok otherwise) :
     // THIS SHOULD NOT BE REQUIRED BUT THERE IS SOME BUG in CBgFits constructors or around :
     for(int ch=0;ch<n_channels;ch++){
        delete image_list[ch];
        image_list.push_back( new CBgFits(size,size) );
     }
  }
  
  fits_close_file(fptr, &status);
  if (status) fits_report_error(stderr, status);

// re-create all objects (as something is not ok otherwise) :
  for(int ch=0;ch<n_channels;ch++){
     delete image_list[ch];
  }
}

