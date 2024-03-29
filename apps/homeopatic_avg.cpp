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
string out_fits="homeopatic_avg.fits";
string out_rms_fits="out_rms.fits";
string beam_fits_file;

double gMinRMSOnSingle  = 0.00001;
double gMaxRMSOnSingle  = 4.00; // maximum allowed RMS on 

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
double gWeigthOfNew=0.5;

void usage()
{
   printf("homeopatic_avg fits_list OUT_TEMPLATE\n\n\n");
   printf("\t-W WEIGHT_OF_NEW : weight of a new image [default %.6f]\n",gWeigthOfNew);
   printf("\t-x : enable calculation of max.fits [default %d]\n",gCalcMax);
    printf("\t-r MAX_RMS_ALLOWED\n");   
   printf("\t-w (x_start,y_start)-(x_end,y_end) - do dump dynamic spectra of all pixels in this window\n");
   printf("\t-c RADIUS : calculate RMS around the center in radius of N pixels and RMS-IQR is used [default disabled]\n");
   printf("\t-i : ignore missing FITS files [default %d]\n",int(gIgnoreMissingFITS));
   printf("\t-C (x,y) : center position to calculate RMS around [default not defined]\n");
   printf("\t-S start_fits_index : default %d\n",gStartFitsIndex);
   printf("\t-E end_fits_index   : default %d\n",gEndFitsIndex);
   printf("\t-B BEAM_IMAGE : for weighting the averaged images and calculating an average as < Image_(x,y) > = Sum_over_images Beam(x,y)^2 Image(x,y) / Sum_over_images Beam(x,y)^2\n");
   
   exit(0);
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "hixr:w:c:C:S:E:B:W:";
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

         case 'W' :
            if( optarg ){
               gWeigthOfNew = atof( optarg );
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
   
   // validate parameters :
   if( gWeigthOfNew>1 || gWeigthOfNew<0 ){
      printf("ERROR : invalid value of weight (%.6f) for the new image. It has to be in the range [0,1]\n",gWeigthOfNew);
      exit(-1);
   }
}

void print_parameters()
{
    printf("############################################################################################\n");
    printf("PARAMETERS :\n");
    printf("############################################################################################\n");
    printf("List file    = %s\n",list.c_str());
    printf("Weight of the new image = %.6f\n",gWeigthOfNew);
    printf("############################################################################################\n");
}

int main(int argc,char* argv[])
{
  string list="fits_list";
  if( argc >= 2 ){
     list = argv[1];
  }
  if( argc >= 3 ){
     out_fits = argv[2];
  }  
  
//  if( argc >= 5 ){
//     if( atol( argv[4] ) <= 0 ){
//        out_rms_fits = "";
//     }
//  }
  
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
  CBgFits first_fits;
  if( first_fits.ReadFits( fits_list[gStartFitsIndex].c_str()  , 0, 1, 1 ) ){
     printf("ERROR : could not read first fits file %s on the list\n",fits_list[gStartFitsIndex].c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",fits_list[gStartFitsIndex].c_str());
  }
  
  CBgFits fits;
  CBgFits homeopatic_avg( first_fits.GetXSize(), first_fits.GetYSize() );

  // starting with the same as the first image
  homeopatic_avg = first_fits;
  
  // 2014-08-20 was from 1 but I've changed to 0 to average all the files 
  int last_fits = fits_list.size();
  if( gEndFitsIndex < last_fits ){
     last_fits = gEndFitsIndex;
  }
  
  int start_fits = 1;
  if ( gStartFitsIndex > start_fits ){
     start_fits = gStartFitsIndex;
  }
  
  int xSize = first_fits.GetXSize();
  int ySize = first_fits.GetYSize();
    
  for(int i=start_fits;i<last_fits;i++){ // 2019-07-11 - start from the 2nd (1st or 0-based) image 
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
     
     
     // add new image :
     for(int y=0;y<ySize;y++){
        for(int x=0;x<xSize;x++){
           double val = homeopatic_avg.getXY(x,y);
           double val_new = fits.getXY(x,y);
           
           val = val*(1-gWeigthOfNew)  + val_new*gWeigthOfNew;
           
           homeopatic_avg.setXY(x,y,val);
        }
     }
     
     // saving temporary files
     char szTmpFits[128];
     sprintf(szTmpFits,"homeo/%05d.fits",i);
     homeopatic_avg.WriteFits(szTmpFits);
     printf("Saved homeopatic average %d to %s\n",i,szTmpFits);
  }
  
  homeopatic_avg.WriteFits( out_fits.c_str() );
  printf("Saved final homeopatic average to %s\n",out_fits.c_str() );
}

