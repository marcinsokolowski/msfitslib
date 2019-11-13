#include "bg_vis.h"

CBgVis::CBgVis( const char* basename, const char* postfix )
: m_basename( basename ), m_postfix( postfix )
{

}

int CBgVis::Read( const char* basename , const char* postfix )
{
   m_basename = basename;
   m_postfix = postfix;
   
   int ret = 0;
   
   char szRealName[1024],szImagName[1024];
   
   sprintf(szRealName,"%s_%s_RE.fits",m_basename.c_str(),m_postfix.c_str());
   if( m_real.ReadFits( szRealName ) ){
      printf("ERROR : could not read FITS file %s\n",szRealName);      
      return ret;
   }

   sprintf(szImagName,"%s_%s_IM.fits",m_basename.c_str(),m_postfix.c_str());
   if( m_imag.ReadFits( szImagName ) ){
      printf("ERROR : could not read FITS file %s\n",szImagName);      
      return ret;
   }
      
   return 0;
}

