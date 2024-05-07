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
string out_fits="out.fits";



int main(int argc,char* argv[])
{
  if( argc >= 2 ){
     in_fits = argv[1];
  }
  if( argc >= 3 ){
     out_fits = argv[2];
  }  
  
  int shift_x = 0;
  int shift_y = 1;
  
  if( argc >= 4 ){
     shift_y = atol( argv[3] );
  }
  
  
  printf("Reading fits file %s ...\n",in_fits.c_str());
  CBgFits infits;
  if( infits.ReadFits( in_fits.c_str() , 0, 1, 1 ) ){
     printf("ERROR : could not read first fits file %s on the list\n",in_fits.c_str());
     exit(-1); 
  }else{
     printf("OK : fits file %s read ok\n",in_fits.c_str());
  }

  CBgFits outfits( infits.GetXSize(), infits.GetYSize() );
  outfits.SetValue(0.00);
  if( shift_y != 0 ){
     for(int y=0;y<infits.GetYSize();y++){
        for(int x=0;x<infits.GetXSize();x++){
           float val = infits.getXY(x,y);
           outfits.setXY(x,y+shift_y,val);
        }
     }
  }  

  outfits.WriteFits( out_fits.c_str() );  
}
  
