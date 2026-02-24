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

double gThreshold = 22.8e9;
bool gSave=false;

void usage()
{
   printf("select_integrations in.fits out.fits -t THRESHOLD -s\n\n\n");
   printf("-s : to save the FITS file with only selected integrations\n");
   exit(0);
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "hst:";
   int opt;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
//      printf("opt = %c (%s)\n",opt,optarg);   
      switch (opt) {
         case 'h':
            // antenna1 = atol( optarg );
            usage();
            break;

         case 's' :
            gSave = true;
            break;
            
         case 't':
            gThreshold = atof(optarg);
            break;

         default:   
            fprintf(stderr,"Unknown option %c\n",opt);
            usage();
      }
   }
}

int main(int argc,char* argv[])
{
  string in_fits = "in.fits";
  if( argc >= 2 ){
     in_fits = argv[1];
  }
  string out_fits_name="out.fits";
  if( argc >= 3 ){
     out_fits_name = argv[2];
  }  
  
  parse_cmdline(argc-2,argv+2);

  printf("Reading fits file %s ...\n",in_fits.c_str());
  CBgFits fits;
  if( fits.ReadFits( in_fits.c_str()  , 0, 1, 1 ) ){
     printf("ERROR : could not read first fits file %s on the list\n",in_fits.c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",in_fits.c_str());
  }
  
  CBgFits out_fits(fits.GetXSize(),fits.GetYSize());

  for(int y=0;y<fits.GetYSize();y++){
     string szResult="OK";
     double total_power = fits.GetTotalPower(y);
     if( total_power >= gThreshold ){
        out_fits.add_line( fits.get_line(y), fits.GetXSize() );
     }else{
        szResult = "SKIPPED";
     }
     
     printf("Y = %d : total_power = %.8f %s\n",y,total_power,szResult.c_str());
  }
  out_fits.set_ysize();
  

  if( gSave ){  
     if( out_fits.WriteFits( out_fits_name.c_str() ) ){
        printf("ERROR : could not write average fits file %s\n",out_fits_name.c_str());
        exit(-1);
     }  
     printf("OK : output fits file %s written ok\n",out_fits_name.c_str());
  }
}

