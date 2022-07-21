// --------------------------------------------------------------------------------------------
// based on mwaconfig.h(cpp) and metafitsfile.h(cpp) in cotter
// --------------------------------------------------------------------------------------------
#include "observation_metadata.h"
#include "myfile.h"
#include "myparser.h"
#include "mystrtable.h"
#include "libnova_interface.h"
#include <math.h>
#include <fitsio.h>

#define MWA_LATTITUDE -26.703319        // Array latitude. degrees North
#define MWA_LONGITUDE 116.67081         // Array longitude. degrees East
#define MWA_HEIGHT 377               // Array altitude. meters above sea level

// #include <cstdlib>
// #include <cmath>

// #include <fstream>
// #include <sstream>
// #include <iostream>

// #include <stdexcept>

// #include "geometry.h"
// #include "metafitsfile.h"

#define VEL_FACTOR  1.204         // the velocity factor of electic fields in RG-6 like coax.

#define DEFAULT_VALUE -1000

bool CObsMetadata::checkStatus(int status, const char* szErrorMsg )
{
   if( status ){
      printf("%s, status = %d\n",szErrorMsg,status);      
      return false;      
   }
   
   return true;
}

CObsMetadata::CObsMetadata( const char* filename ) :
        nInputs(0),
        nScans(0),
        nChannels(0),
        correlationType(None),
        integrationTime(0.0),
        centralFrequencyMHz(0.0),
        bandwidthMHz(0.0),
        haHrs(DEFAULT_VALUE),
        raHrs(DEFAULT_VALUE),
        decDegs(DEFAULT_VALUE),
        haHrsStart(DEFAULT_VALUE),
        refEl(M_PI*0.5),
        refAz(0.0),
        year(0),
        month(0),
        day(0),
        refHour(0),
        refMinute(0),
        refSecond(0.0),
        conjugate(false),
        geomCorrection(true),
        fieldName(),
        polProducts("XXXYYXYY"),
        
        // from MWAHeaderExt
        gpsTime(0), observerName("Unknown"), projectName("Unknown"),
        gridName("Unknown"), mode("Unknown"),
        hasCalibrator(false), hasGlobalSubbandGains(false),
        centreSBNumber(0),
        //fiberFactor(VEL_FACTOR),
        tilePointingRARad(0.0), tilePointingDecRad(0.0),
        dateRequestedMJD(0.0)

{
   for(size_t i=0; i!=16; ++i) delays[i] = 0;
   for(size_t i=0; i!=24; ++i) subbandGains[i] = 0;
   
   if( filename && strlen(filename) ){
      m_filename = filename;
   }
}


bool CObsMetadata::ReadMetaData( const char* filename )
{
   if( strlen(m_filename.c_str())<=0 || strcmp(m_filename.c_str(),filename) ){
      m_filename = filename;
   }

   if( strlen(m_filename.c_str()) ){
      if( strstr(m_filename.c_str(),".txt") ){
         return ReadMetaDataTxt( m_filename.c_str() );
      }
      if( strstr(m_filename.c_str(),".metafits") ){
         return ReadMetaFitsFile( m_filename.c_str() );
      }
   }
   
   printf("ERROR : no filename specified\n");
   return false;   
}


bool CObsMetadata::ReadMetaDataTxt( const char* filename )
{
   if( filename && strlen(filename) ){
      if( !MyFile::DoesFileExist( filename ) ){
         printf("ERROR: filename %s does not exist\n",filename);
         return false;
      }
   }else{
      printf("ERROR : empty filename provided -> cannot continue\n");
      return false;
   }

//   clear();
   int n_ant_index = 0;
   MyFile file(filename);
   const char* pLine;
   if(!file.IsOpened()){
      file.Open( filename );
   }
   while(pLine = file.GetLine(TRUE)){
      if(strlen(pLine) && pLine[0]!='#'){
         MyParser pars=pLine;
         CMyStrTable items;
         pars.GetItems( items );
         
         if( strcmp(items[0].c_str(), "HA" ) == 0 ){
            haHrs = atof( items[2].c_str() )/15.00; // deg -> h 
         }
         if( strcmp(items[0].c_str(), "RA" ) == 0 ){
            raHrs = atof( items[2].c_str() ) / 15.00; // deg -> h 
         }
         if( strcmp(items[0].c_str(), "DEC" ) == 0 ){
            decDegs = atof( items[2].c_str() ); // deg -> h 
         }
         
         // TODO : add more keywords as needed
      }
   }
   
   if(strcmp(m_filename.c_str(),filename)){
      m_filename = filename;
   }
   return true;
}

