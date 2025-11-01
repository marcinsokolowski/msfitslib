#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <bg_globals.h>
#include "bg_fits.h"

#include <vector>
using namespace std;

int main(int argc,char* argv[])
{
  string list="fits_list";
  if( argc >= 2 ){
     list = argv[1];
  }
  string out_file="median.fits";
  if( argc >= 3 ){
     out_file = argv[2];
  }
  
  vector<string> fits_list;
  if( bg_read_list(list.c_str(),fits_list) <= 0 ){
     printf("ERROR : could not read list file %s\n",list.c_str());
     exit(-1);
  }else{
     for(int i=0;i<fits_list.size();i++){
        printf("%i %s\n",i,fits_list[i].c_str());
     }
  }

  if( fits_list.size() > 0 ){
     vector<CBgFits*> fits_tab;
     fits_tab.assign( fits_list.size(), NULL );

     for(int i=0;i<fits_list.size();i++){
        printf("Reading image %s ...\n",fits_list[i].c_str());
        fits_tab[i] = new CBgFits();     
  
        if( (fits_tab[i])->ReadFits( fits_list[i].c_str() ) ){
           printf("ERROR : could not read fits file %s on the list\n",fits_list[i].c_str());
           exit(-1); 
        }else{
           printf("OK : fits file %s read ok\n",fits_list[i].c_str());
        }     
     }
     
     CBgFits& first_fits = *(fits_tab[0]);
     int xSize = first_fits.GetXSize();
     int ySize = first_fits.GetYSize();
     
     CBgFits out_median( xSize, ySize );

     int file_count = fits_list.size();
     double* values = new double[file_count];
     for( int x=0;x<xSize;x++){
        for( int y=0;y<ySize;y++){
           int values_count = 0;
           for(int f=0;f<file_count;f++){
              values[values_count] = (fits_tab[f])->getXY(x,y);
              values_count++;
           }              
           my_sort_float( values, values_count );
           double median =  values[values_count/2];
           out_median.setXY(x,y,median);
        }
     }
     delete [] values;
     
     out_median.WriteFits( out_file.c_str() );
  }
}

