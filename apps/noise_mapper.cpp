#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>

#include <bg_globals.h>
#include <bg_fits.h>
#include <bg_array.h>
#include <bg_bedlam.h>

#include <myfile.h>
#include <mystring.h>

#include <vector>
using namespace std;

// enum eCalcFitsAction_T  {eNone=0,eAdd,eSubtract,eMultiply,eDivide,eSubtractLines,eCompare,eGetStat,eDivideConst, eLog10File, eSqrtFile, eAstroRootImage };

string fits_file="sky.fits";
string fits_out_base="noise_map";
int gRadius=10;
bool gIQR=false;
int gDebugLevel=0;
int gFiducialRadiusAroundCenter = 1000000000;

void usage()
{
   printf("noise_mapper SKY.FITS OUT_FITS_BASE -r %d -i\n",gRadius);
   printf("-h : print this usage message\n");
   printf("-r : radius in pixels to calculate RMS around a given pixel [default %d]\n",gRadius);
   printf("-R : radius around the center to perform the calculations [default %d ~ infinity]\n",gFiducialRadiusAroundCenter);
   printf("-i : use IQR and RMS_IQR= IQR / 1.35 (more time consuming as requires sorting values in the specified radius)\n"); 
   printf("-o FITS_OUT_BASE : there will be 2 files (or more if IQR enabled) with _rms, _mean, and _rmsiqr , _median for -i option\n");
   printf("-d DEBUG_LEVEL [default %d]\n",gDebugLevel);
   /* 
      median = values[ int(values.shape[0]/2) ]
      q75= int(len(values)*0.75);
      q25= int(len(values)*0.25);
      iqr = values[q75] - values[q25] 
      rms_iqr = iqr/1.35;
   */
   exit(-1);
}

void print_parameters()
{
   printf("#####################################\n");
   printf("PARAMETERS :\n");
   printf("#####################################\n");
   printf("FITS  = %s\n",fits_file.c_str());
   printf("OUT_FITS_BASE = %s\n",fits_out_base.c_str());
   printf("Radius = %d\n",gRadius);
   printf("Fiducial radius around center = %d\n",gFiducialRadiusAroundCenter);
   printf("IQR    = %d\n",gIQR);
   printf("Debug level = %d\n",gDebugLevel);
   printf("#####################################\n");   
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "r:ho:id:R:";
   int opt,opt_param,i;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
      switch (opt) {
         case 'd':
            gDebugLevel = atol(optarg);
            break;

         case 'o':
            fits_out_base = optarg;
            break;

         case 'r':
            gRadius = atol( optarg );
            break;

         case 'R':
            gFiducialRadiusAroundCenter = atol( optarg );
            break;
            
         case 'h':
            usage();
            exit(0);
            break;

         case 'i':
            gIQR = true;
            break;
            
         default:  
            fprintf(stderr,"Unknown option %c\n",opt);
            usage();
      }
   }   
}

int main(int argc,char* argv[])                                                        
{
  if( argc<2 || (argc>=2 && (strcmp(argv[1],"-h")==0 || strcmp(argv[1],"--h")==0)) ){
     usage();
  }
  

  fits_file = argv[1];
  
  // parse command line :
  parse_cmdline(argc-1,argv+1);
  print_parameters();
  
  CBgFits in(fits_file.c_str());
  
  printf("Reading file %s ...\n",fits_file.c_str());
  if( in.ReadFits( NULL, 0, 1, 1) ){
     printf("ERROR : error reading fits file %s\n",fits_file.c_str());
     exit(-1);
  }
  
  // 
  char szNoiseRMS[1024],szNoiseMean[1024],szNoiseMedian[1024],szNoiseRMSIQR[1024];
  sprintf(szNoiseRMS,"%s_rms.fits",fits_out_base.c_str());
  sprintf(szNoiseMean,"%s_mean.fits",fits_out_base.c_str());
  sprintf(szNoiseMedian,"%s_median.fits",fits_out_base.c_str());
  sprintf(szNoiseRMSIQR,"%s_rmsiqr.fits",fits_out_base.c_str());
  
  CBgFits noise_rms( szNoiseRMS, in.GetXSize(), in.GetYSize() );
  CBgFits noise_mean( szNoiseMean, in.GetXSize(), in.GetYSize() );
  
  noise_rms.SetKeys( in.GetKeys() );
  noise_mean.SetKeys( in.GetKeys() );
  
  CBgFits* noise_median = NULL;
  CBgFits* noise_rmsiqr = NULL;
  
  if( gIQR ){
     noise_median = new CBgFits( szNoiseMedian, in.GetXSize(), in.GetYSize() );
     noise_rmsiqr = new CBgFits( szNoiseRMSIQR, in.GetXSize(), in.GetYSize() );
     
       noise_rmsiqr->SetKeys( in.GetKeys() );
       noise_median->SetKeys( in.GetKeys() );
  }

  if( !in.MapNoise( noise_mean, noise_rms, gDebugLevel, gRadius, gFiducialRadiusAroundCenter, gIQR, noise_median, noise_rmsiqr ) ){
     printf("ERROR : could not calculate noise map for the current image %s\n",fits_file.c_str());
  }
     
  if( noise_rms.WriteFits( szNoiseRMS ) ){
     printf("ERROR : could not write output file %s\n",szNoiseRMS);
  }                 
  printf("INFO : rms map written to file %s\n",szNoiseRMS);

  if( noise_mean.WriteFits( szNoiseMean ) ){
     printf("ERROR : could not write output file %s\n",szNoiseMean);
  }                 
  printf("INFO : rms map written to file %s\n",szNoiseMean);
  
  if( noise_median ){
     if( noise_median->WriteFits( szNoiseMedian ) ){
        printf("ERROR : could not write output file %s\n",szNoiseMedian);
     }
     printf("INFO : median map written to file %s\n",szNoiseMedian);
  }

  if( noise_rmsiqr ){
     if( noise_rmsiqr->WriteFits( szNoiseRMSIQR ) ){
        printf("ERROR : could not write output file %s\n",szNoiseRMSIQR);
     }
     printf("INFO : rmsiqr map written to file %s\n",szNoiseRMSIQR);
  }
}
