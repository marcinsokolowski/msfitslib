#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "libnova_interface.h"
#include "bg_globals.h"

void usage()
{
   printf("ux2sid UXTIME [SITE default Muresk]\n");
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
		if( is_number(argv[2]) ){
   		geo_long = atof(argv[2]);
		}else{
			if( strcmp(argv[2],"mwa") == 0 || strcmp(argv[2],"mro") == 0 ){
            geo_long = 116.670815;
            gSiteName = "MWA";
         }
         if( strcmp(argv[2],"ebo") == 0 || strcmp(argv[2],"EBO") == 0 || strcasecmp(argv[2],"ebo201309") == 0 ){
            // 2013-09
            geo_long = 126.30268889; // was 126.3013;
            gSiteName = "EBO";
         }
         if( strcasecmp(argv[2],"ebo201312") == 0  ){
            // 2013-12 :
            geo_long = 126.29777778; // was 126.3013;
            gSiteName = "EBO201312";
         }		
         if( strcasecmp(argv[2],"wond") == 0  ){
            geo_long = 118.43999167;
            gSiteName = "WONDINONG";
         }		
		}
		
	}
	
	double jd;
	double sid_time_h = get_local_sidereal_time( (double)uxtime, jd );

	printf("Sidereal time at GeoLong=%.8f [deg] (%s) is  %.8f [h] = %.8f [deg]\n",geo_long,gSiteName.c_str(),sid_time_h,sid_time_h*15.00);
}
