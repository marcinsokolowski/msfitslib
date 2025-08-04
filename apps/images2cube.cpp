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
   printf("images2cube fits_list cube.fits\n\n\n");
/*   printf("\t-x : enable calculation of max.fits [default %d]\n",gCalcMax);
    printf("\t-r MAX_RMS_ALLOWED\n");   
   printf("\t-w (x_start,y_start)-(x_end,y_end) - do dump dynamic spectra of all pixels in this window\n");
   printf("\t-c RADIUS : calculate RMS around the center in radius of N pixels and RMS-IQR is used [default disabled]\n");
   printf("\t-i : ignore missing FITS files [default %d]\n",int(gIgnoreMissingFITS));
   printf("\t-C (x,y) : center position to calculate RMS around [default not defined]\n");
   printf("\t-S start_fits_index : default %d\n",gStartFitsIndex);
   printf("\t-E end_fits_index   : default %d\n",gEndFitsIndex);
   printf("\t-B BEAM_IMAGE : for weighting the averaged images and calculating an average as < Image_(x,y) > = Sum_over_images Beam(x,y)^2 Image(x,y) / Sum_over_images Beam(x,y)^2\n");
 */  
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

         case 'B':
            beam_fits_file = optarg;
            break;

         case 'i':
            gIgnoreMissingFITS = true;
            break;

         case 'r' :
            gMaxRMSOnSingle = atof( optarg );
            break; 

         case 'c' :
            gCenterRadius = atol( optarg );
            break; 
            
         case 'x':
            gCalcMax = 1;
            break;

         case 'C':
            if( optarg ){
               if( sscanf( optarg,"(%d,%d)",&gBorderStartX,&gBorderStartY )==2 ){
                  printf("RMS center coordinates correctly read , will calculate RMS around pixel (%d,%d)\n",gBorderStartX,gBorderStartY);
                  gCalcRMSAroundPixel = true;
               }else{
                  printf("Pixel %s is not correctly defined, exiting now\n",optarg);
                  exit(0);
               }
            }
            break;

         case 'S' :
            if( optarg ){
               gStartFitsIndex = atol( optarg );
            }
            break;

         case 'E' :
            if( optarg ){
               gEndFitsIndex = atol( optarg );
            }
            break;

         case 'w':
            if( optarg ){
               if( sscanf( optarg,"(%d,%d)-(%d,%d)",&gBorderStartX,&gBorderStartY,&gBorderEndX,&gBorderEndY )==4 ){
                  printf("Window correctly read , will save dynamic spectra of pixels in window (%d,%d) - (%d,%d)\n",gBorderStartX,gBorderStartY,gBorderEndX,gBorderEndY);
               }else{
                  printf("window %s is not correctly defined, exiting now\n",optarg);
                  exit(0);
               }
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
    printf("List file    = %s\n",list.c_str());
    printf("out_fits     = %s\n",out_fits.c_str());
    printf("############################################################################################\n");
}

int main(int argc,char* argv[])
{
  string list="fits_list";
  if( argc >= 2 ){
     list = argv[1];
  }
  out_fits="out.fits";
  if( argc >= 3 ){
     out_fits = argv[2];
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
  
  vector<CBgFits*> image_list;
  
  for(int i=0;i<fits_list.size();i++){
      CBgFits* pFits = new CBgFits();
      
      if( pFits->ReadFits( fits_list[i].c_str() , 0, 1, 1 ) ){
         printf("ERROR : could not read FITS file %s\n",fits_list[i].c_str());
         exit(-1);
      }
      image_list.push_back( pFits );
  }
  
  CBgFits::WriteMultiImageFits( image_list, out_fits.c_str() );
  
  // clean up
  for(int i=0;i<image_list.size();i++){
     if( image_list[i] ){
        CBgFits* pImage = image_list[i];
        delete pImage;
        image_list[i] = NULL;
     }
  }
}
 

