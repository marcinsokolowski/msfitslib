#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#include <bg_globals.h>
#include <bg_fits.h>
#include <bg_array.h>

#include <mystring.h>
#include <myfile.h>

#include <vector>
using namespace std;

enum eImageCutType_T  { eCutRows=0, eCutColumns=1, eCutBoth=2 };

string infits="test.fits";
string out_file="merged.fits";
string outdir;
int gVerb=0;
int gStart=0;
int gEnd=-1;
int gFreqMHz=0;
double gStartMHz=0.00;
double gEndMHz=0.00;

int gCutBadValues=0;
double gMinValueOK=-20;
double gMaxValueOK=+20;

int gStartY = -1;
int gEndY   = -1;

eImageCutType_T gCutType = eCutRows;

void usage()
{
   printf("cutimage FITS_FILE -s %d -e %d -f %s -c -b -x MIN_VAL_OK -y MAX_VAL_OK -B\n",gStart,gEnd,out_file.c_str());
   printf("\n");
   printf("Options :\n");   
   printf("\t-f OUT_FILENAME [default %s]\n",out_file.c_str());
   printf("\t-s START_ROW_or_COL [default 0]\n");
   printf("\t-e END_ROW_or_COL [default 2000], this row/col is not included (<END_ROW are included) -> NEW_SIZE=(END_ROW - START_ROW)\n");
   printf("\t-c : cut columns instead of rows\n");
   printf("\t-m : frequency in MHz\n");
   printf("\t-b : fix bad values : use options -x and -y to specify min and max acceptable values [defaults %.2f and %.2f]\n",gMinValueOK,gMaxValueOK);
   printf("\t-B : cut both in columns and rows , use -s and -e for Y axis (-S and -E for X axis)\n");
   exit(-1);
}

void print_parameters()
{
   printf("#####################################\n");
   printf("PARAMETERS :\n");
   printf("#####################################\n");
   printf("Input file        = %s\n",infits.c_str());
   printf("Outdir            = %s\n",outdir.c_str());
   printf("out_file          = %s\n",out_file.c_str());
   printf("verb level        = %d\n",gVerb);
   printf("Start/end in MHz  = %d\n",gFreqMHz);
   printf("Action            = %d\n",gCutType);   
   if( gCutBadValues > 0 ){
      printf("Good values range = %.2f - %.2f\n",gMinValueOK,gMaxValueOK);
   }
   printf("#####################################\n");   
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "bmcvo:f:s:e:x:y:BS:E:";
   int opt,opt_param,i;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
      switch (opt) {
         case 'b' :
            gCutBadValues=1;
            break;
         case 'c':
            gCutType = eCutColumns;
            break;
         case 'B':
            gCutType = eCutBoth;
            break;
         case 'o':
            if( optarg ){
               outdir = optarg;
            }
            break;
         case 'f':
            if( optarg ){
               out_file = optarg;
            }
            break;
         case 's':
            if( optarg ){
               gStart = atol(optarg);
               gStartMHz = atof(optarg);
            }
            break;
         case 'e':
            if( optarg ){
               gEnd = atol(optarg);
               gEndMHz = atof(optarg);
            }
            break;

         case 'S':
            if( optarg ){
               gStartY = atol(optarg);
            }
            break;
         case 'E':
            if( optarg ){
               gEndY = atol(optarg);
            }
            break;

         case 'x':
            if( optarg ){
               gMinValueOK = atof(optarg);
            }
            break;

         case 'y':
            if( optarg ){
               gMaxValueOK = atof(optarg);
            }
            break;

         case 'm':
            gFreqMHz=1;
            break;

         case 'v':
            gVerb++;
            break;
         case 'h':
            usage();
            break;
         default:  
            fprintf(stderr,"Unknown option %c\n",opt);
            usage();
      }
   }
}
                                                        
                                                                 
int main(int argc,char* argv[])
{
  if( argc >= 2 ){
     infits = argv[1];
  }

  // parse command line :
  parse_cmdline(argc,argv);
  print_parameters();
  
  CBgFits fits( infits.c_str() );
  if( fits.ReadFits( infits.c_str() ) ){
    printf("ERROR : could not read first fits file %s\n",infits.c_str());
    exit(-1);
  }
  
  if( gFreqMHz > 0 ){
     gStart = fits.freq2ch( gStartMHz );
     gEnd = fits.freq2ch( gEndMHz );
     printf("Frequency range (%.2f - %.2f) [MHz] -> (%d - %d)\n",gStartMHz,gEndMHz,gStart,gEnd);
  }

  if( gCutBadValues > 0 ){
    fits.FixBadValues( gMinValueOK, gMaxValueOK );
  }
  
  if( gCutType == eCutRows ){
     printf("INFO : cutting image rows\n");
     if( gEnd >= fits.GetYSize() || gEnd<=0 ){
        gEnd = fits.GetYSize()-1;
     }
     int new_ySize=(gEnd-gStart);  // was +1
     printf("Saving truncated fits file (%ld x %d) to file %s\n",fits.GetXSize(),new_ySize,out_file.c_str());  
     
     CBgFits out_image(out_file.c_str(),fits.GetXSize(),new_ySize);
     for(int y=gStart;y<gEnd;y++){ // was <=gEnd
        int new_y = y-gStart;
     
        out_image.set_line(new_y,fits.get_line(y));     
     } 
     out_image.SetKeys( fits.GetKeys() );
     out_image.WriteFits(out_file.c_str());             
  }else{
     if( gCutType == eCutBoth ){
         printf("INFO : getting small part of the image (%d,%d) - (%d,%d)\n",gStart,gStartY,gEnd,gEndY);
         int new_xSize=(gEnd-gStart);
         int new_ySize=(gEndY-gStartY);
         
         printf("Saving truncated fits file (%d x %d) to file %s\n",new_xSize,new_ySize,out_file.c_str());
         
         CBgFits out_image(out_file.c_str(),new_xSize,new_ySize);
         for(int y=gStartY;y<gEndY;y++){
            for(int x=gStart;x<gEnd;x++){
               double val = fits.valXY(x,y);
               int new_x = x-gStart;
               int new_y = y-gStartY;
           
               out_image.setXY(new_x,new_y,val); 
            }
         }
         out_image.SetKeys( fits.GetKeys() );

         // set start end freq :
         out_image.SetKeywordFloat("CRVAL1",gStart);       
         out_image.SetKeywordFloat("CRVAL2",gStartY);         

         out_image.WriteFits(out_file.c_str());

     }else{
         if( gEnd >= fits.GetXSize() || gEnd<=0 ){
            gEnd = fits.GetXSize()-1;
         }
         printf("INFO : cutting image columns\n");
         int new_xSize=(gEnd-gStart);
         printf("Saving truncated fits file (%d x %ld) to file %s\n",new_xSize,fits.GetYSize(),out_file.c_str());
 
         double freq_start = ch2freq(gStart);
          
         CBgFits out_image(out_file.c_str(),new_xSize,fits.GetYSize());
         for(int y=0;y<fits.GetYSize();y++){
            for(int x=gStart;x<gEnd;x++){
               double val = fits.valXY(x,y);
               int new_x = x-gStart;
           
               out_image.setXY(new_x,y,val); 
            }
         }
         out_image.SetKeys( fits.GetKeys() );

         // set start end freq :
         out_image.SetKeywordFloat("CRVAL1",freq_start);       

         out_image.WriteFits(out_file.c_str());
     }
  }
}

