// program averages few FITS images of the same sizes 

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#include <bg_globals.h>
#include "bg_fits.h"
#include "bg_geo.h"
#include <mystring.h>

#include <vector>
using namespace std;

string list="fits_list";

double gLatDeg = ::geo_lat;  // in bg_geo.h(cpp)
double gLongDeg =  ::geo_long; // in bg_geo.h(cpp)
double gLST = 0.00;

bool gIgnoreMissingFITS = false;

void usage()
{
   printf("fixCoordHdr fits_list -l LST\n\n\n");
   printf("-l LST : provide LST as float\n");
   exit(0);
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "hl:";
   int opt;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
//      printf("opt = %c (%s)\n",opt,optarg);   
      switch (opt) {
         case 'h':
            usage();
            break;

         case 'l':
            gLST = atof( optarg );
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
    printf("LST = %.8f [deg]\n",gLST);
    printf("############################################################################################\n");
}

void fixCoordHdr( double ra_center_deg, double dec_center_deg, double lst_hours, double& xi, double& eta )
{
   const double deg2rad = (M_PI/180.00);
   
   double lat_radian = gLatDeg*deg2rad;
   double long_radian = gLongDeg*deg2rad;

   double dec_center_rad=dec_center_deg*deg2rad;
   double ra_center_rad=ra_center_deg*deg2rad;
   double ra_center_h=ra_center_deg/15.0;

   double ha_hours=lst_hours-ra_center_h;
   double ha_radians=(ha_hours*15.00)*deg2rad;

   printf("values = %.8f %.8f %.8f , %.8f\n",lat_radian,ra_center_rad,dec_center_rad,ha_radians);
   double cosZ = sin(lat_radian)*sin(dec_center_rad) + cos(lat_radian)*cos(dec_center_rad)*cos(ha_radians);
   double tanZ = sqrt(1.00-cosZ*cosZ)/cosZ;
    
   printf("tanZ = %.8f\n",tanZ);

   // Parallactic angle
   // http://www.gb.nrao.edu/~rcreager/GBTMetrology/140ft/l0058/gbtmemo52/memo52.html
   double tan_chi = sin(ha_radians)/( cos(dec_center_rad)*tan(lat_radian) - sin(dec_center_rad)*sin(ha_radians)  );

   // $lat_radian $dec_radian $ha_radian
   printf("DEBUG : values2 : %.8f %.8f %.8f\n",lat_radian,dec_center_rad,ha_radians);
   double chi_radian = atan2( sin(ha_radians) , cos(dec_center_rad)*tan(lat_radian) - sin(dec_center_rad)*cos(ha_radians) );

   printf("chi_radian = %.8f\n",chi_radian);
   
   // there is a - sign in the paper, but Randall says it's possibly wrong:
   // so I stay with NO - SIGN version
   xi=tanZ*sin(chi_radian);
   eta=tanZ*cos(chi_radian);
   
   printf("xi = %.8f\n",xi);
   printf("eta = %.8f\n",eta);
   
 
}

int main(int argc,char* argv[])
{
  string list="fits_list";
  if( argc >= 2 ){
     list = argv[1];
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
  
  for(int i=0;i<fits_list.size();i++){
     CBgFits fits;
     if( fits.ReadFits( fits_list[i].c_str() , 0, 1, 1 ) ){
        // ERROR : cannot read FITS file
        if( gIgnoreMissingFITS ){
           printf("WARNING : could not read fits file %s on the list, -i option means that it is ignored -> FITS file skipped\n",fits_list[i].c_str());
           continue;
        }else{
           printf("ERROR : could not read fits file %s on the list\n",fits_list[i].c_str());
           exit(-1); 
        }
     }else{
        printf("OK : fits file %s read ok\n",fits_list[i].c_str());
        // 
        HeaderRecord* pCRVAL1 = fits.GetKeyword("CRVAL1");
        double ra_center_deg = 0.00;       
        if( pCRVAL1 ){
           ra_center_deg = atof(pCRVAL1->Value.c_str());           
        }else{
           printf("ERROR : files %s missing keyword CRVAL1\n",fits_list[i].c_str());
        }

        HeaderRecord* pCRVAL2 = fits.GetKeyword("CRVAL2");
        double dec_center_deg = 0.00;       
        if( pCRVAL2 ){
           dec_center_deg = atof(pCRVAL2->Value.c_str());           
        }else{
           printf("ERROR : files %s missing keyword CRVAL2\n",fits_list[i].c_str());
        }
        printf("OK : centre of the image (%.8f,%.8f) [deg]\n",ra_center_deg,dec_center_deg);
        
        double xi=-1, eta=-1;
        fixCoordHdr( ra_center_deg , dec_center_deg, gLST, xi, eta );
        
        fits.SetKeywordFloat( "PV2_1", xi );
        fits.SetKeywordFloat( "PV2_2", eta );
        
        fits.WriteFits("test.fits");
     }
     
     
     // calculate keywords 
     
  }
}

