#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "libnova_interface.h"
#include "bg_globals.h"

time_t get_gmtime_from_string( const char* szGmTime );
// #include <mydate.h>

void usage()
{
   printf("sid2ux DATE LST [SITE default MRO]\n");
   printf("Sites : mwa(or mro) , ebo (EBO, ebo201309, ebo201312), wond\n");
   exit(-1);
}

int main(int argc,char* argv[])
{
	if( argc<=1 || strncmp(argv[1],"-h",2)==0 ){
	   usage();
	}
	
        int dtm=0;
	if( argc > 1 && strcmp(argv[1],"-") ){
		dtm = atol(argv[1]);		
	}
	double lst=0;
	if( argc > 2 && strcmp(argv[2],"-") ){
		lst = atof(argv[2]);		
	}
	
	if( argc > 3 && strcmp(argv[2],"-") ){
	   set_geo_location( argv[2] );
	}
	
        char szDTM[64];
        sprintf(szDTM,"%d_000000",(int)dtm);
        time_t uxtime = get_gmtime_from_string(szDTM );

	double jd;
	double sid_time_h = get_local_sidereal_time( (double)uxtime, jd ); // OK
	printf("LST(%s) = %.8f h (uxtime = %d)\n",szDTM,sid_time_h,(int)uxtime);
	

// TODO think about * or / 0.99726958 to convert from LST <-> LOCAL TIME !!!
	double uxtime_lst0 = uxtime - sid_time_h*(3600.00*0.99726958); // LST day is by ~4min shorter than SOLAR DAY 
	if( sid_time_h > lst ){
	   uxtime_lst0 = uxtime + (24.00-sid_time_h)*(3600.00*0.99726958);
	}
	printf("uxtime_lst0 = %d\n",(int)uxtime_lst0);
	double uxtime_lst = uxtime_lst0 + lst*(3600.00*0.99726958);
	
	double lst_test = get_local_sidereal_time( (double)uxtime_lst, jd );
	printf("UXTIME(LST = %.8f hours) = %d , LST_TEST = %.8f [h]\n",lst,(int)uxtime_lst,lst_test);
	
//	printf("Sidereal time at GeoLong=%.8f [deg] (%s) is  %.8f [h] = %.8f [deg]\n",geo_long,gSiteName.c_str(),sid_time_h,sid_time_h*15.00);
}
