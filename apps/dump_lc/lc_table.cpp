#include "lc_table.h"
#include <stdio.h>
#include <math.h>
#include <myfile.h>
#include <bg_globals.h>
#include <bg_fits.h>

double CLcTable::m_MinModulationIndex = -1000;
double CLcTable::m_MinChi2 = -1000;
bool   CLcTable::m_bUseMedian=true;

static int debug=10;

CLightcurvePoint::CLightcurvePoint( double uxtime, double flux, double stddev_noise  )
: m_unixtime(uxtime), m_flux(flux), m_stddev_noise(stddev_noise)
{
//   if(debug>=0){
//      printf("DEBUG_UNIXTIME (%x) : %.4f %.4f\n",this,m_unixtime,m_flux);
//      debug--;
//   }
}

CLightcurve::CLightcurve()
{
}

void CLightcurve::GetStat( double& mean, double& rms, double& median, double& rms_iqr, double& chi2, double& mean_weighted, bool bUseMedian )
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

   double mean_robust = mean;
   if( bUseMedian ){
      mean_robust = median;
   }
   double sum_w = 0.00 , sum_ww = 0.00;   
   chi2 = 0.00;
   for(int i=0;i<cnt;i++){
      double value = (*this)[i].m_flux;
      double stddev_noise = (*this)[i].m_stddev_noise;
      double stddev_noise2 = (stddev_noise*stddev_noise);
      
      chi2 += ( (value-mean_robust)*(value-mean_robust) ) / (stddev_noise2);
      
      sum_ww += (1.00/stddev_noise2);
      sum_w  += (value/stddev_noise2);
   }
   chi2 = chi2 / (cnt-1);
   
   mean_weighted = sum_w / sum_ww;
}

int CLightcurve::SaveLC(int x, int y, const char* outdir, double min_mod_index,  double& rms_out, double& modidx_out, double& chi2, bool bUseMedian )
{
    double mean, rms, median, rms_iqr, mean_weighted;
    GetStat( mean, rms, median, rms_iqr, chi2, mean_weighted, bUseMedian );
    double modulation_index = fabs( rms_iqr / median );
    double modulation_index_robust = fabs( rms / mean_weighted );
    if( bUseMedian ){
       modulation_index_robust = fabs( rms_iqr / mean_weighted );
    }
    rms_out     = rms_iqr;
    modidx_out  = modulation_index_robust;
    
//    printf("DEBUG UXTIME - saving lightcurve at pixel (%d,%d)\n",x,y);
    
    int n_saved=0;
//    if( modulation_index > min_mod_index || fabs(rms/mean) > min_mod_index ){
    if( modulation_index_robust > min_mod_index ){
       if( chi2 > CLcTable::m_MinChi2 ){
          char szOutFileName[512];
          sprintf(szOutFileName,"%s/pixel_%05d_%05d.txt",outdir,x,y);

          MyOFile out_f(szOutFileName,"a+");
          out_f.Printf("# LIGHTCURVE STATISTICS : MEAN = %.8f , RMS = %.8f , MEDIAN = %.8f , RMS_IQR = %.8f , MEAN_WEIGHTED = %.8f\n",mean,rms,median,rms_iqr,mean_weighted);
          out_f.Printf("# LIGHTCURVE MOD-INDEX  : MOD_INDEX = RMS_IQR / MEDIAN = %.8f ( RMS/MEAN = %.8f ) , CHI2 = %.8f , MOD_INDEX_EQ3 = %.8f\n",modulation_index,fabs(rms/mean),chi2,modulation_index_robust);
          out_f.Printf("# Number of points = %d\n",size());
          out_f.Printf("# UNIXTIME FLUX[Jy] STDDEV_NOISE[Jy]\n");
//       out_f.Printf("# Number of points = %d (debug ptr = %x)\n",size(),this);
          
    
          for(int i=0;i<size();i++){
             CLightcurvePoint& point = (*this)[i];
           
             out_f.Printf("%.6f %.6f %.6f\n",point.m_unixtime,point.m_flux,point.m_stddev_noise);
       
             n_saved++;
          }
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

CLightcurve* CLcTable::setXY( int x, int y, double uxtime, double flux, double stddev_noise )
{
   CLightcurve* lc = getXY(x,y);
   if( !lc ){
      lc = new CLightcurve();
      setXY(x,y,lc);      
   }
   
   lc->push_back( CLightcurvePoint(uxtime,flux,stddev_noise) );   
   
   return lc;
}

int CLcTable::SaveLC(const char* outdir, int BorderStartX, int BorderStartY, int BorderEndX, int BorderEndY )
{
   MyFile::CreateDir( outdir );
   
   CBgFits rms_map( m_SizeX, m_SizeY ), modidx_map( m_SizeX, m_SizeY ), chi2_map( m_SizeX, m_SizeY );
   double rms,modidx,chi2;
   
   for(int y=BorderStartY;y<BorderEndY;y++){
      if( (y%10) == 0 ){
         printf("Y = %d\n",y);         
      }
      for(int x=BorderStartX;x<BorderEndX;x++){
        CLightcurve* lc = getXY( x, y );

        if( lc ){
           lc->SaveLC(x,y,outdir,CLcTable::m_MinModulationIndex,rms, modidx, chi2, CLcTable::m_bUseMedian );
           
           chi2_map.setXY(x,y,chi2);
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

   sprintf(szOutFits,"%s/chi2.fits",outdir);
   if( chi2_map.WriteFits(szOutFits) ){
      printf("ERROR : could not save FITS file %s\n",szOutFits);
   }else{
     printf("INFO : saved FITS %s\n",szOutFits);
   }
   
   printf("DEBUG : end of CLcTable::SaveLC\n");fflush(stdout);
   
   return 1;
}

