#include "bg_vis.h"
#include <myfile.h>

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
   sprintf(szImagName,"%s_%s_IM.fits",m_basename.c_str(),m_postfix.c_str());
   
   if( !MyFile::DoesFileExist( szRealName ) ){
      printf("WARNING : file %s does not exist !\n",szRealName);
      return -1;
   }
   if( !MyFile::DoesFileExist( szImagName ) ){
      printf("WARNING : file %s does not exist !\n",szImagName);
      return -1;
   }
   
   if( m_real.ReadFits( szRealName ) ){
      printf("ERROR : could not read FITS file %s\n",szRealName);      
      return ret;
   }

   if( m_imag.ReadFits( szImagName ) ){
      printf("ERROR : could not read FITS file %s\n",szImagName);      
      return ret;
   }
      
   return 0;
}

