#ifndef _TRANSIENT_FINDER_H__
#define _TRANSIENT_FINDER_H__

#include <vector>

class CBgFits;

class CCandidate
{
public :
   CCandidate( double _x, double _y, // image coordinates (X,Y)
               double _flux, // flux density of the candidate
               double _bkg,  // mean value in image or locally as an estimate of background level
               double _rms,          // rms of the candidate (may be local or constant for the whole image)
               double _ra=-1000,     // RA [deg decimal]
               double _dec=-1000     // DEC [deg decimal]
             );

   void UpdateCandidate( double _x, double _y, double _flux, double _bkg, double _rms );
   
   double x;
   double y;
   double flux;
   double bkg;
   double rms;
   double ra; // decimal degrees
   double dec;// decimal degrees
   double azim; // decimal degrees
   double elev; // decmial degrees
   double uxtime;
   double min_value_image;
   double max_value_image;

};

// class a list of candidates 
class CTransientFinder : public std::vector<CCandidate>
{
public:
   CTransientFinder();
   ~CTransientFinder();

   int SaveCandidates( const char* szBaseName, double threshold_in_sigma, const char* szOutDir="./" );
   
   int FindTransientCandidates( CBgFits* pDiffImage, const char* szFitsBaseName,
                                double threshold_in_sigma=5.00, // threshold in sigma 
                                double synth_beam_size_in_px=3  // assuming that the synth. beam is oversampled by a factor of 3
                              );   
                              
   // find if the candidate already exists on a list (within the specified distance in pixels)
   // and update if a higher SNR is found or add a new candidate :
   CCandidate* FindCandidate( double _x, double _y, double radius_in_pixels );
                              
   
};

#endif
