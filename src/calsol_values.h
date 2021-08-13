#ifndef _CALSOL_VALUES_H_
#define _CALSOL_VALUES_H_

#include "bg_globals.h"

class CBgArray;

class cCalSolsVsUxtime
{
public :
   cCalSolsVsUxtime();
   
   double unix_time;
   std::vector<double> antenna_cal_solutions;
};

class CalSolValues : public vector<cCalSolsVsUxtime>
{
public :
   CalSolValues();
//   CValueVector& operator=( const CValueVector& right );

   void init(int size);
   int read_file(const char* file, int db2num=0 );
   cCalSolsVsUxtime* find_value( double x, double precision=0.01 );
   cCalSolsVsUxtime* find_closest_value( double x , double max_dist=1800.00 ); // 1/2 hour if .x is uxtime 
};

#endif
