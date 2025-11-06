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
string out_fits="dynaspec.fits";

int gX = 135;
int gY = 140;

void usage()
{
   printf("build_dynaspec_simple fits_list dynaspec.fits\n\n\n");
   exit(0);
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "h";
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
            break;*/

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

  CBgFits dynaspec( 10*50 , 24*32 );
  
  vector<CBgFits*> image_list;
  
  int start_ux,start_finech,start_timestep,start_cc;
  if( sscanf( fits_list[0].c_str(), "start_time_%d_coarse_%d_real/timestep%05d_channel%05d.fits",&start_ux,&start_cc,&start_timestep,&start_finech) != 4 ){
     printf("ERROR : could not parse file name %s\n",fits_list[0].c_str());
     exit(-1);
  }
  printf("%s : start_ux = %d, start_finech = %d, start_timestep = %d , start_cc = %d\n",fits_list[0].c_str(),start_ux,start_cc,start_timestep,start_finech);

  CBgFits fits;  
  for(int i=0;i<fits_list.size();i++){
      if( fits.ReadFits( fits_list[i].c_str() , 0, 1, 1 ) ){
         printf("ERROR : could not read FITS file %s\n",fits_list[i].c_str());
         exit(-1);
      }
      
      int ux,finech,timestep,cc;
      
      // start_time_1508442488_coarse_132_real/timestep00023_channel00001.fits
      if( sscanf( fits_list[i].c_str(), "start_time_%d_coarse_%d_real/timestep%05d_channel%05d.fits",&ux,&cc,&timestep,&finech) == 4 ){
         // calculate which pixel to fill in in the dynamic spectrum :
         int x = (ux-start_ux)*50 + timestep;
         int y = (cc - 109)*32 + finech;
         double value = fits.getXY(gX,gY);
         
         printf("%s : filling timestep = %d, channel = %d with value = %.6f\n",fits_list[i].c_str(), x, y, value );
         if( x>=0 && x<dynaspec.GetXSize() && y>=0 && y<dynaspec.GetYSize() ){
            dynaspec.setXY(x,y,value);
         }
         
      }else{
         printf("ERROR : could not parse parameters from file name %s\n",fits_list[i].c_str());
      }
  }
  
  dynaspec.WriteFits("dynaspec.fits");
  
}
 

