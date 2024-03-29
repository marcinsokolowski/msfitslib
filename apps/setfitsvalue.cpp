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

string list="fits_list";
string out_fits="out.fits";
string out_rms_fits="out_rms.fits";
string beam_fits_file;

double gMinRMSOnSingle  = 0.00001;
double gMaxRMSOnSingle  = 1e20; // default was 4.00, 2023-12-22 -> changed to ~INFINITY to avoid skipping noisy images ; // maximum allowed RMS on 

int gCalcMax=0;

// WINDOW :
int gBorderStartX=-1;
int gBorderStartY=-1;
int gBorderEndX=-1;
int gBorderEndY=-1;
int gCenterRadius = -1;
bool gCalcRMSAroundPixel = false;
bool gIgnoreMissingFITS = false;
int gStartFitsIndex = 0;
int gEndFitsIndex   = 1000000;

void usage()
{
   printf("sum_images fits_list sum.fits\n\n\n");
   exit(0);
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "hixr:w:c:C:S:E:B:";
   int opt;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
//      printf("opt = %c (%s)\n",opt,optarg);   
      switch (opt) {
         case 'h':
            // antenna1 = atol( optarg );
            usage();
            break;

/*         case 'B':
            beam_fits_file = optarg;
            break;
*/

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
    printf("############################################################################################\n");
}

int main(int argc,char* argv[])
{
  string fits_file="fits.fits";
  if( argc >= 2 ){
     fits_file = argv[1];
  }
  string out_file="out.fits";
  if( argc >= 3 ){
     out_file = argv[2];
  }  
  parse_cmdline( argc , argv );
  print_parameters();
 
  
  printf("Reading fits file %s ...\n",fits_file.c_str());
  CBgFits fits;
  if( fits.ReadFits( fits_file.c_str()  , 0, 1, 1 ) ){
     printf("ERROR : could not read first fits file %s on the list\n",fits_file.c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",fits_file.c_str());
  }

  int x_size = fits.GetXSize();
  int y_size = fits.GetYSize();
  int size = fits.GetXSize()*fits.GetYSize();  
  for(int y=0;y<y_size;y++){
     for(int x=0;x<x_size;x++){
        double val = fits.getXY(x,y);
     
        if( fabs(val) > 0.0000001 ){
           fits.setXY(x,y,1.00);
        }
     }
  }
  fits.WriteFits( out_file.c_str() );
}

