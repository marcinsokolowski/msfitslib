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
  string reffile="median.fits";
  if( argc >= 3 ){
     reffile = argv[2];
  }
  
  string szOutDir="diff/";
  if( argc >= 4 ){
     szOutDir = argv[3];
  }
  
  CBgFits reffits( reffile.c_str() );
  reffits.ReadFits( reffile.c_str() );
  
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
     CBgFits fits;
     for(int i=0;i<fits_list.size();i++){
        printf("Reading image %s ...\n",fits_list[i].c_str());
        if( fits.ReadFits( fits_list[i].c_str() ) ){
           printf("ERROR : could not read fits file %s on the list\n",fits_list[i].c_str());
           exit(-1); 
        }else{
           printf("OK : fits file %s read ok\n",fits_list[i].c_str());
        }     
        
        fits.Subtract( reffits );
        
        char szOutFile[256];
        sprintf(szOutFile,"%s/diff_%s",szOutDir.c_str(),fits_list[i].c_str());
        fits.WriteFits(szOutFile);
     }
  }
}

