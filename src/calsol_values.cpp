#include "calsol_values.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "bg_array.h"

#include <myfile.h>
#include <myparser.h>
#include <mystrtable.h>


cCalSolsVsUxtime::cCalSolsVsUxtime()
{}

CalSolValues::CalSolValues()
{}

void CalSolValues::init(int size)
{
  //   vec.clear();
  reserve(size);
  cCalSolsVsUxtime val;
  val.unix_time = 0.00;
            
  for(int i=0;i<size;i++){
     push_back( val );
  }   
}                           

int CalSolValues::read_file(const char* file, int db2num )     
{
//   return ::read_file(file, (*this), db2num, x_col, y_col, min_col );
   MyFile infile(file);
   const char* pLine=NULL;
   clear();
   
   cCalSolsVsUxtime tmp;

   while( pLine = infile.GetLine( TRUE ) ){
      if( mystring::get_first_non_white( pLine )=='#' ){
//         printf("Line skipped : %s\n",pLine);
         continue;
      }
                        
      MyParser pars=pLine;
      CMyStrTable items;
      pars.GetItems( items );
      
      tmp.unix_time = atof( items[0].c_str() );
      tmp.antenna_cal_solutions.clear();

      for(int i=1;i<items.size();i++){
         double ant_phase = atof( items[i].c_str() );
         tmp.antenna_cal_solutions.push_back( ant_phase );
      }      
      
      push_back( tmp );
   }
   printf("Read %d cal. sol. lines from file %s\n",(int)size(),file);
   
   return size();

}


cCalSolsVsUxtime* CalSolValues::find_closest_value( double x , double max_dist /* =1800.00 */ )
{
   double min_dist = 1e20;
   int ret_index = -1;
   
   for(int i=0;i<size();i++){
      cCalSolsVsUxtime& val = (*this)[i];
      
      double diff = fabs( val.unix_time - x );
      if( diff < min_dist && diff <= max_dist ){
         ret_index = i;
      }      
    }
    
    if( ret_index >= 0 ){
       return &((*this)[ret_index]);
    }
    
    return NULL;
}

cCalSolsVsUxtime* CalSolValues::find_value( double x, double precision )
{
   for(int i=0;i<size();i++){
      cCalSolsVsUxtime& val = (*this)[i];
      
      double diff = fabs( val.unix_time - x );
      if ( diff <= precision ){
        return &((*this)[i]);
      }
   }
   
   return NULL;
}

