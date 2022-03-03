#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "libnova_interface.h"
#include "bg_globals.h"
#include "bg_geo.h"
#include "cvalue_vector.h"

void usage()
{
   printf("ux2sid_file IN_FILE OUT_FILE [SITE default Muresk]\n");
   printf("Sites : mwa(or mro) , ebo (EBO, ebo201309, ebo201312), wond\n");
   exit(-1);
}

int main(int argc,char* argv[])
{
   double uxtime = get_dttm();
   if( argc<=1 || strncmp(argv[1],"-h",2)==0 ){
      usage();
   }
   
        string in_file="value_vs_uxtime.txt";
        string out_file="value_vs_lst.txt";
        
   if( argc > 1 && strcmp(argv[1],"-") ){
      in_file = argv[1];      
   }
   if( argc > 2 && strcmp(argv[2],"-") ){
      out_file = argv[2];      
   }
   if( argc > 3 && strcmp(argv[3],"-") ){
      if( is_number(argv[3]) ){
         geo_long = atof(argv[3]);
      }else{
         if( strcmp(argv[3],"mwa") == 0 || strcmp(argv[3],"mro") == 0 ){
            geo_long = 116.670815;
            gSiteName = "MWA";
         }
         if( strcmp(argv[3],"ebo") == 0 || strcmp(argv[3],"EBO") == 0 || strcasecmp(argv[3],"ebo201309") == 0 ){
            // 2013-09
            geo_long = 126.30268889; // was 126.3013;
            gSiteName = "EBO";
         }
         if( strcasecmp(argv[3],"ebo201312") == 0  ){
            // 2013-12 :
            geo_long = 126.29777778; // was 126.3013;
            gSiteName = "EBO201312";
         }      
         if( strcasecmp(argv[3],"wond") == 0  ){
            geo_long = 118.43999167;
            gSiteName = "WONDINONG";
         }      
      }
      
   }

   CValueVector in_file_values;
   FILE* out_f = fopen(out_file.c_str(),"w");
   if( in_file_values.read_file( in_file.c_str() ) > 0 ){
      for(int i=0;i<in_file_values.size();i++){
         cValue& val = in_file_values[i];
         double uxtime = val.x;
         
         double jd;
         double sid_time_h = get_local_sidereal_time( (double)uxtime, jd );
         
         fprintf(out_f,"%.20f %.20f %.20f\n",sid_time_h,val.y,uxtime);
      }
   }
   fclose(out_f);
   printf("Value vs. LST written to output file = %s\n",out_file.c_str());   
}
