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
    printf("List file    = %s\n",list.c_str());
    printf("Fits files range : %d - %d\n",gStartFitsIndex,gEndFitsIndex);
    printf("out_fits     = %s\n",out_fits.c_str());
    printf("############################################################################################\n");
}

int main(int argc,char* argv[])
{
  string list="fits_list";
  if( argc >= 2 ){
     list = argv[1];
  }
  string out_file="output.txt";
  if( argc >= 3 ){
     out_file = argv[2];
  }  
  parse_cmdline( argc , argv );
  print_parameters();
 
  
  vector<string> fits_list;
  if( bg_read_list(list.c_str(),fits_list) <= 0 ){
     printf("ERROR : could not read list file %s\n",list.c_str());
     exit(-1);
  }else{
     for(int i=0;i<fits_list.size();i++){
        printf("%i %s\n",i,fits_list[i].c_str());
     }
  }
  
  //  int ReadFits( const char* fits_file=NULL, int bAutoDetect=0, int bReadImage=1, int bIgnoreHeaderErrors=0, bool transposed=false );
  printf("Reading fits file %s ...\n",fits_list[gStartFitsIndex].c_str());
  CBgFits first_fits, first_fits2;
  if( first_fits.ReadFits( fits_list[gStartFitsIndex].c_str()  , 0, 1, 1 ) ){
     printf("ERROR : could not read first fits file %s on the list\n",fits_list[gStartFitsIndex].c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",fits_list[gStartFitsIndex].c_str());
  }
  
  // read the same second time to save RMS 
  if( strlen(out_rms_fits.c_str()) > 0 ){
      if( first_fits2.ReadFits( fits_list[gStartFitsIndex].c_str() , 0, 1, 1 ) ){
         printf("ERROR : could not read first fits file %s on the list\n",fits_list[gStartFitsIndex].c_str());
         exit(-1); 
      }else{
         printf("OK : fits file %s read ok\n",fits_list[gStartFitsIndex].c_str());
      }
  }
  
  int size = first_fits.GetXSize()*first_fits.GetYSize();
  
  FILE* out_f = fopen( out_file.c_str() , "w" );
  CBgFits fits;
  for(int i=0;i<fits_list.size();i++){ // 2019-07-11 - start from the 2nd (1st or 0-based) image 
     if( fits.ReadFits( fits_list[i].c_str() , 0, 1, 1 ) ){
        if( gIgnoreMissingFITS ){
           printf("WARNING : could not read fits file %s on the list, -i option means that it is ignored -> FITS file skipped\n",fits_list[i].c_str());
           continue;
        }else{
           printf("ERROR : could not read fits file %s on the list\n",fits_list[i].c_str());
           exit(-1); 
        }
     }else{
        printf("OK : fits file %s read ok\n",fits_list[i].c_str());
     }
     
     for (int pos=0;pos<size;pos++){
        double val = fits.get_data()[pos];
        
        fprintf(out_f,"%.20f\n",val);
     }
  }
  fclose(out_f);
}

