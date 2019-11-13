#include "bg_geo.h"
#include <string.h>

//double geo_long=127.885437;
//double geo_lat=-31.826232; 
double geo_long=116.670815;
double geo_lat=-26.703319;

string gSiteName="MWA";

/*
   Cone position measurements in 2014-10 using Nokia E52 :
   
   Cone at MRO1 :
     116 40 16.31 East , -26 42 10.38 , error = 7.92 m, h = 357.5 m
     116.67119722 , -26.70288333

   Cone at MRO2 : 
     116 40 16.26 East , -26 42 10.35 , error = 9.83 m, h = 357 m
     116.67118333 , -26.70287500

   Cone at MRO3 :
     116 40 16.34 East , -26 42 10.32 , error 8.8m m, h = 358 m 
     116.67120556 , -26.70286667
     
     
   Mean of the 3 measurements is at : 
      ( 116.67119537 , -26.702875 )

Randall 20180209 :
Hey… I’ve done this before. Who’d have thought! I had a slightly different location for BIGHORNS. Do you think yours is more correct? I can update if so.
LonLat for ant BIGHORNS: lat: -26.702868, lon: 116.671254, alt: 377.00
Local_XYZ: 21.73 43.66 45.05
Local_ENU: BIGHORNS 043.658 050.009 -00.827


LonLat for ant AAVS1.2: lat: -26.704796, lon: 116.670769, alt: 376.33
Local_XYZ: -74.86 -4.60 -145.49
Local_ENU: AAVS1.2 -04.599 -163.615 -01.496
LonLat for ant AAVS1.1: lat: -26.704080, lon: 116.670231, alt: 376.11
Local_XYZ: -39.41 -58.11 -74.52
Local_ENU: AAVS1.1 -58.108 -84.284 -01.714
LonLat for ant AAVS1.3: lat: -26.704757, lon: 116.672438, alt: 376.08
Local_XYZ: -73.12 161.47 -141.47
Local_ENU: AAVS1.3 161.475 -159.243 -01.749
LonLat for ant AAVS1.4: lat: -26.705962, lon: 116.670754, alt: 375.04
Local_XYZ: -134.07 -6.11 -260.32
Local_ENU: AAVS1.4 -06.106 -292.797 -02.791
LonLat for ant EDA: lat: -26.703069, lon: 116.672257, alt: 374.78
Local_XYZ: 9.76 143.44 26.18
Local_ENU: EDA 143.441 027.773 -03.045

So these give the local corrds in “east north up” units and XYZ units. XYZ is where the X axis goes through the equator. I can explain if you like, but if you’re writing your own code, then I suggest using ENU.
The zero point of the ccord system is the rock next to the donga/Telstra hut.      
*/

void set_geo_location( const char* szSiteName )
{
    if( strcasecmp(szSiteName,"mwa") == 0 || strcasecmp(szSiteName,"mro") == 0 ){
       geo_long = 116.670815;
       geo_lat  = -26.703319;
       gSiteName = "MWA";
    }
    if( strcmp(szSiteName,"muresk") == 0 || strcmp(szSiteName,"MURESK") == 0 ){
       geo_long=116.68609722;
       geo_lat=-31.74625000;
       gSiteName = "MURESK";
    }
    if( strcmp(szSiteName,"ebo") == 0 || strcmp(szSiteName,"EBO") == 0 || strcasecmp(szSiteName,"ebo201309") == 0 ){
       // 2013-09
       geo_long = 126.30268889; // was 126.3013;  
       geo_lat  = -32.25256389; // was -32.246391;
       gSiteName = "EBO201309";
    }
    if( strcasecmp(szSiteName,"ebo201312") == 0  ){
       // 2013-12 :
       geo_long = 126.29777778; // was 126.3013;  
       geo_lat  = -32.25250000; // was -32.246391;
       gSiteName = "EBO201312";
    }
    if( strcasecmp(szSiteName,"wond20140406") == 0  || strcasecmp(szSiteName,"wond")==0 ){
       // 2014-04-06 :
       geo_long = 118.43999167; // - 27deg 51'10.31''
       geo_lat  = -27.85286389; // 118deg 26' 23.97''
       gSiteName = "WONDINONG_20140406";
    }
    if( strcasecmp(szSiteName,"wond20140405") == 0  ){
       // 2014-04-05 :
       // TODO: change according to Randall's info ! 
       geo_long = 118.43999167; // - 27deg 51'10.31''
       geo_lat  = -27.85286389; // 118deg 26' 23.97''
       gSiteName = "WONDINONG_20140406";
    }    
    if( strcasecmp(szSiteName,"carmel") == 0  ){
       // 2014-04-05 :
       // TODO: change according to Randall's info ! 
       geo_long = 116.096;
       geo_lat  = -32.021;
       gSiteName = "CARMEL";
    }    
}
    

