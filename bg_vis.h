#ifndef _BG_VIS_H_
#define _BG_VIS_H_

#include "bg_fits.h"
#include <string>

class CBgVis
{
public :
   CBgFits m_real;
   CBgFits m_imag;
   
   std::string m_basename;
   std::string m_postfix; // CorrMatrix , SimulMatrix or anything else 
   
   CBgVis( const char* basename="", const char* postfix="CorrMatrix" );
   int Read( const char* basename="" , const char* postfix="CorrMatrix" );
};

#endif