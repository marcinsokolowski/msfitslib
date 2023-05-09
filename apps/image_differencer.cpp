// program averages few FITS images of the same sizes 

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#include "../src/bg_globals.h"
#include "../src/bg_fits.h"
#include "../src/transient_finder.h"
#include <mystring.h>

#include <vector>
using namespace std;

string list="fits_list";
string out_fits="homeopatic_avg.fits";
string out_rms_fits="out_rms.fits";
string beam_fits_file;
string out_dir="diff/";

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
bool gSubtractHomeopatic=false;
bool gSaveDiffImages=false;

double gThresholdInSigma=-1;

void usage()
{
   printf("image_differencer fits_list OUT_TEMPLATE\n\n\n");
   printf("\t-W WEIGHT_OF_NEW : weight of a new image [default %.6f]\n",gWeigthOfNew);
   printf("\t-H : subtract homeopatic (weighted/running) average (weight specified with -W , default %.3f)\n",gWeigthOfNew);
   printf("\t-t THRESHOLD_IN_SIGMA : find transients above threshold in sigma [default %.3f x sigma], <0 -> do not look for transients at all\n",gThresholdInSigma);
   printf("\t-s SAVE_DIFF_IMAGES [default %d]\n",gSaveDiffImages);
   
   printf("\t-x : enable calculation of max.fits [default %d]\n",gCalcMax);
    printf("\t-r MAX_RMS_ALLOWED\n");   
   printf("\t-w (x_start,y_start)-(x_end,y_end) - do dump dynamic spectra of all pixels in this window\n");
   printf("\t-c RADIUS : calculate RMS around the center in radius of N pixels and RMS-IQR is used [default disabled]\n");
   printf("\t-i : ignore missing FITS files [default %d]\n",int(gIgnoreMissingFITS));
   printf("\t-C (x,y) : center position to calculate RMS around [default not defined]\n");
   printf("\t-S start_fits_index : default %d\n",gStartFitsIndex);
   printf("\t-E end_fits_index   : default %d\n",gEndFitsIndex);
   printf("\t-B BEAM_IMAGE : for weighting the averaged images and calculating an average as < Image_(x,y) > = Sum_over_images Beam(x,y)^2 Image(x,y) / Sum_over_images Beam(x,y)^2\n");
   printf("\t-o OUT_DIR : output directory [default %s]\n",out_dir.c_str());
   
   exit(0);
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "hixr:w:c:C:S:E:B:W:o:Hs";
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

         case 'o' :
            if( optarg ){
               out_dir = optarg;
            }
            break;

         case 't' :
            if( optarg ){
               gThresholdInSigma = atof( optarg );
            }
            break;

         case 's' :
            if( optarg ){
               gSaveDiffImages = (atol( optarg ) > 0);
            }
            break;
            
         // subtraction of homeopatic (running) average with a specified weight for the new image:
         case 'W' :
            if( optarg ){
               gWeigthOfNew = atof( optarg );
            }
            break;

         case 'H' :
            gSubtractHomeopatic = true;
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
    printf("Subtract running average = %d\n",gSubtractHomeopatic);
    printf("Output directory = %s\n",out_dir.c_str());
    printf("Find transient candidates exceeding threshold : %.3f sigma above mean\n",gThresholdInSigma);
    printf("############################################################################################\n");
}

int main(int argc,char* argv[])
{
  string list="fits_list";
  if( argc >= 2 ){
     list = argv[1];
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
  CBgFits prev_fits;
  if( prev_fits.ReadFits( fits_list[gStartFitsIndex].c_str()  , 0, 1, 1 ) ){
     printf("ERROR : could not read first fits file %s on the list\n",fits_list[gStartFitsIndex].c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",fits_list[gStartFitsIndex].c_str());
  }
  
  CBgFits fits( prev_fits.GetXSize(), prev_fits.GetYSize() ), diff_fits( prev_fits.GetXSize(), prev_fits.GetYSize() );
  CBgFits homeopatic_avg( prev_fits.GetXSize(), prev_fits.GetYSize() );

  // starting with the same as the first image
  homeopatic_avg = prev_fits;
  
  // 2014-08-20 was from 1 but I've changed to 0 to average all the files 
  int last_fits = fits_list.size();
  if( gEndFitsIndex < last_fits ){
     last_fits = gEndFitsIndex;
  }
  
  int start_fits = 1;
  if ( gStartFitsIndex > start_fits ){
     start_fits = gStartFitsIndex;
  }
  
  int xSize = prev_fits.GetXSize();
  int ySize = prev_fits.GetYSize();
    
  CTransientFinder transient_finder;    
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
     
     // basename of FITS file will be used in a couple of places :
     string szFitsBaseName;
     getbasename_new( fits_list[i].c_str(), szFitsBaseName );

     
     
     if( gSubtractHomeopatic ){
        fits.Subtract( homeopatic_avg, diff_fits );
     }else{
        fits.Subtract( prev_fits, diff_fits );
     }
     prev_fits = fits;        
     
     // add new image :
     // BUT BE CAREFUL : it should be done after the subtraction to avoid removing fraction of the current image from itself !!!
     for(int y=0;y<ySize;y++){
        for(int x=0;x<xSize;x++){
           double val = homeopatic_avg.getXY(x,y);
           double val_new = fits.getXY(x,y);

           val = val*(1-gWeigthOfNew)  + val_new*gWeigthOfNew;

           homeopatic_avg.setXY(x,y,val);
        }
     }

     
     // saving temporary files
     // const char* getbasename_new(const char* name,string& out)
     if( gSaveDiffImages ){
        char szTmpFits[128];
        sprintf(szTmpFits,"%s/%s_diff.fits",out_dir.c_str(), szFitsBaseName.c_str() );
        if( diff_fits.WriteFits(szTmpFits) ){
           printf("ERROR : could not save FITS file %s\n",szTmpFits);        
        }else{ 
           printf("OK : saved FITS file %s\n",szTmpFits);       
        }
     }
     
     if( gThresholdInSigma > 0 ){
        if( transient_finder.FindTransientCandidates( &diff_fits, szFitsBaseName.c_str(), gThresholdInSigma ) > 0 ){
            // save list of candidates using the same format as in eda2tv library :
            transient_finder.SaveCandidates( szFitsBaseName.c_str() , gThresholdInSigma );
        }
     }
     
     /*sprintf(szTmpFits,"homeo/%05d.fits",i);
     homeopatic_avg.WriteFits(szTmpFits);
     printf("Saved homeopatic average %d to %s\n",i,szTmpFits);*/
  }
  
  homeopatic_avg.WriteFits( out_fits.c_str() );
  printf("Saved final homeopatic average to %s\n",out_fits.c_str() );
}