bool CObsMetadata::ReadMetaFitsFile( const char* filename )
{
   fitsfile *_fptr = NULL; 
   int status = 0;
   
   if(fits_open_file(&_fptr, filename, READONLY, &status)){
      printf("ERROR : could not read FITS file %s\n",filename);
      return false;
   }

   int hduCount;
   fits_get_num_hdus(_fptr, &hduCount, &status);
//   checkStatus(status);

   if(hduCount < 2){
      printf("ERROR : FITS file %s has less than 2 keywords -> cannot continue\n",filename);
      return false;     
   }
   
   // TODO : add reading FITS file here
   int hduType;
   fits_movabs_hdu(_fptr, 1, &hduType, &status);
   if( !checkStatus(status, "ERROR calling fits_movabs_hdu") ){ return false; }
        
   int keywordCount;
   fits_get_hdrspace(_fptr, &keywordCount, 0, &status);
   if( !checkStatus(status, "ERROR calling fits_get_hdrspace") ){ return false; }

   // initailise some keywords 
   tilePointingDecRad = DEFAULT_VALUE;
   tilePointingRARad  = DEFAULT_VALUE;
   raHrs   = DEFAULT_VALUE;
   decDegs = DEFAULT_VALUE;
   
   for(int i=0; i!=keywordCount; ++i){
      char keyName[80], keyValue[80], *keyValueLong, szErrorMsg[1024];
      fits_read_keyn(_fptr, i+1, keyName, keyValue, 0, &status);
      if( status ){
         printf("ERROR : could not read information about keyword %d using fits_read_keyn in fitsfile %s, status = %d\n",i,filename,status);
         return false;
      }

      std::string keyValueStr = keyValue;
      if(keyValueStr.size()>=3  && keyValueStr[0]=='\'' && keyValueStr[keyValueStr.size()-1]=='\'' && keyValueStr[keyValueStr.size()-2]=='&')
      {
         fits_read_key_longstr(_fptr, keyName, &keyValueLong, NULL, &status);
         if(status == 0){
            keyValueStr = std::string("'") + keyValueLong;
         }
         if(keyValueStr[keyValueStr.size()-1] == '&'){
            keyValueStr[keyValueStr.size()-1] = '\'';
         }else{
            keyValueStr += '\'';
         }
         if( status ){
            printf("ERROR : could not read keyword %d/%s using fits_read_key_longstr from fitsfile %s, status = %d\n",i,keyName,filename,status);
            return false;
         }
       
         fffree(keyValueLong, &status); // use the short name; some fftw version don't know the long name
         if( !checkStatus(status,"ERROR : fffree failed on variable keyValueLong") ){
            return false;
         }
      }

      if( !parseKeyword( keyName, keyValueStr ) ){
         printf("WARNING : could not parse keyword %s\n",keyName);
      }
   }

   correlationType = CObsMetadata::BothCorrelations;

   if( tilePointingDecRad == DEFAULT_VALUE || tilePointingRARad == DEFAULT_VALUE ){
      printf("The metafits file does not specify a pointing direction (keywords RA and DEC)");
      return false;
   }else{
      if(raHrs == DEFAULT_VALUE || decDegs == DEFAULT_VALUE )
        {
                printf("The metafits file does not specify a phase centre; will use pointing centre as phase\n");
                printf("centre, unless overridden on command line.\n");
                
                raHrs = tilePointingRARad * (12.0/M_PI);
                decDegs = tilePointingDecRad * (180.0/M_PI);
        }
   }
   
   if( _fptr ){
      if(fits_close_file(_fptr, &status)){
         printf("ERROR : could not close FITS file %s (fits_close_file status = %d)\n",filename,status);
         return false;
      }
   }
   
   if(strcmp(m_filename.c_str(),filename)){
      m_filename = filename;
   }
   
   return true;
}

