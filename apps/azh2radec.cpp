#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "libnova_interface.h"
#include "bg_globals.h"
#include "bg_geo.h"


void usage()
{
   printf("azh2radec UXTIME [SITE default Muresk] AZIM[deg] ALT[deg]\n");
   printf("Sites : mwa(or mro) , ebo (EBO, ebo201309, ebo201312), wond\n");
   exit(-1);
}

int main(int argc,char* argv[])
{
	double uxtime = get_dttm();
	if( argc<=1 || strncmp(argv[1],"-h",2)==0 ){
	   usage();
	}
	
	if( argc > 1 && strcmp(argv[1],"-") ){
		uxtime = atof(argv[1]);		
	}
	if( argc > 2 && strcmp(argv[2],"-") ){
      set_geo_location( argv[2] );
	}

	double azim = 0.00;  
   double alt = 0.00;
   if( argc > 3 && strcmp(argv[3],"-") ){	   
      azim = atof(argv[3]);
   }
   if( argc > 4 && strcmp(argv[4],"-") ){	   
      alt = atof(argv[4]);
   }
	
   azim += 180.00;
   if (azim > 360){
      azim = azim - 360;
   }
	
	double jd;
   double ra,dec;
// void azh2radec( double az, double alt, time_t unix_time, double geo_long_deg, double geo_lat_deg, double& out_ra, double& out_dec );   
   azh2radec( azim , alt, uxtime,  geo_long, geo_lat,  ra, dec);
   printf("(RA,DEC) = ( %.8f , %.8f )\n",ra,dec);

}
