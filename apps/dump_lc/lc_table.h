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

   CLightcurvePoint( double uxtime, double flux );
};

class CLightcurve : public vector<CLightcurvePoint>
{
public :
  CLightcurve();
  
  int SaveLC(int x, int y, const char* outdir, double min_mod_index, double& rms_out, double& modidx_out );
  void GetStat( double& mean, double& rms, double& median, double& rms_iqr );
};

class CLcTable : public vector<CLightcurve*>
{
public: 
   CLcTable( int sizeX , int sizeY );
   ~CLcTable();
   
   int m_SizeX;
   int m_SizeY;
   
   static double m_MinModulationIndex;
   
//   void Alloc( int sizeX , int sizeY );   
   CLightcurve* getXY(int x, int y);
   void  setXY(int x, int y, CLightcurve* ptr );
   CLightcurve* setXY( int x, int y, double uxtime, double flux );
   
   int SaveLC(const char* outdir, int BorderStartX, int BorderStartY, int BorderEndX, int BorderEndY );
};

#endif