bool CObsMetadata::parseKeyword( const std::string& keyName, const std::string& keyValue )
{
        if(keyName == "SIMPLE" || keyName == "BITPIX" || keyName == "NAXIS" || keyName == "EXTEND" || keyName == "CONTINUE")
                ; //standard FITS headers; ignore.
        else if(keyName == "GPSTIME")
                gpsTime = atoi(keyValue.c_str());
        else if(keyName == "FILENAME")
        {
                if( !parseFitsString(keyValue.c_str(), filename ) ){
                   return false;
                }
                // filename = parseFitsString(keyValue.c_str());
                fieldName = stripBand(filename);
        }
        else if(keyName == "DATE-OBS")
        {
                parseFitsDate(keyValue.c_str(), year, month, day, refHour, refMinute, refSecond);
                dateRequestedMJD = parseFitsDateToMJD(keyValue.c_str());
        }
        else if(keyName == "RAPHASE")
                raHrs = atof(keyValue.c_str()) * (24.0 / 360.0);
        else if(keyName == "DECPHASE")
                decDegs = atof(keyValue.c_str());
        else if(keyName == "RA")
                tilePointingRARad = atof(keyValue.c_str()) * (M_PI / 180.0);
        else if(keyName == "DEC")
                tilePointingDecRad = atof(keyValue.c_str()) * (M_PI / 180.0);
        else if(keyName == "GRIDNAME"){
                // gridName = parseFitsString(keyValue.c_str());
                if( !parseFitsString(keyValue.c_str(), gridName ) ){
                   return false;   
                }
        }else if(keyName == "CREATOR"){
                if( !parseFitsString(keyValue.c_str(),observerName) ){
                   return false;
                }
        }else if(keyName == "PROJECT")
                if( !parseFitsString(keyValue.c_str(),projectName) ){ 
                   return false;
                }
        else if(keyName == "MODE")
                if( !parseFitsString(keyValue.c_str(),mode) ){
                   return false;
                }
        else if(keyName == "DELAYS"){
                if( !parseIntArray(keyValue.c_str(), delays, 16) ){
                   return false;
                }
        }else if(keyName == "CALIBRAT")
                hasCalibrator = parseBool(keyValue.c_str());
        else if(keyName == "CENTCHAN")
                centreSBNumber = atoi(keyValue.c_str());
        else if(keyName == "CHANGAIN") {
                if( !parseIntArray(keyValue.c_str(), subbandGains, 24) ){
                   return false;
                }
                hasGlobalSubbandGains = true;
        }
        //else if(keyName == "FIBRFACT")
        //      fiberFactor = atof(keyValue.c_str());
        else if(keyName == "INTTIME")
                integrationTime = atof(keyValue.c_str());
        else if(keyName == "NSCANS")
                nScans = atoi(keyValue.c_str());
        else if(keyName == "NINPUTS")
                nInputs = atoi(keyValue.c_str());
        else if(keyName == "NCHANS")
                nChannels = atoi(keyValue.c_str());
        else if(keyName == "BANDWDTH")
                bandwidthMHz = atof(keyValue.c_str());
        else if(keyName == "FREQCENT")
                centralFrequencyMHz = atof(keyValue.c_str());
        else if(keyName == "CHANNELS"){
                if( !parseIntArray(keyValue.c_str(), subbandNumbers, 24) ){
                   return false;
                }
        }else if(keyName == "DATESTRT")
                ; //parseFitsDate(keyValue, year, month, day, refHour, refMinute, refSecond);
        else if(keyName == "DATE")
                ; // Date that metafits was created; ignored.
        else if(keyName == "VERSION")
                metaDataVersion = keyValue.c_str();
        else if(keyName == "MWAVER"){
                if( !parseFitsString(keyValue.c_str(),mwaPyVersion) ){
                   return false;
                }
        }else if(keyName == "MWADATE"){
                if( !parseFitsString(keyValue.c_str(),mwaPyDate) ){
                   return false;
                }
        }else if(keyName == "TELESCOP")
                ; // Ignore; will always be set to 'MWA'
        else if( keyName == "EXPOSURE" || keyName == "MJD" || keyName == "LST" || keyName == "HA" || keyName == "AZIMUTH" || keyName == "ALTITUDE" || keyName == "SUN-DIST" || keyName == "MOONDIST" || 
                 keyName == "JUP-DIST" || keyName == "GRIDNUM" || keyName == "RECVRS" || keyName == "CHANNELS" || keyName == "SUN-ALT" || keyName == "TILEFLAG" || keyName == "NAV_FREQ" || 
                 keyName == "FINECHAN" || keyName == "TIMEOFF" )
                ; // Ignore these fields, they can be derived from others.
        else{
                printf("Ignored keyword: %s\n",keyName.c_str());
                return false;
        }

        return true;
}

