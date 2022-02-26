// program averages few FITS images of the same sizes 

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#include <bg_globals.h>
#include "bg_fits.h"
#include <mystring.h>
#include <myfile.h>

#include <vector>
using namespace std;

#include "lc_table.h"

string list="fits_list";
string gOutDir="lc/";

//double gMinRMSOnSingle  = 0.00001;
double gMaxRMSOnSingle  = 4.00; // maximum allowed RMS on 

bool gIgnoreMissingFITS = false;

// WINDOW :
bool gUseBorder = false;
int gBorderStartX=0;
int gBorderStartY=0;
int gBorderEndX=-1;
int gBorderEndY=-1;
int gCenterRadius = -1;
bool gCalcRMSAroundPixel = false;

// 
bool gFast=true; // use in-memory version

void usage()
{
   printf("dump_lc fits_list\n");
   printf("\n\nVersion : with saving RMS/MODIDX map\n\n");
   
   printf("\t-r MAX_RMS_ALLOWED\n");   
   printf("\t-w (x_start,y_start)-(x_end,y_end) - do dump dynamic spectra of all pixels in this window\n");
   printf("\t-r MAX_RMS on image [default %.4f]\n",gMaxRMSOnSingle);
   printf("\t-s : slow (old legacy) version for comparisons\n");
   printf("\t-o OUTDIR : name of output directory [default %s]\n",gOutDir.c_str());
   printf("\t-i MIN_MODULATION_INDEX : minimu modulation index to save lightcurve [default %.4f]\n",CLcTable::m_MinModulationIndex);
   
   exit(0);
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "hr:w:so:i:";
   int opt;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
//      printf("opt = %c (%s)\n",opt,optarg);   
      switch (opt) {
         case 'h':
            usage();
            break;

         case 'i' :
            if( optarg ){
               CLcTable::m_MinModulationIndex = atof( optarg );
            }
            break;

         case 's' :
            gFast = false;
            break;
            
         case 'r' :
            gMaxRMSOnSingle = atof( optarg );
            break; 

         case 'o' :
            if( optarg ){
               gOutDir = optarg;
            }
            break; 

         case 'w':
            if( optarg ){
               if( sscanf( optarg,"(%d,%d)-(%d,%d)",&gBorderStartX,&gBorderStartY,&gBorderEndX,&gBorderEndY )==4 ){
                  printf("Window correctly read , will save lightcurves of all pixels in window (%d,%d) - (%d,%d). If not specified all pixels in the image are used\n",gBorderStartX,gBorderStartY,gBorderEndX,gBorderEndY);
                  gUseBorder = true;
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

bool generate_lc_in_memory( vector<string>& fits_list )
{
   if( fits_list.size() <= 0 ){
      printf("ERROR : no FITS files provided for the lightcurve generation\n");
      return false;
   }

   CBgFits first_fits;
   if( first_fits.ReadFits( fits_list[0].c_str() ) ){
      printf("ERROR : could not read the first FITS file %s\n",fits_list[0].c_str());
      return false;
   }
   
   CLcTable lc_table( first_fits.GetXSize(), first_fits.GetYSize() );
   // lc_table.Alloc( first_fits.GetXSize(), first_fits.GetYSize() );
   
  for(int i=0;i<fits_list.size();i++){
     CBgFits fits; // if not here field dtime_fs needs to be set to -1 as otherwise it will be the same for all the FITS files 
     fits.dtime_fs = -1000;
     
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
     if( gBorderEndX <= 0 ){
        gBorderEndX = fits.GetXSize();
     }
     if( gBorderEndY <= 0 ){
        gBorderEndY = fits.GetYSize();
     }     
     double uxtime = fits.GetUnixTime();
     printf("DEBUG_UXTIME : %s -> %.4f\n",fits_list[i].c_str() , uxtime );
     
     // check image :
     double mean, rms, minval, maxval;
     double mean_center, rms_center, minval_center, maxval_center, median_center, iqr_center, rms_iqr_center;
     
     if( gCenterRadius > 0 ){
//        printf("DEBUG : checking stat in radius %d pixels around center\n",gCenterRadius);
//        fits.GetStatRadius( mean, rms, minval, maxval, gCenterRadius );
        fits.GetStatRadiusAll( mean_center, rms_center, minval_center, maxval_center, median_center, iqr_center, rms_iqr_center, gCenterRadius, true, gBorderStartX, gBorderStartY );
        printf("%s : at CENTER mean stat = %.8f, rms = %.8f, min_val = %.8f, max_val = %.8f , median = %.8f, rms_iqr = %.8f\n",fits_list[i].c_str(),mean_center, rms_center, minval_center, maxval_center, median_center, rms_iqr_center );
     }else{
        if( gBorderStartX>0 && gBorderStartY>0 && gBorderEndX>0 && gBorderEndY>0 && gUseBorder ){
           printf("DEBUG : Checking stat in the window (%d,%d) - (%d,%d)\n",gBorderStartX,gBorderStartY,gBorderEndX,gBorderEndY );
           fits.GetStat( mean, rms, minval, maxval, gBorderStartX, gBorderStartY, gBorderEndX, gBorderEndY );

           // setting the other variables to just use the center ones, 
           // in the future I may want to impose a criteria on both            
        }else{
           fits.GetStatBorder( mean, rms, minval, maxval, 5 );
        }
        
        mean_center = mean;
        rms_center  = rms;
        minval_center = minval;
        maxval_center = maxval;
        median_center = mean;
        rms_iqr_center = rms;
        iqr_center    = rms*1.35;
     }
     
     if( rms_iqr_center > gMaxRMSOnSingle ){
        printf("WARNING : image %s skipped due to RMS_IQR = %.4f > limit = %.4f\n",fits_list[i].c_str(),rms_iqr_center,gMaxRMSOnSingle);
        continue;
     }

     // only dump pixels in a specified Window :
     for(int y=gBorderStartY;y<gBorderEndY;y++){
        for(int x=gBorderStartX;x<gBorderEndX;x++){
           double value = fits.getXY(x,y);
           
           lc_table.setXY(x,y,uxtime,value);                              
        }
     }
   }

   // save lightcurves :
   lc_table.SaveLC( gOutDir.c_str(), gBorderStartX, gBorderStartY, gBorderEndX, gBorderEndY );   
}

void generate_lc_slow( vector<string>& fits_list )
{
  for(int i=0;i<fits_list.size();i++){
     CBgFits fits; // if not here field dtime_fs needs to be set to -1 as otherwise it will be the same for all the FITS files 
     fits.dtime_fs = -1000;
     
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
     if( gBorderEndX <= 0 ){
        gBorderEndX = fits.GetXSize();
     }
     if( gBorderEndY <= 0 ){
        gBorderEndY = fits.GetYSize();
     }     
     double uxtime = fits.GetUnixTime();
     
     // check image :
     double mean, rms, minval, maxval;
     double mean_center, rms_center, minval_center, maxval_center, median_center, iqr_center, rms_iqr_center;
     
     if( gCenterRadius > 0 ){
//        printf("DEBUG : checking stat in radius %d pixels around center\n",gCenterRadius);
//        fits.GetStatRadius( mean, rms, minval, maxval, gCenterRadius );
        fits.GetStatRadiusAll( mean_center, rms_center, minval_center, maxval_center, median_center, iqr_center, rms_iqr_center, gCenterRadius, true, gBorderStartX, gBorderStartY );
        printf("%s : at CENTER mean stat = %.8f, rms = %.8f, min_val = %.8f, max_val = %.8f , median = %.8f, rms_iqr = %.8f\n",fits_list[i].c_str(),mean_center, rms_center, minval_center, maxval_center, median_center, rms_iqr_center );
     }else{
        if( gBorderStartX>0 && gBorderStartY>0 && gBorderEndX>0 && gBorderEndY>0 && gUseBorder ){
           printf("DEBUG : Checking stat in the window (%d,%d) - (%d,%d)\n",gBorderStartX,gBorderStartY,gBorderEndX,gBorderEndY );
           fits.GetStat( mean, rms, minval, maxval, gBorderStartX, gBorderStartY, gBorderEndX, gBorderEndY );

           // setting the other variables to just use the center ones, 
           // in the future I may want to impose a criteria on both            
        }else{
           fits.GetStatBorder( mean, rms, minval, maxval, 5 );
        }
        
        mean_center = mean;
        rms_center  = rms;
        minval_center = minval;
        maxval_center = maxval;
        median_center = mean;
        rms_iqr_center = rms;
        iqr_center    = rms*1.35;
     }
     
     // only dump pixels in a specified Window :
     char szOutFileName[512];
     for(int y=gBorderStartY;y<gBorderEndY;y++){
        for(int x=gBorderStartX;x<gBorderEndX;x++){
           // dump value to a file or store in memory 
           sprintf(szOutFileName,"%s/pixel_%05d_%05d.txt",gOutDir.c_str(),x,y);
           
           // TODO : add class to store these in memory and write in a single loop - will be much faster !
           // TODO : test on Susmita's lightcurves for J0034-0534 
           MyOFile out_f(szOutFileName,"a+");
           out_f.Printf("%.4f %.4f\n",uxtime,fits.getXY(x,y));           
        }
     }

   }

}

void print_parameters()
{
    printf("############################################################################################\n");
    printf("PARAMETERS :\n");
    printf("############################################################################################\n");
    printf("List file        = %s\n",list.c_str());
    printf("Output directory = %s\n",gOutDir.c_str());
    printf("Min. mod. index  = %.8f\n",CLcTable::m_MinModulationIndex);
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
  

  if( gFast ){
     generate_lc_in_memory( fits_list );
  }else{
     generate_lc_slow( fits_list );
  }
}

