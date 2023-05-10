#include "transient_finder.h"
#include "../src/bg_fits.h"
#include <math.h>

CCandidate::CCandidate( double _x, double _y,  // image coordinates (X,Y)
                        double _flux,  // flux density of the candidate
                        double _bkg,  // mean value in image or locally as an estimate of background level
                        double _rms,           // rms of the candidate (may be local or constant for the whole image)
                        double _ra /*=-1000*/, // RA [deg decimal]
                        double _dec /*=-1000*/ // DEC [deg decimal]
                      )
: x(_x), y(_y), flux(_flux), rms(_rms), ra(_ra), dec(_dec), bkg(_bkg), azim(-1000), elev(-1000), uxtime(-1), min_value_image(0), max_value_image(0)
{

}

void CCandidate::UpdateCandidate( double _x, double _y, double _flux, double _bkg, double _rms )
{
   x = _x;
   y = _y;
   flux = _flux;
   rms = _rms;
   bkg = _bkg;
}


CTransientFinder::CTransientFinder()
{
}

CTransientFinder::~CTransientFinder()
{

}

CCandidate* CTransientFinder::FindCandidate( double _x, double _y, double radius_in_pixels )
{
   for(int i=0;i<size();i++){
      CCandidate* pCand = &((*this)[i]);

      double dist = sqrt( (pCand->x - _x)*(pCand->x - _x) + (pCand->y - _y)*(pCand->y - _y) );

      if( dist <= radius_in_pixels ){
         return pCand;
      }
   }

   return NULL;
}



int CTransientFinder::FindTransientCandidates( CBgFits* pDiffImage, const char* szFitsBaseName, 
                                               double threshold_in_sigma    /* =5.00 */,
                                               double synth_beam_size_in_px /* =3 */     // assuming that the synth. beam is oversampled by a factor of 3
                                             )
{
   if( pDiffImage ){
       clear(); // clear the list (remove all candidates)
   
       // GetStatBorder( double& mean, double& rms, double& minval, double& maxval, int border )
       double mean, rms, minval, maxval;
       int border=20; // ignore 20 pixels border
       // TODO : enable usage of local RMS by calculating an RMS map:
       pDiffImage->GetStatBorder( mean, rms, minval, maxval, border );
       printf("STAT ( %s ) : ( mean , rms , minval , maxval , maxval_snr ) = ( %.6f , %.6f , %.6f , %.6f , %.6f )\n",szFitsBaseName,mean, rms, minval, maxval, maxval/rms );
       
       int xSize = pDiffImage->GetXSize();
       int ySize = pDiffImage->GetYSize();
       
       for(int y=0;y<ySize;y++){
          for(int x=0;x<xSize;x++){
             double flux = pDiffImage->getXY(x,y);
             double snr = flux/rms;
             
             if( snr > threshold_in_sigma ){
                // candidate found, check if it has the highest RMS
                // use radius of 1 synthesised beam (in pixels = synth_beam_size_in_px ):
                // TODO : find the highest SNR candidate
                CCandidate* pCand = FindCandidate( x, y, synth_beam_size_in_px );
                
                if( pCand ){
                   if( snr > (pCand->flux/pCand->rms) ){
                      // update candidate 
                      pCand->UpdateCandidate( x, y, flux, mean, rms );
                   }
                }else{
                   push_back( CCandidate( x, y, flux, mean, rms ) );
                }
             }
          }
       }
       
       return size();
   }
   
   return 0;
}

int CTransientFinder::SaveCandidates( const char* szBaseName, 
                                      double threshold_in_sigma, 
                                      const char* szOutDir /*="./"*/ )
{
   char szOutTxtFile[512], szOutRegFile[512];
   sprintf(szOutTxtFile,"%s/%s_cand.txt",szOutDir,szBaseName);
   sprintf(szOutRegFile,"%s/%s_cand.reg",szOutDir,szBaseName);
   
   FILE* out_txt_f = fopen( szOutTxtFile, "w" );
   if( !out_txt_f ){
      printf("ERROR : could not open output file %s\n",szOutTxtFile);
      return -1;
   }
   
   FILE* out_reg_f = fopen( szOutRegFile, "w" );
   if( !out_reg_f ){
      printf("ERROR : could not open output file %s\n",szOutRegFile);
      return -1;
   }
   
   fprintf(out_reg_f,"global color=white width=5 font=\"times 10 normal\"\n");
   fprintf(out_txt_f,"# FITSNAME X Y FLUX[Jy] SNR ThreshInSigma RMS RA[deg] DEC[deg] AZIM[deg] ELEV[deg] UXTIME IMAGE_MIN IMAGE_MAX\n");

   for(int i=0;i<size();i++){
      CCandidate* pCand = &((*this)[i]);
      double snr = ((pCand->flux - pCand->bkg)/pCand->rms);
       
      fprintf( out_reg_f, "circle %d %d %d # %.2f Jy = %.2f sigmas (>=%.2f x %.2f + %.2f), %s ,  (ra,dec) = ( %.6f , %.6f ) [deg]\n", int(pCand->x+1),int(pCand->y+1),int(snr),
                                                                                        pCand->flux,snr,threshold_in_sigma,pCand->rms,pCand->bkg,szBaseName,pCand->ra,pCand->dec );
                                                                                                                                                                                
      fprintf( out_txt_f, "%s %d %d %.2f %.2f %.2f %.2f %.6f %.6f %.6f %.6f %.2f %.4f %.4f\n",szBaseName,int(pCand->x),int(pCand->y),pCand->flux,(pCand->flux/pCand->rms),threshold_in_sigma,pCand->rms,pCand->ra,pCand->dec,pCand->azim,pCand->elev,pCand->uxtime,pCand->min_value_image,pCand->max_value_image ) ;
   }
   
   fclose( out_txt_f );
   fclose( out_reg_f );
}  


                                    