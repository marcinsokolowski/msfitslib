#ifndef _CMN_VECTOR_H__
#define _CMN_VECTOR_H__

class CCmnVector
{
public :
  double v[3]; // kartesian vector 
  
  CCmnVector( double vx, double vy, double  vz );
  CCmnVector( double _v[3] );

  // Creates cartesian vector (X,Y,Z) from spherical coordinates (R,Theta,Phi) :
  static CCmnVector V_create_spherical(double R,       // XYZ spherical radius > 0
                                double Phi,     // XYZ spherical Phi [rad]
                                double Theta);   // XYZ spherical Theta [rad]

  // Creates vector (X,Y,Z) from spherical coordinates (RA,DEC) :
  static CCmnVector V_create_RaDec1(double ra_in_rad, double dec_in_rad );
                                                                                      

  CCmnVector V_cross_product( const CCmnVector& V2 );
  double V_scalar_product( const CCmnVector& V2 );
  
  static CCmnVector V_cross_product( const CCmnVector& V1, const CCmnVector& V2 );
  static double V_scalar_product( const CCmnVector& V1, const CCmnVector& V2 );
    
};

#endif