bool CObsMetadata::parseFitsString(const char* valueStr, std::string& outString )
{
        if(valueStr[0] != '\''){
           printf("ERROR: could not parse string %s\n",valueStr);
           return false;
        }
        
        std::string value(valueStr+1);
        if((*value.rbegin())!='\''){
           printf("ERROR : could not parse string : %s\n",valueStr);
           return false;
        }
        int s = value.size() - 1;
        while(s > 0 && value[s-1]==' ') --s;
        
        outString = value.substr(0, s);
        
        return true;
}

std::string CObsMetadata::stripBand(const std::string& input)
{
        if(!input.empty())
        {
                size_t pos = input.size()-1;
                while(pos>0 && isDigit(input[pos]))
                {
                        --pos;
                }
                if(pos > 0 && input[pos] == '_')
                {
                        int band = atoi(&input[pos+1]);
                        if(band > 0 && band <= 256)
                        {
                                return input.substr(0, pos);
                        }
                }
        }
        return input;
}

bool CObsMetadata::parseFitsDate(const char* valueStr, int& year, int& month, int& day, int& hour, int& min, double& sec)
{
        std::string dateStr;
        if( !parseFitsString(valueStr,dateStr) ){
           return false;
        }
        if(dateStr.size() != 19){
           //      throw std::runtime_error("Error parsing fits date");
           printf("ERROR : when parsing FITS date %s\n",valueStr);
           return false;
        }
        year = (dateStr[0]-'0')*1000 + (dateStr[1]-'0')*100 +
                (dateStr[2]-'0')*10 + (dateStr[3]-'0');
        month = (dateStr[5]-'0')*10 + (dateStr[6]-'0');
        day = (dateStr[8]-'0')*10 + (dateStr[9]-'0');

        hour = (dateStr[11]-'0')*10 + (dateStr[12]-'0');
        min = (dateStr[14]-'0')*10 + (dateStr[15]-'0');
        sec = (dateStr[17]-'0')*10 + (dateStr[18]-'0');
        
        return true;
}

double CObsMetadata::parseFitsDateToMJD(const char* valueStr)
{
        int year, month, day, hour, min;
        double sec;
        parseFitsDate(valueStr, year, month, day, hour, min, sec);
//        return Geometry::GetMJD(year, month, day, hour, min, sec);

        return ::date2jd( year, month, day, hour, min, sec );
}

bool CObsMetadata::parseIntArray(const char* valueStr, int* values, size_t count)
{
        printf("DEBUG : valueStr = %s , count = %d\n",valueStr,int(count));
        std::string str;
        if( !parseFitsString(valueStr,str) ){
           return false;
        }
        size_t pos = 0;
        for(size_t i=0; i!=count-1; ++i)
        {
                size_t next = str.find(',', pos);
                if(next == str.npos){
                        printf("DEBUG : next = %d , pos =  %d\n",int(next),int(str.npos));
                        printf("ERROR : parsing integer list %s in metafits file\n",valueStr);
                        return false;
                }
                *values = atoi(str.substr(pos, next-pos).c_str());
                ++values;
                pos = next+1;
        }
        *values = atoi(str.substr(pos).c_str());
        
        return true;
}

bool CObsMetadata::parseBool(const char* valueStr)
{
        if(valueStr[0] != 'T' && valueStr[0] != 'F')
                throw std::runtime_error("Error parsing boolean in fits file");
        return valueStr[0]=='T';
}
