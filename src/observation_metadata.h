#ifndef _OBSERVATION_METADATA_H__
#define _OBSERVATION_METADATA_H__

#include <vector>
#include <cstring>
#include <string>
#include <map>

// based on mwaconfig.h(cpp) and metafitsfile.h(cpp) in cotter 

class CObsMetadata 
{
public :
   CObsMetadata();

   size_t nInputs;                 // the number of inputs to the correlator
   size_t nScans;                  // the number of time samples
   size_t nChannels;               // the number of spectral channels
   enum CorrType { None, CrossCorrelation, AutoCorrelation, BothCorrelations } correlationType;
   double integrationTime;         // per time sample, in seconds
   double centralFrequencyMHz;     // observing central frequency and 
   double bandwidthMHz;            // bandwidth (MHz)
   double haHrs, raHrs, decDegs;          // ra,dec of phase centre.
   double haHrsStart;              // the HA of the phase center at the start of the integration
   double refEl, refAz;            // the el/az of the normal to the plane of the array (radian)
   int    year, month, day;        // date/time in UTC.
   int    refHour, refMinute;
   double refSecond;
   //bool   invertFrequency;         // flag to indicate that freq decreases with increasing channel number.
   bool   conjugate;               // conjugate the final vis to correct for any sign errors
   bool   geomCorrection;          // apply geometric phase correction
   std::string fieldName;
   std::string polProducts;

   double dateFirstScanMJD;        // The central time of the first time step. Not in header file, derived from it.
   
   // part from MWAHeaderExt
   int gpsTime;
   std::string observerName, projectName, gridName, mode, filename, mwaPyVersion, mwaPyDate, metaDataVersion;
   int delays[16], subbandGains[24], subbandNumbers[24];
   bool hasCalibrator, hasGlobalSubbandGains;
   int centreSBNumber;
   //double fiberFactor;
   double tilePointingRARad, tilePointingDecRad;
   double dateRequestedMJD;

   double GetDateLastScanMJD() const
   {
      return dateFirstScanMJD + (integrationTime/86400.0)*nScans;
   }
   
   // TODO :
//   double GetStartDateMJD() const;
//   double GetDateFirstScanFromFields() const;
   
   //------------------------------------------------------------------------------------------------------------------------------
   // Read metadata 
   // reading meta data from text file : 
   bool ReadMetaDataTxt( const char* filename );
   
   bool ReadMetaFitsFile( const char* filename );

   // parsing Metafits file information :
   bool parseKeyword( const std::string& keyName, const std::string& keyValue );
   bool parseFitsString(const char* valueStr, std::string& outString );
   std::string stripBand(const std::string& input);
   bool parseFitsDate(const char* valueStr, int& year, int& month, int& day, int& hour, int& min, double& sec);
   double parseFitsDateToMJD(const char* valueStr);
   bool parseIntArray(const char* valueStr, int* values, size_t count);
   bool parseBool(const char* valueStr);

   // error checking :
   bool checkStatus(int status, const char* szErrorMsg );
   

   // void Validate(bool lockPointing);
   private:
      CObsMetadata(const CObsMetadata &) { }
      void operator=(const CObsMetadata &) { }
      
      static bool isDigit(char c) { return c >= '0' && c <= '9'; }
};


#endif
