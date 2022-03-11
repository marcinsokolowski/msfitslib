#ifndef _LC_TABLE_H__
#define _LC_TABLE_H__

#include <vector>
using namespace std;

// Assign : https://www.cplusplus.com/reference/vector/vector/assign/

class CLightcurvePoint
{
public :
   double m_unixtime;
   double m_flux;
   double m_stddev_noise; // local noise 

   CLightcurvePoint( double uxtime, double flux, double stddev_noise );
};

class CLightcurve : public vector<CLightcurvePoint>
{
public :
  CLightcurve();
  
  int SaveLC(int x, int y, const char* outdir, double min_mod_index, double& rms_out, double& modidx_out, double& chi2, bool bUseMedian=true );
  void GetStat( double& mean, double& rms, double& median, double& rms_iqr, double& chi2, double& mean_weighted, bool bUseMedian=true );
};

class CLcTable : public vector<CLightcurve*>
{
public: 
   CLcTable( int sizeX , int sizeY );
   ~CLcTable();
   
   int m_SizeX;
   int m_SizeY;
   
   static double m_MinModulationIndex;
   static double m_MinChi2;
   static bool   m_bUseMedian; // to calculate robust Chi2 and modulation index as in Martin Bell et al. (2016), eq. 1,2,3 
   
//   void Alloc( int sizeX , int sizeY );   
   CLightcurve* getXY(int x, int y);
   void  setXY(int x, int y, CLightcurve* ptr );
   CLightcurve* setXY( int x, int y, double uxtime, double flux, double stddev_noise );
   
   int SaveLC(const char* outdir, int BorderStartX, int BorderStartY, int BorderEndX, int BorderEndY );
};

#endif