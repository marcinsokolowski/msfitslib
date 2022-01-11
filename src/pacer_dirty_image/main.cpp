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

string in_basename="1276619416_20200619164456";
string gPostfix="";
string out_fits="1276619416_20200619164456_dirty_image.fits";
// string out_rms_fits="out_rms.fits";
string beam_fits_file;

int n_pixels = 512;

/*double gMinRMSOnSingle  = 0.00001;
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
int gEndFitsIndex   = 1000000;*/

void usage()
{
   printf("pacer_dirty_image VISIBILITY_FITS_BASENAME\n\n\n");
   
   printf("\t-p POSTFIX : default is not postfix\n");

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
   char optstring[] = "hixr:w:c:C:S:E:B:p:";
   int opt;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
//      printf("opt = %c (%s)\n",opt,optarg);   
      switch (opt) {
         case 'h':
            // antenna1 = atol( optarg );
            usage();
            break;

         case 'p':
            gPostfix = optarg;
            break;

/*         case 'B':
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
    printf("Base name for FITS  = %s\n",in_basename.c_str());
    printf("out_fits     = %s\n",out_fits.c_str());
    printf("Postfix      = %s\n",gPostfix.c_str());
    printf("############################################################################################\n");
}

int main(int argc,char* argv[])
{
  if( argc >= 2 ){
     in_basename = argv[1];
  }
  out_fits="out.fits";
  if( argc >= 3 ){
     out_fits = argv[2];
  }  
//  out_rms_fits="out_rms.fits";
//  if( argc >= 4 ){
//     out_rms_fits = argv[3];
//  }
  
  parse_cmdline( argc , argv );
  print_parameters();

  // creating FITS file names for REAL, IMAG and U,V,W input FITS files :
  string fits_file_real = in_basename.c_str();
  fits_file_real += "_vis_real";
  if( strlen( gPostfix.c_str() ) ){
     fits_file_real += gPostfix.c_str();
  }
  fits_file_real += ".fits";
 
  string fits_file_imag = in_basename.c_str();
  fits_file_imag += "_vis_imag";
  if( strlen( gPostfix.c_str() ) ){
     fits_file_imag += gPostfix.c_str();
  }
  fits_file_imag += ".fits";
 
  string fits_file_u = in_basename.c_str();
  fits_file_u += "_u";
  if( strlen( gPostfix.c_str() ) ){
     fits_file_u += gPostfix.c_str();
  }
  fits_file_u += ".fits";
 
  string fits_file_v = in_basename.c_str();
  fits_file_v += "_v";
  if( strlen( gPostfix.c_str() ) ){
     fits_file_v += gPostfix.c_str();
  }
  fits_file_v += ".fits";
 
  string fits_file_w = in_basename.c_str();
  fits_file_w += "_w";
  if( strlen( gPostfix.c_str() ) ){
     fits_file_w += gPostfix.c_str();
  }
  fits_file_w += ".fits";


  printf("Expecting the following files to exist:\n");
  printf("\t%s\n",fits_file_real.c_str()); 
  printf("\t%s\n",fits_file_imag.c_str()); 
  printf("\t%s\n",fits_file_u.c_str()); 
  printf("\t%s\n",fits_file_v.c_str()); 
  printf("\t%s\n",fits_file_w.c_str()); 
  
  CBgFits fits_vis_real, fits_vis_imag, fits_vis_u, fits_vis_v, fits_vis_w;
  
  // REAL(VIS)
  printf("Reading fits file %s ...\n",fits_file_real.c_str());
  if( fits_vis_real.ReadFits( fits_file_real.c_str(), 0, 1, 1 ) ){
     printf("ERROR : could not read visibility FITS file %s\n",fits_file_real.c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",fits_file_real.c_str());
  }

  // IMAG(VIS)
  printf("Reading fits file %s ...\n",fits_file_imag.c_str());
  if( fits_vis_imag.ReadFits( fits_file_imag.c_str(), 0, 1, 1 ) ){
     printf("ERROR : could not read visibility FITS file %s\n",fits_file_imag.c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",fits_file_imag.c_str());
  }

  // U : 
  double u_mean, u_rms, u_min, u_max;
  printf("Reading fits file %s ...\n",fits_file_u.c_str());
  if( fits_vis_u.ReadFits( fits_file_u.c_str(), 0, 1, 1 ) ){
     printf("ERROR : could not read U FITS file %s\n",fits_file_u.c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",fits_file_u.c_str());
  }
  fits_vis_u.GetStat( u_mean, u_rms, u_min, u_max );
  double delta_u = (u_max - u_min) / n_pixels;
  printf("DEBUG : U limits %.8f - %.8f -> delta_u = %.8f\n",u_min, u_max , delta_u );
  
  // V : 
  double v_mean, v_rms, v_min, v_max;
  printf("Reading fits file %s ...\n",fits_file_v.c_str());
  if( fits_vis_v.ReadFits( fits_file_v.c_str(), 0, 1, 1 ) ){
     printf("ERROR : could not read V FITS file %s\n",fits_file_v.c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",fits_file_v.c_str());
  }
  fits_vis_v.GetStat( v_mean, v_rms, v_min, v_max );
  double delta_v = (v_max - v_min) / n_pixels;
  printf("DEBUG : V limits %.8f - %.8f -> delta_v = %.8f\n", v_min, v_max , delta_v );
  
  // W : 
  double w_mean, w_rms, w_min, w_max;
  printf("Reading fits file %s ...\n",fits_file_w.c_str());
  if( fits_vis_w.ReadFits( fits_file_w.c_str(), 0, 1, 1 ) ){
     printf("ERROR : could not read W FITS file %s\n",fits_file_w.c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",fits_file_w.c_str());
  }
  fits_vis_w.GetStat( w_mean, w_rms, w_min, w_max );
  printf("DEBUG : W limits %.8f - %.8f\n", w_min, w_max );

  

  // Limits of UVW :
  // double GetStat( double& mean, double& rms, double& minval, double& maxval, 


  // simple gridding :
  CBgFits uv_grid_real( n_pixels, n_pixels ) , uv_grid_imag( n_pixels, n_pixels );
  
  int added=0;
  for( int ant1 = 0 ; ant1 < fits_vis_real.GetXSize(); ant1 ++ ){
     for( int ant2 = 0 ; ant2 < fits_vis_real.GetXSize(); ant2 ++ ){
        if( ant1 > ant2 ){
           double re = fits_vis_real.getXY(ant1,ant2);
           double im = fits_vis_imag.getXY(ant1,ant2);
           
           if( !isnan(re) and !isnan(im) ){
              double u = fits_vis_u.getXY(ant1,ant2);
              double v = fits_vis_v.getXY(ant1,ant2);
              double w = fits_vis_w.getXY(ant1,ant2);
              
              int u_index = round( (u - u_min)/delta_u );
              int v_index = round( (v - v_min)/delta_v );
              
              uv_grid_real.setXY( u_index, v_index, re );
              uv_grid_imag.setXY( u_index, v_index, im );
           
              added++;
           }
        }
     }
  }  
  printf("DEBUG : added %d UV points to the grid\n",added);    
  
  if( uv_grid_real.WriteFits( "uv_grid_real.fits" ) ){
     printf("ERROR : could not write output file %s\n","uv_grid_real.fits");
  }

  if( uv_grid_imag.WriteFits( "uv_grid_imag.fits" ) ){
     printf("ERROR : could not write output file %s\n","uv_grid_imag.fits");
  }

}

