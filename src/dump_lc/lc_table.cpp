#include "lc_table.h"
#include <stdio.h>
#include <math.h>
#include <myfile.h>
#include <bg_globals.h>
#include <bg_fits.h>

double CLcTable::m_MinModulationIndex = -1000;

static int debug=10;

CLightcurvePoint::CLightcurvePoint( double uxtime, double flux )
: m_unixtime(uxtime), m_flux(flux)
{
//   if(debug>=0){
//      printf("DEBUG_UNIXTIME (%x) : %.4f %.4f\n",this,m_unixtime,m_flux);
//      debug--;
//   }
}

CLightcurve::CLightcurve()
{
}

void CLightcurve::GetStat( double& mean, double& rms, double& median, double& rms_iqr )
{
   vector<double> values;
   int cnt = size();
   
   double sum=0.00,sum2=0.00; 
   for(int i=0;i<cnt;i++){
       double value = (*this)[i].m_flux;
       values.push_back( value );
       
       sum += value;
       sum2 += (value*value);
   }
   
   my_sort_float( values );

   int q75= int(cnt*0.75);
   int q25= int(cnt*0.25);

   median = values[ cnt / 2 ];
   double iqr = values[q75] - values[q25];
   rms_iqr = iqr / 1.35;
   
   mean = sum/cnt;
   rms = sqrt( (sum2/cnt) - mean*mean );
}

int CLightcurve::SaveLC(int x, int y, const char* outdir, double min_mod_index,  double& rms_out, double& modidx_out )
{
    double mean, rms, median, rms_iqr;
    GetStat( mean, rms, median, rms_iqr );
    double modulation_index = fabs( rms_iqr / median );
    rms_out     = rms_iqr;
    modidx_out  = modulation_index;
    
//    printf("DEBUG UXTIME - saving lightcurve at pixel (%d,%d)\n",x,y);
    
    int n_saved=0;
    if( modulation_index > min_mod_index || fabs(rms/mean) > min_mod_index ){
       char szOutFileName[512];
       sprintf(szOutFileName,"%s/pixel_%05d_%05d.txt",outdir,x,y);

       MyOFile out_f(szOutFileName,"a+");
       out_f.Printf("# MEAN = %.8f , RMS = %.8f , MEDIAN = %.8f , RMS_IQR = %.8f , MOD_INDEX = RMS_IQR / MEDIAN = %.8f ( RMS/MEAN = %.8f )\n",mean,rms,median,rms_iqr,modulation_index,fabs(rms/mean));
//       out_f.Printf("# Number of points = %d (debug ptr = %x)\n",size(),this);
       out_f.Printf("# Number of points = %d\n",size());
    
       for(int i=0;i<size();i++){
          CLightcurvePoint& point = (*this)[i];
           
          out_f.Printf("%.4f %.4f\n",point.m_unixtime,point.m_flux);
       
          n_saved++;
       }
    }
    return n_saved;
}

CLcTable::CLcTable( int sizeX , int sizeY )
: m_SizeX(sizeX) , m_SizeY(sizeY)
{
   int size = m_SizeX*m_SizeY;
   assign( size, NULL );
}

/*void CLcTable::Alloc( int sizeX , int sizeY )
{
   m_SizeX = sizeX;
   m_sizeY = sizeY;
   
   int size = m_SizeX*m_SizeY;
   assign( size, NULL );
}*/

CLcTable::~CLcTable()
{
   for(int i=0;i<size();i++){
      CLightcurve* ptr = (*this)[i];

      if (ptr ){
         delete ptr;
      }
   }
}
   
CLightcurve* CLcTable::getXY(int x, int y)
{
   int pos = y*m_SizeX + x;
   if ( pos < (m_SizeX*m_SizeY) ){   
      CLightcurve* ptr = (*this)[pos];
      
      return ptr;
   }else{
      printf("ERROR in code : requested element at (x,y) = (%d,%d) - index = %d from the array size (%d,%d) size = %d\n",x,y,pos,m_SizeX,m_SizeY,(m_SizeX*m_SizeY));   
   }
   
   return NULL;
}

void  CLcTable::setXY(int x, int y, CLightcurve* ptr )
{
   int pos = y*m_SizeX + x;
   if ( pos < (m_SizeX*m_SizeY) ){   
      (*this)[pos] = ptr;
   }else{
      printf("ERROR in code : requested to set element at (x,y) = (%d,%d) - index = %d in the array size (%d,%d) size = %d\n",x,y,pos,m_SizeX,m_SizeY,(m_SizeX*m_SizeY));   
   }
}

CLightcurve* CLcTable::setXY( int x, int y, double uxtime, double flux )
{
   CLightcurve* lc = getXY(x,y);
   if( !lc ){
      lc = new CLightcurve();
      setXY(x,y,lc);      
   }
   
   lc->push_back( CLightcurvePoint(uxtime,flux) );   
   
   return lc;
}

int CLcTable::SaveLC(const char* outdir, int BorderStartX, int BorderStartY, int BorderEndX, int BorderEndY )
{
   MyFile::CreateDir( outdir );
   
   CBgFits rms_map( m_SizeX, m_SizeY ), modidx_map( m_SizeX, m_SizeY );
   double rms,modidx;
   
   for(int y=BorderStartY;y<BorderEndY;y++){
      for(int x=BorderStartX;x<BorderEndX;x++){
        CLightcurve* lc = getXY( x, y );

        if( lc ){
           lc->SaveLC(x,y,outdir,CLcTable::m_MinModulationIndex,rms,modidx);
           
           rms_map.setXY(x,y,rms);
           modidx_map.setXY(x,y,modidx);
        }
      }
   }
   
   char szOutFits[1024];
   sprintf(szOutFits,"%s/rmsmap.fits",outdir);
   if( rms_map.WriteFits(szOutFits) ){
      printf("ERROR : could not save FITS file %s\n",szOutFits);
   }else{
      printf("INFO : saved FITS %s\n",szOutFits);
   }
   
   sprintf(szOutFits,"%s/modidx.fits",outdir);
   if( modidx_map.WriteFits(szOutFits) ){
      printf("ERROR : could not save FITS file %s\n",szOutFits);
   }else{
     printf("INFO : saved FITS %s\n",szOutFits);
   }
}
