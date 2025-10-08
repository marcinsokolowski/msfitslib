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

string in_fits="in.fits";
string out_file="total_power.txt";

bool gRMS=false;
int gDebug=0;
bool gVertical=false;

void usage()
{
   printf("Usage:\n");
   printf("total_power_simple FITS\n");
   printf("-r : RMS instead of total power\n");  
   printf("-o OUTPUT_FILE [default %s]\n",out_file.c_str());
   printf("-v : vertical FITS file [default %d]\n",gVertical);
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "ho:rvd";
   int opt,opt_param,i;

   while ((opt = getopt(argc, argv, optstring)) != -1) {
      switch (opt) {
         case 'o':
            if( optarg ){
               out_file = optarg;
            }
            break;

         case 'r':
            gRMS=true;
            break;

         case 'v':
            gVertical=true;
            break;

         case 'd':
            gDebug++;
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

void print_parameters()
{
   printf("#####################################################\n");
   printf("PARAMETERS\n");
   printf("#####################################################\n");
   printf("RMS = %d\n",gRMS);
   printf("#####################################################\n");
}

void TotalPowerHorizontal( CBgFits& fits )
{
   vector<double> total_power_vec;
   int size = fits.GetXSize();
   
   for(int x=0;x<size;x++){
       double sum=0.0;
       double sum2=0.0;
       int count=0;
       
       for(int y=0;y<fits.GetYSize();y++){
          double val = fits.getXY(x,y);
          sum += val;
          sum2 += (val*val);
          count++;          
       }
       
       double total_power = 0.00;
       if( gRMS ){
          double mean = sum/count;
          double rms = sqrt( sum2/count - mean*mean);
          total_power = rms; 
          
       }else{
          total_power = sum;
       }
       
       total_power_vec.push_back(total_power);              
   }
   
   FILE* outf = fopen(out_file.c_str(),"w");
   for( int i=0;i<total_power_vec.size();i++){
      fprintf(outf,"%d %.8f\n",i,total_power_vec[i]);
   }   
   fclose(outf);
}


int main(int argc,char* argv[])
{
  if( argc >= 2 ){
     in_fits = argv[1];
  }
  
  // parse command line :
  parse_cmdline(argc,argv);
  print_parameters();
  
  printf("Reading fits file %s ...\n",in_fits.c_str());
  CBgFits infits;
  if( infits.ReadFits( in_fits.c_str() , 0, 1, 1 ) ){
     printf("ERROR : could not read first fits file %s on the list\n",in_fits.c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",in_fits.c_str());
  }

  TotalPowerHorizontal( infits );
}
  
