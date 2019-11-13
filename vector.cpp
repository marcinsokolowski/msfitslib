#include "vector.h"
#include <math.h>
#include "mathdefs.h"

CCmnVector::CCmnVector( double vx, double vy, double vz )
{
  v[0] = vx;
  v[1] = vy;
  v[2] = vz;
}


CCmnVector::CCmnVector( double _v[3] )
{
  v[0] = _v[0];
  v[1] = _v[1];
  v[2] = _v[2];
}

CCmnVector CCmnVector::V_cross_product( const CCmnVector& V2 )
{
  return V_cross_product( (*this) , V2 );
}

CCmnVector CCmnVector::V_cross_product( const CCmnVector& V1, const CCmnVector& V2 )
{
  double vx = (V1.v[1]*V2.v[2] - V2.v[1]*V1.v[2]);
  double vy = (V1.v[2]*V2.v[0] - V2.v[2]*V1.v[0]);
  double vz = (V1.v[0]*V2.v[1] - V2.v[0]*V1.v[1]);

  return CCmnVector(vx,vy,vz);  
}

double CCmnVector::V_scalar_product( const CCmnVector& V2 )
{
  return V_scalar_product( (*this), V2 );
}

double CCmnVector::V_scalar_product( const CCmnVector& V1, const CCmnVector& V2 )
{
  double ret = ( V1.v[0]*V2.v[0] + V1.v[1]*V2.v[1] + V1.v[2]*V2.v[2] );
  return ret;
}

CCmnVector CCmnVector::V_create_RaDec1(double ra_in_rad, double dec_in_rad )
{
  return V_create_spherical(1.00,ra_in_rad,(PI_VALUE/2.00)-dec_in_rad);
}

CCmnVector CCmnVector::V_create_spherical(double R,       // XYZ spherical radius > 0
                                          double Phi,     // XYZ spherical Phi [rad]
                                          double Theta)   // XYZ spherical Theta [rad]
{                                        
   double factor=fabs(R)*sin(Theta);
   return CCmnVector( cos(Phi)*factor, sin(Phi)*factor, fabs(R)*cos(Theta) );
} 

                              