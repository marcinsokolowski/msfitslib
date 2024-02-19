#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "../src/bg_globals.h"
#include "../src/bg_fits.h"
#include "../src/bg_geo.h"
// #include <bg_array.h>
#include "../src/libnova_interface.h"

#include <vector>
using namespace std;

string in_file="test.fits";
int gStartChannel=0;
int gEndChannel=4095;
double gUnixTime=get_dttm();
int gVerb=1;
int gPrintAZH=0;
int gPrintALL=1;
int gTest=0;

bool gAstroAzim = false;

void usage()
{
   printf("print_sun UNIX_TIME[default NOW] -s -c -l GEO_LONGITUDE -a GEO_LATITUDE -o OBS_SITE[default Muresk]\n");
   printf("-s : silent mode, just required output\n");
   printf("-c : calculate horizontal coordinates for given time\n");
   printf("-l GEO_LONGITUDE [deg], default %.2f [deg], <0 = WEST, >0 = EAST\n",geo_long);
   printf("-a GEO_LATITUDE  [deg], default %.2f [deg]\n",geo_lat);   
   printf("-o OBS_SITE : default Muresk, other recognised values are : ebo, ebo201312, wond, mro(or mwa)\n");
   printf("-A : astro azim (from North = 0 deg -> East = 90 deg -> South = 180 deg -> West = 270 deg\n");
   exit(-1);
}

void print_parameters()
{
   if( gVerb >= 1 ){  
      printf("#####################################\n");
      printf("PARAMETERS :\n");
      printf("#####################################\n");
      printf("Unix time = %.2f\n",gUnixTime);
      printf("Geo location = (%.8f,%.8f) [deg] [%s]\n",geo_long,geo_lat,gSiteName.c_str());
      printf("Astro azim   = %d\n",gAstroAzim);
      printf("#####################################\n");   
   }
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "hsca:l:o:A";
   int opt,opt_param,i;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
      switch (opt) {
         case 'h':
            usage();
            break;

         case 'A':
            gAstroAzim = true;
            break;

         case 'c':
            gPrintAZH=1;
            gPrintALL=0;
            break;
         case 's':
            gVerb=0;
            break;
         case 'l':
            geo_long = atof(optarg);            
            break;
         case 'a':
            geo_lat = atof(optarg);
            break;                    
         case 'o':
            set_geo_location(optarg);

/*            if( strstr(optarg,"ebo201312") || strstr(optarg,"EBO201312") ){
               geo_long = 126.29777778; // was 126.3013;
               geo_lat  = -32.25250000; // was -32.246391;
               gSiteName = "EBO201312";                            
            }else{
               if( strstr(optarg,"ebo") || strstr(optarg,"EBO") ){
                  geo_long = 126.30268889; // was 126.3013;
                  geo_lat  = -32.25256389; // was -32.246391;                                              
                  gSiteName = "EBO201309";
               }
            }
            if( strcasecmp(optarg,"wond20140406") == 0  || strcasecmp(optarg,"wond")==0 ){
               // 2014-04-06 :
               geo_long = 118.43999167; // - 27deg 51'10.31''
               geo_lat  = -27.85286389; // 118deg 26' 23.97''
               gSiteName = "WONDINONG_20140406";
            }
            if( strcasecmp(optarg,"wond20140405") == 0  ){
               // 2014-04-05 :
               // TODO: change according to Randall's info !
               geo_long = 118.43999167; // - 27deg 51'10.31''
               geo_lat  = -27.85286389; // 118deg 26' 23.97''
               gSiteName = "WONDINONG_20140406";
            }*/
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
     if( strncmp(argv[1],"-h",2)==0 ){
        usage();
     }else{
        if( atof(argv[1]) > 0 ){
           gUnixTime = atof(argv[1]);
        }
     }
  }

  // parse command line :
  parse_cmdline(argc-1,argv+1);
  print_parameters();

  if( gPrintALL ){
     time_t out_rise_ux,out_set_ux,out_transit_ux;
     get_sun_info( gUnixTime , geo_long, geo_lat, out_rise_ux, out_set_ux, out_transit_ux );
     
     // TEST :
     if( gTest > 0 ){
        time_t out_sun_rise_ux, out_sun_set_ux;
        get_sun_rise_set( gUnixTime , geo_long, geo_lat, out_sun_rise_ux, out_sun_set_ux, 10 );
        printf("TEST : sun_rise = %d (%.2f minutes), sun set = %d (%.2f minutes)\n",int(out_sun_rise_ux),((out_sun_rise_ux-gUnixTime)/60.00),int(out_sun_set_ux),((out_sun_set_ux-gUnixTime)/60.00));
     }
  }
  if( gPrintAZH ){    
    double out_ra, out_dec, out_az, out_alt;
    get_sun_all( (time_t)gUnixTime, geo_long, geo_lat, out_ra, out_dec, out_az, out_alt );    
    
    if( gAstroAzim ){
       out_az = out_az + 180.00;
       if ( out_az > 360 ){
          out_az = out_az - 360.00;
       }
    }
    
    printf("(RA,DEC)   = ( %.4f , %.4f ) [deg]\n",out_ra,out_dec);
    printf("(AZIM,ALT) = ( %.4f , %.4f ) [deg]\n",out_az, out_alt);
  }
}

