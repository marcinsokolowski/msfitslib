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

void usage()
{
   printf("avg_images fits_list out.fits out_rms.fits CALCULATE_RMS -x\n\n\n");
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
    printf("Fits files range : %d - %d\n",gStartFitsIndex,gEndFitsIndex);
    printf("out_fits     = %s\n",out_fits.c_str());
    printf("out_rms_fits = |%s| (empty means no RMS fits file will be written)\n",out_rms_fits.c_str());
    printf("Calc max     = %d\n",gCalcMax);
    if( gMaxRMSOnSingle <= 0 ){
        printf("No RMS cut imposed on individual images\n");
    }else{
        printf("Allowed RMS range : %.8f - %.8f\n",gMinRMSOnSingle,gMaxRMSOnSingle);
    }
    printf("Dump window = (%d,%d) - (%d,%d)\n",gBorderStartX,gBorderStartY,gBorderEndX,gBorderEndY);
    printf("RMS calculated in radius of %d pixels around center\n",gCenterRadius);
    if( gCalcRMSAroundPixel ){
       printf("\tRMS calculated around pixel (%d,%d)\n",gBorderStartX,gBorderStartY);
    }
    printf("Ignore missing FITS = %d\n",int(gIgnoreMissingFITS));
    printf("Beam image for weighting = %s\n",beam_fits_file.c_str());
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
  out_rms_fits="out_rms.fits";
  if( argc >= 4 ){
     out_rms_fits = argv[3];
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
  
  printf("Reading fits file %s ...\n",fits_list[gStartFitsIndex].c_str());
  CBgFits first_fits, first_fits2;
  if( first_fits.ReadFits( fits_list[gStartFitsIndex].c_str() ) ){
     printf("ERROR : could not read first fits file %s on the list\n",fits_list[gStartFitsIndex].c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",fits_list[gStartFitsIndex].c_str());
  }
  
  // read the same second time to save RMS 
  if( strlen(out_rms_fits.c_str()) > 0 ){
      if( first_fits2.ReadFits( fits_list[gStartFitsIndex].c_str() ) ){
         printf("ERROR : could not read first fits file %s on the list\n",fits_list[gStartFitsIndex].c_str());
         exit(-1); 
      }else{
         printf("OK : fits file %s read ok\n",fits_list[gStartFitsIndex].c_str());
      }
  }
  
  int size = first_fits.GetXSize()*first_fits.GetYSize();
  
  CBgFits* pMax = NULL;
  if( gCalcMax > 0 ){
     pMax = new CBgFits( first_fits.GetXSize(), first_fits.GetYSize() );
     pMax->SetKeysWithoutStates( first_fits.GetKeys() );
  }

  double* sum_tab = new double[size];
  double* sum2_tab = NULL;
  if( strlen(out_rms_fits.c_str()) > 0 ){
      sum2_tab = new double[size];
      printf("DEBUG : sum2_tab initialised\n");      
  }

  CBgFits* pBeamImage = NULL;
  double* sum_beam = NULL;

// 2021-11-06 : start with zeros to do everything in the loop (required after adding beam weighting) 
  // initialise sum_tab and sum2_tab with values from the 1st image :
//  float* first_fits_data_ptr = first_fits.get_data();
  for (int pos=0;pos<size;pos++){
//     double val = first_fits_data_ptr[pos];
  
     sum_tab[pos] = 0; // val;
     if( sum2_tab ){
         sum2_tab[pos] = 0; // val*val;
     }
     if( pMax ){
        pMax->get_data()[pos] = -1e20; // val;
     }
  }
  int good_image_count = 1; // first image already included 
  
  CBgFits fits;
  // 2014-08-20 was from 1 but I've changed to 0 to average all the files 
  int last_fits = fits_list.size();
  if( gEndFitsIndex < last_fits ){
     last_fits = gEndFitsIndex;
  }
  for(int i=(gStartFitsIndex);i<last_fits;i++){ // 2019-07-11 - start from the 2nd (1st or 0-based) image 
     if( fits.ReadFits( fits_list[i].c_str() ) ){
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
     
     //  if weigthing by the beam is required :
     if( strlen( beam_fits_file.c_str() ) > 0 ){
        mystring szFullBeamFitsFile = beam_fits_file.c_str();
        if( strstr( beam_fits_file.c_str() , "/" ) ){
           printf("INFO : directory already included in beam file name %s\n",beam_fits_file.c_str());
        }else{
           mystring szDrv,szDir,szFName, szExt;
           mystring szTmp = fits_list[i].c_str();
           szTmp.splitpath( szDrv,szDir,szFName,szExt );
                
           szFullBeamFitsFile = szDir.c_str();
           szFullBeamFitsFile += "/";
           szFullBeamFitsFile += beam_fits_file.c_str();
           printf("INFO : full beam FITS file path = %s\n",szFullBeamFitsFile.c_str());
        }
        if( !pBeamImage ){
           pBeamImage = new CBgFits( szFullBeamFitsFile.c_str() );
        }
        if( pBeamImage->ReadFits( szFullBeamFitsFile.c_str() ) ){
           printf("ERROR : could not read beam file %s to be used for weighthing images\n",szFullBeamFitsFile.c_str());
           exit(-1);
        }else{
           printf("OK : beam fits file %s read OK\n",szFullBeamFitsFile.c_str());
        }
        if( pBeamImage->GetXSize() != first_fits.GetXSize() || pBeamImage->GetYSize() != first_fits.GetYSize() ){
           printf("ERROR : size of the beam FITS file is %d x %d != size of image FITS %d x %d -> cannot continue\n",pBeamImage->GetXSize(),first_fits.GetXSize(),pBeamImage->GetYSize(),first_fits.GetYSize());
           exit(-1);
        }

        if( !sum_beam ){     
           sum_beam = new double[size];
        }
     }


     // check image :
     double mean, rms, minval, maxval;
     double mean_center, rms_center, minval_center, maxval_center, median_center, iqr_center, rms_iqr_center;
     
     if( gCenterRadius > 0 ){
//        printf("DEBUG : checking stat in radius %d pixels around center\n",gCenterRadius);
//        fits.GetStatRadius( mean, rms, minval, maxval, gCenterRadius );
        fits.GetStatRadiusAll( mean_center, rms_center, minval_center, maxval_center, median_center, iqr_center, rms_iqr_center, gCenterRadius, true, gBorderStartX, gBorderStartY );
        printf("%s : at CENTER mean stat = %.8f, rms = %.8f, min_val = %.8f, max_val = %.8f , median = %.8f, rms_iqr = %.8f\n",fits_list[i].c_str(),mean_center, rms_center, minval_center, maxval_center, median_center, rms_iqr_center );
     }else{
        if( gBorderStartX>0 && gBorderStartY>0 && gBorderEndX>0 && gBorderEndY>0 ){
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

     printf("STAT %s : mean = %.4f rms = %.4f minval = %.4f maxval = %.4f , median_center = %.4f, rms_iqr_center = %.4f\n",fits_list[i].c_str(), mean, rms, minval, maxval, median_center, rms_iqr_center );     
     if ( gMaxRMSOnSingle <= 0 || ( gMinRMSOnSingle <= rms_iqr_center && rms_iqr_center <= gMaxRMSOnSingle ) ){     
         for (int pos=0;pos<size;pos++){
            double val = fits.get_data()[pos];
            double beam = 1.00;
            if( pBeamImage ){
               beam = (pBeamImage->get_data())[pos];
               sum_tab[pos] += val*(beam*beam);               
               sum_beam[pos] += (beam*beam);
            }else{                   
               sum_tab[pos] += val;
            }
            if( sum2_tab ){
                sum2_tab[pos] += (val*val);
            }
         }

         printf("DEBUG : included image %s\n",fits_list[i].c_str());         
         good_image_count++;
     }else{
        printf("WARNING : %s skipped due to rms = %.8f outside the allowed range %.8f - %.8f\n",fits_list[i].c_str(),rms_iqr_center,gMinRMSOnSingle,gMaxRMSOnSingle);
     }
     
     if( pMax ){
        for (int pos=0;pos<size;pos++){
           double val = fits.get_data()[pos];
           
           if( val > pMax->get_data()[pos] ){
              pMax->get_data()[pos] = val;
           }
        }
     }
//     first_fits += fits;     
  }

  printf("STAT_INFO : averaged %d good images of %d all\n",good_image_count,fits_list.size());    
  for (int pos=0;pos<size;pos++){       
     double mean = ( sum_tab[pos] / good_image_count );
     
     if( pBeamImage && sum_beam ){
        mean = sum_tab[pos] / sum_beam[pos];
     }
     
     first_fits.get_data()[pos] = mean;
     if( sum2_tab ){
         first_fits2.get_data()[pos] = sqrt( (sum2_tab[pos] / good_image_count ) - mean*mean );
     }
     
//     printf("%.8f %.20f\n",first_fits.ch2freq(pos),( sum_tab[pos] / fits_list.size() ));
  }           
//  first_fits.Normalize(fits_list.size());

  double mean,rms,minval,maxval;
  first_fits.GetStat( mean, rms, minval, maxval );
  printf("FINAL : mean stat = %.8f, rms = %.8f, min_val = %.8f, max_val = %.8f\n",mean, rms, minval, maxval );

  int count_center;
  double mean_center, rms_center, minval_center, maxval_center, median_center, iqr_center, rms_iqr_center;  
  first_fits.GetStatRadiusAll( mean_center, rms_center, minval_center, maxval_center, median_center, iqr_center, rms_iqr_center, count_center, 100, true );
  printf("FINAL CENTER : mean stat = %.8f, rms = %.8f, min_val = %.8f, max_val = %.8f , median = %.8f, rms_iqr = %.8f\n",mean_center, rms_center, minval_center, maxval_center, median_center, rms_iqr_center );

  
  if( first_fits.WriteFits( out_fits.c_str() ) ){
     printf("ERROR : could not write average fits file %s\n",out_fits.c_str());
     exit(-1);
  }  
  printf("OK : output fits file %s written ok\n",out_fits.c_str());
  printf("INFO : mean/rms calculated based on %d good images\n",good_image_count);

  if( sum2_tab ){
      if( first_fits2.WriteFits( out_rms_fits.c_str() ) ){
         printf("ERROR : could not write average fits file %s\n",out_rms_fits.c_str());
         exit(-1);
      }  
      printf("OK : output fits file %s written ok\n",out_rms_fits.c_str());
  }
  
  if( pMax ){
     pMax->WriteFits( "max.fits" );
     delete pMax;
  }

  delete [] sum_tab;
  if( sum2_tab ){
      delete [] sum2_tab;
  }
  
  if( pBeamImage ){
     delete pBeamImage;
  }
  
  if( sum_beam ){
     delete [] sum_beam;
  }
}

