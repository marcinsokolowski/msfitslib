#include "bg_fits.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <myfile.h>
#include <myfits.h>
#include "bg_globals.h"
#include "bg_array.h"
#include "bg_bedlam.h"
#include "bg_units.h"

int CBgFits::gFitsUnixTimeError=0;
const int CBgFits::m_TypicalBighornsChannels=4096;
const int CBgFits::m_TypicalBighornsYSize=200;
string CBgFits::gInAOFlaggerDir;

const char* cIntRange::GetIntTypeDesc(eIntType inttype)
{
   if( inttype == eIntTypeANT ){
      return "ANT";      
   }
   
   return "REF";
}

eIntType cIntRange::ParseIntType(const char* szIntType)
{
   if( strcmp(szIntType,"ANT") == 0 ){
      return eIntTypeANT;
   }

   if( strcmp(szIntType,"REF") == 0 || strcmp(szIntType,"TERM") == 0 || strcmp(szIntType,"NSOFF") == 0 ){
      return eIntTypeREF;
   }

   if( strcmp(szIntType,"NSON") == 0 ){
      return eIntTypeNSON;
   }
      
   return eIntTypeUndefined;
}

int cIntRange::IsInRange( int integr )
{
   if( start_int<=integr && integr<=end_int ){
      return 1;
   }
   
   return 0;   
}

double CBgFits::ch2freq(int ch)
{
   double ch_res = (stop_freq-start_freq)/(m_SizeX-1);   // 2013-06-10 : m_Size -> (m_Size-1) due to s11 csv -> fits file convertion (data from ZVL 10-500 MHz 201 bins -> delta_freq=2.45 MHz etc , etc to agree I had to change m_Size -> (m_Size-1)
                                                         // After discussion with Randall Discret FFT  : delta_freq = 480/4096 FULL STOP , 
                                                         // CENTER OF BINS : 1st channel 0 MHz (DC - term), last channel 4095 * (480/4096) = 479.8828125 MHz the bin ends at 479.8828125 + (1/2)*(480/4096) = 479.94140625 (half bin is missing)
                                                         // may also have to do with aliasing etc see : http://en.wikipedia.org/wiki/Discrete_Fourier_transform
                                                         
   return start_freq + ch*ch_res;
}

int CBgFits::freq2ch(double freq)
{
   int ch = ( (freq-start_freq)/(stop_freq-start_freq) ) * m_SizeX;
   return ch;
}

CBgFits::CBgFits( const char* fits_file, int xSize, int ySize )
 : data(NULL),m_SizeX(xSize),m_SizeY(ySize),bitpix(-32),inttime(0), start_freq(0), stop_freq(480), m_fptr(NULL), total_counter(0),m_lines_counter(0),delta_freq(480.00/4096.00), m_pRFIMask(NULL), 
   image_type(TFLOAT), dtime_fs(0), dtime_fu(0), m_bExternalData(false)
{
  if( fits_file && strlen(fits_file) ){   
     m_FileName = fits_file;
  }
  if( xSize>0 && ySize>0 ){
     Realloc( xSize, ySize, FALSE );
  }
}

CBgFits::CBgFits( int xSize, int ySize )
: data(NULL),m_SizeX(xSize),m_SizeY(ySize),bitpix(-32),inttime(0), start_freq(0), stop_freq(480), m_fptr(NULL), total_counter(0),m_lines_counter(0),delta_freq(480.00/4096.00), m_pRFIMask(NULL), image_type(TFLOAT), dtime_fs(0), dtime_fu(0), m_bExternalData(false)
{
   int size = m_SizeX*m_SizeY;
   Realloc( xSize, ySize, FALSE );   
}

CBgFits& CBgFits::operator=(const CBgFits& right)
{
   if( GetXSize() != right.GetXSize() || GetYSize() != right.GetYSize() ){
      printf("WARNING : size of the left is (%d,%d) != right (%d,%d) -> realloc called\n",GetXSize(),GetYSize(),right.GetXSize(),right.GetYSize());
      Realloc( right.GetXSize(), right.GetYSize() );
   }
   
   int size=right.GetXSize()*right.GetYSize();
   memcpy( data, right.data, size*sizeof(BG_FITS_DATA_TYPE));
   
   return (*this);
}

void CBgFits::Realloc( int sizeX, int sizeY, int bKeepOldData, bool bInitToZeros /*=false*/ )
{   
   if( sizeX>0 && sizeY>0 ){
      float* new_data = NULL;
      long int size = ((long int)(sizeX))*((long int)(sizeY));
      if( gBGPrintfLevel >= BG_INFO_LEVEL ){
         printf("CBgFits::Realloc : allocated array of size = %ld floats = %ld bytes = %.2f GB\n",(long int)size,size*sizeof(float),(float(size)*sizeof(float))/(1024*1024*1024));
      }
      try{ 
         new_data = new float[size];
      }catch(...){
         printf("ERROR : exception caught when trying to allocate array of size = %ld floats = %ld bytes = %.2f GB\n",(long int)size,size*sizeof(float),(float(size)*sizeof(float))/(1024*1024*1024));
         exit(-1);
      }
      if( data ){
         if( bKeepOldData ){            
            memcpy(new_data,data,m_SizeX*m_SizeY*sizeof(float));
         }
         delete [] data;
      }
      data = new_data;   
      m_SizeX = sizeX;
      m_SizeY = sizeY;         
      
      if( bInitToZeros ){
         memset(data,'\0',sizeof(float)*size);
      }
   }
}

void CBgFits::SetData( BG_FITS_DATA_TYPE* ptr ){ 
   if( data && !m_bExternalData ){
      // remove existing data, otherwise memory leak :
      // but remove only if NOT EXTERNAL DATA YET !!!
      // !!! WARNING : if there are problems with this line (segmentation fault or simialr crash) -> just comment it out !!!
      delete [] data;
      // !!! WARNING : if there are problems with this line (segmentation fault or simialr crash) -> just comment it out !!!
   }

   data = ptr; 
   m_bExternalData=true; 
}

void CBgFits::SetData( int xSize, int ySize, BG_FITS_DATA_TYPE* ptr )
{
   SetData( ptr );
   m_SizeX = xSize;
   m_SizeY = ySize;
}


void CBgFits::Clean()
{
  if( !m_bExternalData ){
     if( data ){
        delete [] data;
        data = NULL;
     }   
  }
}

CBgFits::~CBgFits()
{
  Clean();
  
  if( m_pRFIMask ){
     delete m_pRFIMask;
  }
}

int CBgFits::Create( const char* fits_file )
{
   int status = 0;
 
   if( fits_file && strlen(fits_file) ){
      long naxes[2] = { m_SizeX, m_SizeY };   /* image is 300 pixels wide by 200 rows */
      long naxis    = 2;
      long fpixel   = 1; /* first pixel to write      */
      
/*      char szRmCmd[1024];
      sprintf(szRmCmd,"rm -f %s",fits_file);
      int ret = system(szRmCmd);
      if( ret < 0 ){
          printf("system(%s) returned %d, due to error = %s\n",szRmCmd,ret,strerror(errno));
      }*/

      string szFitsFileToOverwrite; // with ! mark added to overwrite if file exists (otherwise ERROR occures), see http://www.aip.de/~weber/doc/fitsio/cfitsiohtml/node62.html
      szFitsFileToOverwrite = "!";
      szFitsFileToOverwrite += fits_file;
      if (fits_create_file(&m_fptr, szFitsFileToOverwrite.c_str(), &status)){ /* create new FITS file */
         if (status) fits_report_error(stderr, status);            
         return( status );
      }
                
      /* Write the required keywords for the primary array image */
      if ( fits_create_img(m_fptr,  bitpix, naxis, naxes, &status) ){
         if (status) fits_report_error(stderr, status);
         return( status );
      }
      
      total_counter = 0;         
      m_lines_counter = 0;
      return status;    
   }
   
   return -1;
}

int CBgFits::DumpFitsLine( float* buffer, int size )
{
   int status=0;

   if( m_fptr ){
      int ret = fits_write_img(m_fptr, TFLOAT, total_counter+1, size, buffer, &status );
      if( ret ){
         if (status) fits_report_error(stderr, status);
      }
      
      total_counter += size;
      m_lines_counter++;
   }else{
      printf("ERROR IN CODE [CBgFits::DumpFitsLine] : fits file was not initialized with call of CBgFits::CreateFits(FILENAME) !!!\n");
   }

   return status;
}

void CBgFits::set_ysize( int lines_counter )
{ 
   if( lines_counter > 0 ){
      m_lines_counter = lines_counter;
   }

   if( m_lines_counter > 0 ){
      if( m_lines_counter <= m_SizeY ){
         m_SizeY = m_lines_counter;
      }else{
         printf("ERROR : could not set Y size of array to value %d because allocated y-size is only %d\n",m_lines_counter,m_SizeY);
      }
   }
}

int CBgFits::add_line( CBgArray& line )
{
   if( data && line.size()==m_SizeX ){
      if( m_lines_counter == m_SizeY ){
         Realloc( m_SizeX, 2*m_SizeY );
      }
   
      int pos = m_lines_counter*m_SizeX;
      
      for(int i=0;i<line.size();i++){
         data[pos+i] = line[i];
      }
      m_lines_counter++;
   }else{
      if( gBGPrintfLevel >= BG_INFO_LEVEL ){
         printf("Line size = %d different than %d -> ignored\n",(int)line.size(),m_SizeX);
      }
   }
   
   return m_lines_counter;

}

int CBgFits::add_line( float* buffer, int size )
{
   if( buffer && data && size==m_SizeX ){
      if( m_lines_counter == m_SizeY ){
         Realloc( m_SizeX, 2*m_SizeY );
      }
   
      long int pos = ((long int)m_lines_counter)*((long int)m_SizeX);
      try{
         BG_FITS_DATA_TYPE* ptr = data+pos;
         memcpy(ptr,buffer,size*sizeof(float));
      }catch(...){
         printf("ERROR : could not copy data to position %ld in the array\n",pos);
         exit(-1);
      }
      m_lines_counter++;
   }
   
   return m_lines_counter;
}

int CBgFits::Close()
{
   int status=0;
   
   if( m_fptr ){
      fits_close_file(m_fptr, &status);
//      m_fptr = NULL; // NEW 2016-09-28 - will it be a problem for other things, it was a problem when using single object CBgFits to save many FITS files 
 
      if (status) fits_report_error(stderr, status);
         return( status );         
   }else{
      printf("ERROR IN CODE [CBgFits::Close] : fits file was not initialized with call of CBgFits::CreateFits(FILENAME) !!!\n");
   }

   return status;
}

int CBgFits::WriteKeys()
{
   int status=0;

   SetKeyValues(); // set values like inttime etc to strings keyword values 
   for(int k=0;k<_fitsHeaderRecords.size();k++){
         HeaderRecord& rec = _fitsHeaderRecords[k];
       
         const char* key = rec.Keyword.c_str();
         const char* value = rec.Value.c_str();
         const char* comment = rec.Comment.c_str();
         
         if( strcmp(key,"SIMPLE")==0 || strcmp(key,"BITPIX")==0 ||  strcmp(key,"NAXIS")==0 ||  strcmp(key,"NAXIS1")==0 || strcmp(key,"NAXIS2")==0 || strcmp(key,"EXTEND")==0 ){
            continue;
         }

         if( (rec.keytype == 'L') || (rec.keytype ==  'I') ){
            long tmp_val = atol(value);
            if ( fits_write_key(m_fptr, TLONG, key, (void*)&tmp_val, comment, &status) ){
               printf("ERROR : could not write key header %s , value = %s , comment = %s\n",key,value,comment);
            }
         }
         if( rec.keytype == 'F' ){
            float tmp_val = atof(value);
            if ( fits_write_key(m_fptr, TFLOAT, key, (void*)&tmp_val, comment, &status) ){
               printf("ERROR : could not write key header %s , value = %s , comment = %s\n",key,value,comment);
            }
         }
         if( rec.keytype == 'C' ){
            if ( fits_write_key(m_fptr, TSTRING, key, (void*)value, comment, &status) ){
               printf("ERROR : could not write key header %s , value = %s , comment = %s\n",key,value,comment);
            }
         }
   }
      
   return status;
}

int CBgFits::WriteKeys( fitsfile* fptr )
{
   int status=0;

   SetKeyValues(); // set values like inttime etc to strings keyword values 
   for(int k=0;k<_fitsHeaderRecords.size();k++){
         HeaderRecord& rec = _fitsHeaderRecords[k];
       
         const char* key = rec.Keyword.c_str();
         const char* value = rec.Value.c_str();
         const char* comment = rec.Comment.c_str();
         
         if( strcmp(key,"SIMPLE")==0 || strcmp(key,"BITPIX")==0 ||  strcmp(key,"NAXIS")==0 ||  strcmp(key,"NAXIS1")==0 || strcmp(key,"NAXIS2")==0 || strcmp(key,"EXTEND")==0 ){
            continue;
         }

         if( (rec.keytype == 'L') || (rec.keytype ==  'I') ){
            long tmp_val = atol(value);
            if ( fits_write_key(fptr, TLONG, key, (void*)&tmp_val, comment, &status) ){
               printf("ERROR : could not write key header %s , value = %s , comment = %s\n",key,value,comment);
            }
         }
         if( rec.keytype == 'F' ){
            float tmp_val = atof(value);
            if ( fits_write_key(fptr, TFLOAT, key, (void*)&tmp_val, comment, &status) ){
               printf("ERROR : could not write key header %s , value = %s , comment = %s\n",key,value,comment);
            }
         }
         if( rec.keytype == 'C' ){
            if ( fits_write_key(fptr, TSTRING, key, (void*)value, comment, &status) ){
               printf("ERROR : could not write key header %s , value = %s , comment = %s\n",key,value,comment);
            }
         }
   }
      
   return status;
}



int CBgFits::WriteFits( const char* fits_file, int bUpdateSizeY, int bWriteKeys )
{
   int status = 0;

   if( !fits_file || strlen(fits_file) == 0 ){
      fits_file = m_FileName.c_str();
   }
       
   if( fits_file && strlen(fits_file) ){
      MyFile::CreateDir(fits_file);
   
      if( bUpdateSizeY ){
         if( m_lines_counter ){
            m_SizeY = m_lines_counter;
         }
      }
   
      fitsfile *fptr=m_fptr;
      long naxes[2] = { m_SizeX, m_SizeY };   /* image is 300 pixels wide by 200 rows */
      long naxis    = 2;
      long fpixel   = 1; /* first pixel to write      */

//      if( bCreate > 0 ){
      if( !fptr ){
/*         char szRmCmd[1024];
         sprintf(szRmCmd,"rm -f %s",fits_file);
         int ret = system(szRmCmd);                                           
         if( ret < 0 ){
            printf("system(%s) returned %d, due to error = %s\n",szRmCmd,ret,strerror(errno));
         }*/
      
         string szFitsFileToOverwrite; // with ! mark added to overwrite if file exists (otherwise ERROR occures), see http://www.aip.de/~weber/doc/fitsio/cfitsiohtml/node62.html
         szFitsFileToOverwrite = "!";
         szFitsFileToOverwrite += fits_file;
                                 
         if (fits_create_file(&m_fptr, szFitsFileToOverwrite.c_str(), &status)){ /* create new FITS file */
            if (status) fits_report_error(stderr, status);
            return( status );
         }
         fptr = m_fptr;
      }
//      }
                
      /* Write the required keywords for the primary array image */      
      if ( fits_create_img(fptr,  bitpix, naxis, naxes, &status) ){
         if (status) fits_report_error(stderr, status);
         return( status );
      }
                                                                                              
      long int nelements = naxes[0] * naxes[1];          /* number of pixels to write */
                                                                     
      /* Write the array of long integers (after converting them to short) */
      if ( fits_write_img(fptr, TFLOAT, fpixel, nelements, data, &status) ){
         printf("ERROR : could not write output FITS files of size %ld elements\n",nelements);
         return( status );
      }
         
      if( bWriteKeys > 0 ) {   
          SetKeyValues(); // set values like inttime etc to strings keyword values 
          WriteKeys(); // write fits keys 
      }

/*      for(int k=0;k<_fitsHeaderRecords.size();k++){
         HeaderRecord& rec = _fitsHeaderRecords[k];
       
         const char* key = rec.Keyword.c_str();
         const char* value = rec.Value.c_str();
         const char* comment = rec.Comment.c_str();
         
         if( strcmp(key,"SIMPLE")==0 || strcmp(key,"BITPIX")==0 ||  strcmp(key,"NAXIS")==0 ||  strcmp(key,"NAXIS1")==0 || strcmp(key,"NAXIS2")==0 || strcmp(key,"EXTEND")==0 ){
            continue;
         }

         if( (rec.keytype == 'L') || (rec.keytype ==  'I') ){
            long tmp_val = atol(value);
            if ( fits_write_key(fptr, TLONG, key, (void*)&tmp_val, comment, &status) ){
               printf("ERROR : could not write key header %s , value = %s , comment = %s\n",key,value,comment);
            }
         }
         if( rec.keytype == 'F' ){
            float tmp_val = atof(value);
            if ( fits_write_key(fptr, TFLOAT, key, (void*)&tmp_val, comment, &status) ){
               printf("ERROR : could not write key header %s , value = %s , comment = %s\n",key,value,comment);
            }
         }
         if( rec.keytype == 'C' ){
            if ( fits_write_key(fptr, TSTRING, key, (void*)value, comment, &status) ){
               printf("ERROR : could not write key header %s , value = %s , comment = %s\n",key,value,comment);
            }
         }
      }*/ 
                                                                                     
      fits_close_file(fptr, &status);            /* close the file */      
      fptr = NULL;
//      m_fptr = NULL; // NEW 2016-09-28 - will it be a problem for other things, it was a problem when using single object CBgFits to save many FITS files
      m_fptr = NULL; // 2018-12-31 - if commented crashes when using the same object to save FITS files with different names as 
                     // it uses fptr=m_fptr which points to an already closed FITS file ...
      return( status );
   }
   
   return status;      
}


int CBgFits::UpdateImage(  const char* fits_file, const char* out_file )
{
   int status = 0;

   if( !fits_file || strlen(fits_file) == 0 ){
      fits_file = m_FileName.c_str();
   }
       
       
   fitsfile* infptr;
   fitsfile* outfptr;
   
   /* Open the input file */
   if ( !fits_open_file(&infptr, fits_file, READONLY, &status) )
   {
      /* Create the output file */
      
      string szFitsFileToOverwrite; // with ! mark added to overwrite if file exists (otherwise ERROR occures), see http://www.aip.de/~weber/doc/fitsio/cfitsiohtml/node62.html
      szFitsFileToOverwrite = "!";
      szFitsFileToOverwrite += out_file;
      
      if ( !fits_create_file(&outfptr, szFitsFileToOverwrite.c_str(), &status) )
      {

        /* copy the previous, current, and following HDUs */
        // fits_copy_file(infptr, outfptr, 1, 1, 1, &status);
        fits_copy_header( infptr, outfptr, &status );        
        fits_close_file(infptr, &status);
 
        long naxes[2] = { m_SizeX, m_SizeY };   /* image is 300 pixels wide by 200 rows */
        long naxis    = 2;
        long fpixel   = 1; /* first pixel to write      */
 
        /* Write the required keywords for the primary array image */
        /*if ( fits_create_img(outfptr,  bitpix, naxis, naxes, &status) ){
           if (status) fits_report_error(stderr, status);
           return( status );
        }*/
       
        int nelements = naxes[0] * naxes[1];          /* number of pixels to write */
        /* Write the array of long integers (after converting them to short) */
        if ( fits_write_img( outfptr, TFLOAT, fpixel, nelements, data, &status) )
           return( status );


        fits_close_file(outfptr,  &status);
      }
    }
   
   
   
   return status;
}


cIntRange* CBgFits::GetRange(int idx, eIntType inttype)
{
   int passed_idx=0;

   for(int i=0;i<m_IntegrationRanges.size();i++){
      cIntRange& intrange = m_IntegrationRanges[i];
      
      if( intrange.inttype == inttype ){
         if( passed_idx == idx ){
            return &intrange;
         }
         passed_idx++;
      }
   }

   return NULL;
}


int CBgFits::ParseStates( const char* szStatesList )
{
   vector<string> states_list;
   char szTmpList[128];
   strcpy(szTmpList,szStatesList);
   ParseCommaList( szTmpList , states_list );
   
   if( gBGPrintfLevel >= BG_INFO_LEVEL ){
      printf("Parsed states = ");
      for(int i=0;i<states_list.size();i++){
         printf("%s,",states_list[i].c_str());
      }
      printf("\n");
   }

/* Previous version, with states in order not agreeing with FITS header (first TERM, then ANT keywords) 
   not working well in calib_bg_data program, so I changed it to be exactly like in fits header to be in 
   the same order :

   if( states_list.size() ){
      for(int s=0;s<states_list.size();s++){
         string& state = states_list[s];
      
         // m_IntegrationRanges      
         for(int i=0;i<_fitsHeaderRecords.size();i++){
            HeaderRecord& key = _fitsHeaderRecords[i];
            
            if( strstr(key.Keyword.c_str(),state.c_str()) ){
               // picking up keywords for given state :
               cIntRange int_range;
               if( sscanf(key.Value.c_str(),"%d - %d",&(int_range.start_int),&(int_range.end_int)) == 2 ){
                  int_range.start_int--; // to shift to C/C++ indexing from 0 ( in FITS file it is from 1 ) !
                  int_range.end_int--;   // to shift to C/C++ indexing from 0 ( in FITS file it is from 1 ) !
                  int_range.inttype = s;
                  
                  // special flagging for ANT and REF :
                  if( strcmp(state.c_str(),"ANT") == 0 ){
                     int_range.inttype = eIntTypeANT;
                  }
                  if( strcmp(state.c_str(),"REF") == 0 || strcmp(state.c_str(),"TERM") == 0 ){
                     int_range.inttype = eIntTypeREF;
                  }
                  
                  int_range.m_szName = state.c_str();
                  
                  const char* szIdx = key.Keyword.c_str() + strlen(state.c_str());
                  if( szIdx && strlen(szIdx) && atol(szIdx)>=0 ){
                     int_range.StateIdx = atol(szIdx);
                  }
                  
                  m_IntegrationRanges.push_back(int_range);
               }else{
                  printf("WARNING : could not parse correctly %s integration range keyword %s = %s\n",state.c_str(),key.Keyword.c_str(),key.Value.c_str());
               }
            }
         }
      }
   }*/


// better to have them in FITS header order (at least required for calib_bg_data program ), changed on 2013-04-23 
   if( states_list.size() ){
      m_IntegrationRanges.clear(); // added 2016-04-11 so that every new file have ANT,REF keywords cleared
      for(int i=0;i<_fitsHeaderRecords.size();i++){
         HeaderRecord& key = _fitsHeaderRecords[i];
         if( gBGPrintfLevel >= BG_INFO_LEVEL ){
            printf("DEBUG : %s = %s\n",key.Keyword.c_str(),key.Value.c_str());
         }

         int state_idx=-1;
         for(int s=0;s<states_list.size();s++){
//            if( strstr(key.Keyword.c_str(),states_list[s].c_str()) ){ // due to HISENS vs NS when analysing Morgan's test data 20140128
              if( strncmp(key.Keyword.c_str(),states_list[s].c_str(),strlen(states_list[s].c_str()))==0 ){
               state_idx=s;
               break;
            }            
         }
            
         if( state_idx >= 0 ){
            // picking up keywords for given state :
            string& state = states_list[state_idx];
            cIntRange int_range;
            if( sscanf(key.Value.c_str(),"%d - %d",&(int_range.start_int),&(int_range.end_int)) == 2 ){
               int_range.start_int--; // to shift to C/C++ indexing from 0 ( in FITS file it is from 1 ) !
               int_range.end_int--;   // to shift to C/C++ indexing from 0 ( in FITS file it is from 1 ) !


               int_range.inttype = cIntRange::ParseIntType(state.c_str());
/* OLD - before testing 3 states 
               int_range.inttype = state_idx;
                  
               // special flagging for ANT and REF :
               if( strcmp(state.c_str(),"ANT") == 0 ){
                  int_range.inttype = eIntTypeANT;
               }
               if( strcmp(state.c_str(),"REF") == 0 || strcmp(state.c_str(),"TERM") == 0 || strcmp(state.c_str(),"NSOFF") == 0 ){
                  int_range.inttype = eIntTypeREF;
               }
 */                 
               int_range.m_szName = state.c_str();
                  
               const char* szIdx = key.Keyword.c_str() + strlen(state.c_str());
               if( szIdx && strlen(szIdx) && atol(szIdx)>=0 ){
                  int_range.StateIdx = atol(szIdx);
               }
                  
               m_IntegrationRanges.push_back(int_range);
            }else{
               printf("WARNING : could not parse correctly %s integration range keyword %s = %s\n",state.c_str(),key.Keyword.c_str(),key.Value.c_str());
            }
         }
      }
   }


   return m_IntegrationRanges.size();
}

int CBgFits::ParseSkyIntegrations()
{
   int ret=0;
   
   for(int i=0;i<_fitsHeaderRecords.size();i++){
      HeaderRecord& key = _fitsHeaderRecords[i];
      
      if( strstr(key.Keyword.c_str(),"ANT") ){
         cIntRange ant_int;
         ant_int.inttype = eIntTypeANT;
         
         if( sscanf(key.Value.c_str(),"%d - %d",&(ant_int.start_int),&(ant_int.end_int)) == 2 ){            
            ant_int.start_int--; // to shift to C/C++ indexing from 0 ( in FITS file it is from 1 ) !
            ant_int.end_int--;   // to shift to C/C++ indexing from 0 ( in FITS file it is from 1 ) !                              
         
            if( ant_int.start_int > 1 ){
               // add previous REF integrations range :            
               cIntRange ref_int;
               ref_int.inttype = eIntTypeREF;            
               if( m_IntegrationRanges.size() > 0 )
               {
                  // adding reference integrations range in between :
                  ref_int.start_int = m_IntegrationRanges.back().end_int+2;
                  ref_int.end_int = ant_int.start_int-2;
               }else{
                  // first integration :
                  ref_int.start_int = 0;
                  ref_int.end_int = ant_int.start_int-2;
               }
               
               // FITS (from 1) -> C numbering (from 0)
               ref_int.start_int++;
               ref_int.end_int++;               
               m_IntegrationRanges.push_back(ref_int);
            }
         
            // FITS (from 1) -> C numbering (from 0)
            ant_int.start_int++;
            ant_int.end_int++;
            m_IntegrationRanges.push_back(ant_int);
         }else{
            printf("WARNING : could not parse correctly ANT integration range keyword %s = %s\n",key.Keyword.c_str(),key.Value.c_str());
         }
      }
   }  

   if( m_IntegrationRanges.size() > 0 && m_IntegrationRanges.back().end_int<(GetYSize()-1) ){
      // add last reference integrations :
      cIntRange ref_int;
      ref_int.start_int = m_IntegrationRanges.back().end_int+2;
      ref_int.end_int = GetYSize()-1;
      ref_int.inttype = eIntTypeREF;
      
      // FITS (from 1) -> C numbering (from 0)
      ref_int.start_int++;
      ref_int.end_int++;
      m_IntegrationRanges.push_back(ref_int);                           
   }
   
   if( gBGPrintfLevel >= BG_INFO_LEVEL && m_IntegrationRanges.size()>0 ){
      printf("INTEGRATION RANGES:\n");
      for(int i=0;i<m_IntegrationRanges.size();i++){
         cIntRange& range = m_IntegrationRanges[i];
      
         printf("\t%d - %d : %s\n",range.start_int,range.end_int,cIntRange::GetIntTypeDesc((eIntType)range.inttype));
      }
   }

   return ret; 
}

int CBgFits::ReadFits( const char* fits_file, int bAutoDetect /*=0*/, int bReadImage /* =1 */ , int bIgnoreHeaderErrors /* =0 */ , bool transposed /* =false */ )
{
  if( gBGPrintfLevel >= BG_DEBUG_LEVEL ){
     printf("DEBUG : ReadFits( %s , %d , %d , %d , %d )\n",fits_file,bAutoDetect,bReadImage,bIgnoreHeaderErrors,transposed);
  }

  // split path :
  //void mystring::splitpath(BaseString& drv,BaseString& dir,
  //                                           BaseString& fname,BaseString& ext) const
  mystring szFullPathFits = fits_file;
  mystring fits_drv,fits_dir,fits_fname,fits_ext;
  szFullPathFits.splitpath( fits_drv, fits_dir, fits_fname, fits_ext );

  fitsfile *fp=0;
  int status = 0;
  string szFreqKeyword = "CRVAL1", szFreqDelta = "CDELT1", szTimeDelta = "CDELT2";
  if( transposed ){
     szFreqKeyword = "CRVAL2";
     szFreqDelta = "CDELT2";
     szTimeDelta = "CDELT1";
  }

  if( !fits_file || strlen(fits_file) == 0 ){
     fits_file = m_FileName.c_str();
  }else{
     m_FileName = fits_file;
  }
   
  if( fits_file && strlen(fits_file) ){
     fits_open_image(&fp, m_FileName.c_str(), READONLY, &status);
     if( status ){
        printf("ERROR : could not open FITS file %s , due to error %d\n",m_FileName.c_str(),status);        
        return status;
     }

     //Read image parameters
     int naxis=0;
     long axsizes[2];
                
     fits_get_img_param(fp, 2, &bitpix, &naxis, axsizes, &status);
     if( gBGPrintfLevel >= BG_INFO_LEVEL ){
        printf("INFO : auto-detected file format = %d bits\n",bitpix);
     }
     if( status ){ 
         printf("ERROR : could not read parameters from FITS file %s, due to error %d\n",m_FileName.c_str(),status);
         return status;
     }
     if( bAutoDetect > 0 ){
        if( bitpix == 8 ){
           printf("INFO : image type set to TBYTE=%d (was %d) , number of bits=%d\n",TBYTE,image_type,bitpix);
           image_type = TBYTE;
        }
     } 
     
     // check sizes and allocate/re-allocate :
     if( data && (axsizes[0] != m_SizeX || axsizes[1] != m_SizeY) ){        
        delete [] data;
        data = NULL;
     }     
     m_SizeX = axsizes[0];
     if( naxis > 1 ){
        m_SizeY = axsizes[1];     
     }else{
        m_SizeY = 1;
     }
     
     if( bReadImage > 0 ){
         long int sizeXY = ((long int)m_SizeX)*((long int)m_SizeY);
         if( !data ){
            if( gBGPrintfLevel >= BG_DEBUG_LEVEL ){
               printf("Allocating %d x %d = %ld image for naxis = %d \n",m_SizeX,m_SizeY,sizeXY,naxis);fflush(stdout);
            }
            data = new float[sizeXY];
         }
     
//         long firstpixel[2] = {1, 1};
// see : https://heasarc.gsfc.nasa.gov/fitsio/c/c_user/node40.html
         long* firstpixel = new long[naxis];
         for(int a=0;a<naxis;a++){
             firstpixel[a] = 1;
         }
         
//         int sizeXY = m_SizeX*m_SizeY;
         int fits_read_ret = fits_read_pix(fp, image_type, firstpixel, sizeXY, NULL, data, NULL, &status);
         if( status ){ 
             printf("ERROR : could not read data from FITS file %s, due to error %d\n",m_FileName.c_str(),status);
             return status;
         }
         printf("DEBUG : fits_read_pix returned %d\n",fits_read_ret);         
         delete [] firstpixel;
     }

     // reading keywords :
     int nkeys=0;
     fits_get_hdrspace(fp, &nkeys, NULL, &status);
   
     char keyname[1024]; // was FLEN_KEYWORD+1
     char keyvalue[1024];  // was FLEN_VALUE+1 
     char comment[1024]; // was FLEN_COMMENT+1
            
     if(status)nkeys=0;

     int bStopFreqFound=0;
     int bDeltaFreqFound=0;
     _fitsHeaderRecords.clear();         
     mystring szDATE_UT,szCDELT2;     
     for(int ikey=0; ikey<nkeys; ikey++){
       HeaderRecord rec;
                           
       fits_read_keyn(fp, ikey+1, keyname, keyvalue, comment, &status);
       
       if( strcmp(keyname,"CHANNELS") == 0 ){        
           char long_keyvalue1[1024],long_comment[1024],long_keyvalue2[1024],long_keyvalue3[1024];
           char* longstr[3];
           longstr[0] = long_keyvalue1;
           longstr[1] = long_keyvalue2;
           longstr[2] = long_keyvalue3;

           // fits_read_string_key( fp, keyname, 0, 1023, long_keyvalue, &keylen, long_comment, &status);
           // http://www.mssl.ucl.ac.uk/swift/om/sw/help/fitsio/node78.html
           fits_read_key_longstr( fp, keyname, longstr, long_comment, &status);
           // printf("CHANNELS = %s -> strlen = %d vs. %d \n",longstr[0],strlen(longstr[0]),(FLEN_VALUE+1));           
           strcpy( keyvalue, longstr[0] );           
       }
       
       
       
       if (status) break;
       
       int keylen = strlen(keyvalue);
       if( keylen ){
          fits_get_keytype( keyvalue, &rec.keytype, &status);
          
          if( rec.keytype == 'C' ){
             // removing aphostrophs from start and end (to be tested if not spoils anything !)
             const char* ptr = keyvalue;
             while( (*ptr) == '\'' ){
                ptr++;
             }
             int len=strlen(ptr);
             while(len>0 && ptr[len-1]=='\'' ){
                len--;
             }
             
             char tmpvalue[FLEN_VALUE+1];
             strncpy(tmpvalue,ptr,len);
             tmpvalue[len] = '\0';
             strcpy(keyvalue,tmpvalue);
          }
       }
                               
       rec.Keyword = keyname;
       rec.Value = keyvalue;
       rec.Comment = comment; 
       
       if( gBGPrintfLevel >= BG_DEBUG_LEVEL ){
          printf("DEBUG0 : %s = %s\n",rec.Keyword.c_str(),rec.Value.c_str());
       }
       
       if( ( strcmp(rec.Keyword.c_str(),"CTYPE2") == 0  && strcmp(rec.Value.c_str(),"Frequency")==0 ) || ( strcmp(rec.Keyword.c_str(),"CTYPE1") ==0 && strcmp(rec.Value.c_str(),"Time")==0 ) ){
          if( !transposed ){
             transposed = true;
             szFreqKeyword = "CRVAL2";
             szFreqDelta = "CDELT2";
             szTimeDelta = "CDELT1";             
             if( gBGPrintfLevel >= BG_INFO_LEVEL ){
                printf("DEBUG : auto-detected that the dynamic spectrum has time on horizontal axis and frequency on vertical (frequency vs. time)\n");
             }
          }
       }

       if( strstr(rec.Keyword.c_str(),"AVERF" ) ){
          m_AverList.push_back( rec.Value );
       }
       if( strstr(rec.Keyword.c_str(),"INTTIME" ) ){
          inttime = atof(rec.Value.c_str());
       }
       if( strstr(rec.Keyword.c_str(),"DTIME-FS" ) ){
          dtime_fs = atol(rec.Value.c_str());

          // fix Unixtime for data 20121004 - 20121105 :          
          if( gFitsUnixTimeError>0 && dtime_fs>1349049600 && dtime_fs<1352160000 ){
             // only for 201210 data 
             double time_drift=21.5;
             double dt=711+((dtime_fs-1351753823)/86400)*time_drift;
             dtime_fs = dtime_fs - dt;

             char szUxTime[64];
             sprintf(szUxTime,"%d",(int)dtime_fs);
             rec.Value = szUxTime;
          }
       }
       if( strstr(rec.Keyword.c_str(),"DATE" ) ){
          if( strcmp(rec.Keyword.c_str(),"DATE-OBS" )==0 ){
              // DATE-OBS has priority over others 
              if( strlen( szDATE_UT.c_str() ) > 0 ){
                 printf("WARNING : overwritting value in szDATE_UT = %s with DATE-OBS = %s\n",szDATE_UT.c_str(),rec.Value.c_str());
              }
              szDATE_UT = rec.Value.c_str();
          }else{
              // only overwrite if still empty to avoid overwritting DATE-OBS value !
              if( strlen(szDATE_UT.c_str()) == 0 ){                 
                 szDATE_UT = rec.Value.c_str();
              }else{
                 printf("WARNING : szDATE_UT already has a value = %s , therefore date non-priority keyword %s = %s ignored (priority keyword is DATE-OBS)\n",szDATE_UT.c_str(),rec.Keyword.c_str(),rec.Value.c_str());
              }
          }
       }
//       if( strstr(rec.Keyword.c_str(),"DATE-OBS" ) ){
          // overwrites DATE 
//          szDATE_UT = rec.Value.c_str();
//       }
       if( strstr(rec.Keyword.c_str(),"DTIME-FU" ) ){
          dtime_fu = atol(rec.Value.c_str());
       }
       if( strstr(rec.Keyword.c_str(),"NACCUM" ) ){    
          naccum = atol(rec.Value.c_str());            
       }
       
       if( strstr(rec.Keyword.c_str(),"STARTFRQ" ) || strstr(rec.Keyword.c_str(), szFreqKeyword.c_str() ) ){
//       if( strstr(rec.Keyword.c_str(),"CRVAL1" ) ){       
//          double unit = 1000000.00; // MHz in Hz 
          double unit = 1.00; // MHz 
          start_freq = atof(rec.Value.c_str())/unit; // in header in Hz
          if( gBGPrintfLevel >= BG_INFO_LEVEL ){
             printf("DEBUG : start freq = %.2f MHz\n",start_freq);
          }
       }
       if( strstr(rec.Keyword.c_str(),"STOPFRQ" ) ){
          double unit = 1.00; // MHz
          stop_freq = atof(rec.Value.c_str())/unit; // in header in Hz
          if( gBGPrintfLevel >= BG_INFO_LEVEL ){
             printf("DEBUG : stop freq = %.2f MHz\n",stop_freq);
          }
          bStopFreqFound = 1;
       }                          
       
       if( strstr(rec.Keyword.c_str(), szFreqDelta.c_str() ) ){
          bDeltaFreqFound = 1; 
          delta_freq = atof(rec.Value.c_str());
       }                         
       if( strstr(rec.Keyword.c_str(), szTimeDelta.c_str() ) ){
          szCDELT2 = rec.Value.c_str();
       }                         
                                                                 
       _fitsHeaderRecords.push_back(rec);                                                                     
       // _headerKeywordMap.insert( pair<string,int>(rec.Keyword,ikey) );
     }  
     
     if( !bStopFreqFound && bDeltaFreqFound ){
        // m_Size -> (m_SizeX-1) - 2013-06-10 due to S11 csv files -> fits (see comment in CBgFits::ch2freq)
        stop_freq = start_freq + delta_freq*(m_SizeX-1);
     }

     if( inttime <= 0 ){
        // CDELT2
        if( strlen(szCDELT2.c_str())>0 ){
           inttime = atof(szCDELT2.c_str());
        }else{
           if( bIgnoreHeaderErrors <= 0 ){
               printf("WARNING : could not read INTTIME nor CDELT2 to get time resolutions information for file %s\n",fits_file);
           }
        }
     }
     if( dtime_fs <= 0 ){
        if( strlen(szDATE_UT.c_str()) > 0 ){
           // meaning that DTIME-FS was not found - try to use format 2016-04-15T00:12:23
           struct tm _tm;
           memset( &_tm,'\0',sizeof(struct tm));       
           strptime(szDATE_UT.c_str(), "%Y-%m-%dT%H:%M:%S", &_tm);
           double unixtime = (double)timegm( &_tm );

          // add fractional if : %Y-%m-%dT%H:%M:%S.5           
           const char* p_dot = strstr(szDATE_UT.c_str(),".");
           if( p_dot ){
              double extra = atof( p_dot );
              unixtime += extra;                            
           }
           
           dtime_fs = (int)unixtime;
           dtime_fu = (unixtime-(int)unixtime)*1e6;                 
           
//           printf("%s -> %.2f -> %d / %d\n",szDATE_UT.c_str(), unixtime,dtime_fs,dtime_fu);
           
/*           int match = sscanf(szDateTime.c_str(),"%d-%d-%dT%d:%d:%d",&_tm->tm_year,&_tm->tm_mon,&_tm->tm_mday,&_tm->tm_hour,&_tm->tm_min,&_tm->tm_sec);
           if( match == 6 ){
              _tm->tm_year = (_tm->tm_year - 1900);            
               // and months from 1 :
               _tm->tm_mon--;                              
           
              double unixtime = (double)timegm( &_tm );
              dtime_fs = (int)unixtime;
              dtime_us = (unixtime-(int)unixtime)*1e6;
           }else{
              printf("ERROR : could not parse UT date/time from string %s\n",szDateTime.c_str());
           }*/
        }else{
           const char* fits_fname_ptr = fits_fname.c_str();
           if( strncmp(fits_fname_ptr,"dirty_image_",12)==0 ){
              // read UTC from FITS file name :
              char szDATE_UT[64];
              memset(szDATE_UT,'\0',64);
              strncpy(szDATE_UT,fits_fname_ptr+12,18);   
      
              printf("DEBUG : szDATE_UT = |%s|\n",szDATE_UT);
      
              struct tm _tm;
              memset( &_tm,'\0',sizeof(struct tm));
              strptime(szDATE_UT, "%Y%m%dT%H%M%S", &_tm);
              dtime_fs = (int)timegm( &_tm );
              dtime_fu = atol(szDATE_UT+15)*1000; // ms -> us 
      
              printf("DEBUG : %s (%s) -> unixtime = %d sec and %d usec\n",fits_file,fits_fname_ptr,(int)dtime_fs,dtime_fu);      
           }else{          
              if( bIgnoreHeaderErrors <= 0 ){
                  printf("WARNING : cannot get start UT time of the image %s !\n",fits_file);
              }
           }
        }
     }
     
     int sky_int_count = 0;
     HeaderRecord* szStates = GetKeyword("STATES");
     if( szStates ){
        sky_int_count = ParseStates(szStates->Value.c_str());
     }else{
        sky_int_count = ParseSkyIntegrations();
     }

     if( gBGPrintfLevel >= BG_INFO_LEVEL ){
        printf("---------------------------------------\n");                                                                                                                 
        printf("CBgFits::ReadFits %s : read %d header keys\n",fits_file,nkeys);
        printf("---------------------------------------\n");
        for(int i=0;i<_fitsHeaderRecords.size();i++){
           printf("%s = %s\n",_fitsHeaderRecords[i].Keyword.c_str(),_fitsHeaderRecords[i].Value.c_str());
        }
     }


     if( m_AverList.size() == 0 ){
        m_AverList.push_back( m_FileName.c_str() );
     }
     
     fits_close_file(fp, &status);                     
     if( status ){ 
         printf("ERROR : could not close FITS file %s, due to error %d\n",m_FileName.c_str(),status);
         return status;
     }     
  }else{
     printf("ERROR : empty fits_file name parameter passed to CBgFits::ReadFits !\n");
     return -1;
  }
  
  return status;
}

void CBgFits::SetKeyValues()
{
   char szTmp[64];
   
   if( inttime > 0 ){
      sprintf(szTmp,"%.8f",inttime);
      SetKeyword( "INTTIME", szTmp );      
   }
}

HeaderRecord* CBgFits::GetKeyword(const char *keyword)
{
   for(int i=0;i<_fitsHeaderRecords.size();i++){
      if( strcmp(_fitsHeaderRecords[i].Keyword.c_str(), keyword ) == 0 ){
         return &(_fitsHeaderRecords[i]);
      }
   }
   
   return NULL;
}

HeaderRecord* CBgFits::GetKeyword(vector<HeaderRecord>& keys_list, const char *keyword)
{
   for(int i=0;i<keys_list.size();i++){
      if( strcmp(keys_list[i].Keyword.c_str(), keyword ) == 0 ){
         return &(keys_list[i]);
      }
   }
   
   return NULL;
}



void CBgFits::SetKeywordFloat(const char *keyword, float new_value )
{
   char szNewValue[128];
   sprintf(szNewValue,"%.8f",new_value);
   SetKeyword(keyword,szNewValue,'F');
   
   if( strcmp(keyword,"STOPFRQ") == 0 ){
      stop_freq = new_value;
   }
   if( strcmp(keyword,"STARTFRQ") == 0 ){
      start_freq = new_value;
   }
//   if( strcmp(keyword,"CDELT1") == 0 ){
//       = new_value;
//   }
}

void CBgFits::SetKeyword(const char *keyword, int new_value ){
   char szNewValue[128];
   sprintf(szNewValue,"%d",new_value);
   SetKeyword(keyword,szNewValue,'L');
}

void CBgFits::SetKeyword(const char *keyword, const char* new_value, char keytype, const char* comment )
{
   if( keyword && strlen(keyword) && new_value && strlen(new_value) ){
      for(int i=0;i<_fitsHeaderRecords.size();i++){
        if( strcmp(_fitsHeaderRecords[i].Keyword.c_str(), keyword ) == 0 ){
           _fitsHeaderRecords[i].Value = new_value;
           return;
        }
      }
      
      HeaderRecord new_rec;
      new_rec.Keyword = keyword;      
      new_rec.Value = new_value;
      new_rec.keytype = keytype;
      if( comment && strlen(comment) ){
         new_rec.Comment = comment;
      }
      _fitsHeaderRecords.push_back(new_rec);
   }
}

CBgFits& CBgFits::add_squared(const CBgFits& right)
{
    if( data && right.data && m_SizeX==right.m_SizeX && m_SizeY==right.m_SizeY ){
        int sizeXY=m_SizeX*m_SizeY;
      
      for(int i=0;i<sizeXY;i++){
         data[i] = data[i] + (right.data[i] * right.data[i]);
      }

    }    
    return (*this);
}

CBgFits& CBgFits::operator+=(const CBgFits& right)
{
   if( data && right.data && m_SizeX==right.m_SizeX && m_SizeY==right.m_SizeY ){
      int sizeXY=m_SizeX*m_SizeY;
      
      for(int i=0;i<sizeXY;i++){
         data[i] = data[i] + right.data[i];
      }
   } 
   
   inttime += right.inttime;
   
   
   return (*this);
}

void CBgFits::NormalizeY()
{
   vector<double> col_avg;
   col_avg.assign( GetYSize(), 0.00 );
   
   for(int y=0;y<GetYSize();y++){
      for(int x=0;x<GetXSize();x++){
         col_avg[y] += getXY(x,y);
      }
      
      col_avg[y] = col_avg[y] / GetXSize();
      
      if( gBGPrintfLevel >= BG_DEBUG_LEVEL ){
         printf("%d %.8f\n",y,col_avg[y]);
      }
   }
   
   for(int y=0;y<GetYSize();y++){
      double norm_val = col_avg[y];
   
      for(int x=0;x<GetXSize();x++){
         double val = getXY(x,y);
         setXY( x, y, val/norm_val );
      }
   }   
   
   double mean,rms,minval,maxval;
   GetStat( mean, rms, minval, maxval );
   
   if( gBGPrintfLevel >= BG_INFO_LEVEL ){
      printf("INFO (NormalizeY) : mean = %.8f , rms = %.8f , minval = %.8f , maxval = %.8f\n",mean, rms, minval, maxval );
   }
}

void CBgFits::NormalizeX()
{
   vector<double> line_avg;
   int n_ch = GetXSize();
   int n_times = GetYSize();
   line_avg.assign( n_ch, 0.00 );
   
   for(int ch=0;ch<n_ch;ch++){
      for(int t=0;t<n_times;t++){
         line_avg[ch] += getXY(ch,t);
      }
      
      line_avg[ch] = line_avg[ch] / n_times;
      if( gBGPrintfLevel >= BG_DEBUG_LEVEL ){
         printf("%d %.8f\n",ch,line_avg[ch]);
      }
   }
   
   for(int ch=0;ch<n_ch;ch++){
      double norm_val = line_avg[ch];
   
      for(int t=0;t<n_times;t++){
         double val = getXY(ch,t);
         setXY( ch, t, val/norm_val );
      }
   }   

   double mean,rms,minval,maxval;
   GetStat( mean, rms, minval, maxval );
   
   if( gBGPrintfLevel >= BG_INFO_LEVEL ){
      printf("INFO (NormalizeX) : mean = %.8f , rms = %.8f , minval = %.8f , maxval = %.8f\n",mean, rms, minval, maxval );
   }
}



void CBgFits::Normalize(double norm_factor)
{
   if( data ){
      int sizeXY=m_SizeX*m_SizeY;
      
      for(int i=0;i<sizeXY;i++){
         data[i] = data[i] / norm_factor;
      }
   }
}

float CBgFits::valXY( int x, int y )
{
/*   int pos = y*m_SizeX + x;
   if( pos>=0 && pos < (m_SizeX*m_SizeY) ){
      return data[pos];      
   }   
   return -1;*/
   
   return valXY_auto(x,y);
}

char CBgFits::valXY_char( int x, int y )
{
   // WRONG : if( pos>=0 && pos < (m_SizeX*m_SizeY) ){
   if( x>=0 && y>=0 && x<m_SizeX && y<m_SizeY ){
      int pos = y*m_SizeX + x;
      char* data_char = (char*)data;

      return data_char[pos];      
   }
   
   // was -1 , but it does not make sense as it is perfectly valid value NaN is better    
   return (0.00/0.00); // NaN is more robust way of returning None/NULL like value, checked with isnan or fpclassify(val) == FP_NAN , test code /home/msok/bighorns/software/analysis/test/nan.cpp
}

float CBgFits::valXY_auto( int x, int y )
{
   if( image_type == TBYTE ){
      // WRONG : if( pos>=0 && pos < (m_SizeX*m_SizeY) ){ - it can allow x>m_SizeX !!!
      if( x>=0 && y>=0 && x<m_SizeX && y<m_SizeY ){
         char* data_char = (char*)data;
         int pos = y*m_SizeX + x;      
         return (float)(data_char[pos]);
      }
   }else{
      if( x>=0 && y>=0 && x<m_SizeX && y<m_SizeY ){
         int pos = y*m_SizeX + x;      
         return data[pos];      
      }
   }
      
   // was -1 , but it does not make sense as it is perfectly valid value NaN is better    
   return (0.00/0.00); // NaN is more robust way of returning None/NULL like value, checked with isnan or fpclassify(val) == FP_NAN , test code /home/msok/bighorns/software/analysis/test/nan.cpp
}

int CBgFits::setY( int y, float value )
{
   int ret=0;
   for(int x=0;x<m_SizeX;x++){
      setXY(x,y,value);
      ret++;
   }
   
   return ret;
}

float CBgFits::addXY( int x, int y, float value )
{
   // WRONG : if( pos>=0 && pos < (m_SizeX*m_SizeY) ){ - it can allow x>m_SizeX !!!
   if( x>=0 && y>=0 && x<m_SizeX && y<m_SizeY ){
      int pos = y*m_SizeX + x;
      data[pos] += value;
      return data[pos];
   }

   // was -1 , but it does not make sense as it is perfectly valid value NaN is better    
   return (0.00/0.00); // NaN is more robust way of returning None/NULL like value, checked with isnan or fpclassify(val) == FP_NAN , test code /home/msok/bighorns/software/analysis/test/nan.cpp
}

float CBgFits::setXY( int x, int y, float value )
{
   // WRONG : if( pos>=0 && pos < (m_SizeX*m_SizeY) ){  - it can allow x>m_SizeX !!!
   if( x>=0 && y>=0 && x<m_SizeX && y<m_SizeY ){
      int pos = y*m_SizeX + x;
      data[pos] = value;
      return data[pos];
   }

   // was -1 , but it does not make sense as it is perfectly valid value NaN is better    
   return (0.00/0.00); // NaN is more robust way of returning None/NULL like value, checked with isnan or fpclassify(val) == FP_NAN , test code /home/msok/bighorns/software/analysis/test/nan.cpp
}

float* CBgFits::set_line( int y, float* buffer )
{
   int pos = y*m_SizeX;
   if( buffer ){
      if( y < m_SizeY ){
         memcpy(&(data[pos]),buffer,m_SizeX*sizeof(float));
      }else{
         printf("ERROR : trying to write to line y=%d, file size is %d !\n",y,m_SizeY);
         return NULL;
      }
   }

   return &(data[pos]);
}

float* CBgFits::set_line( int y, CBgArray& line )
{
   int pos = y*m_SizeX;
   
   if( m_SizeX != line.size() ){
      printf("ERROR : cannot set line, array size = %d != image X size = %d\n",(int)line.size(),m_SizeX);
      return NULL;
   }
   
   for(int x=0;x<m_SizeX;x++){
      data[pos+x] = line[x];
   }
   
   return &(data[pos]);
}

float* CBgFits::set_line( int y, vector<cValue>& line )
{
   int pos = y*m_SizeX;
   
   if( m_SizeX != line.size() ){
      printf("ERROR : cannot set line, array size = %d != image X size = %d\n",(int)line.size(),m_SizeX);
      return NULL;
   }
   
   for(int x=0;x<m_SizeX;x++){
      data[pos+x] = line[x].y;
   }
   
   return &(data[pos]);
}

float* CBgFits::set_line( int y, vector<double>& line )
{
   int pos = y*m_SizeX;
   
   if( m_SizeX != line.size() ){
      printf("ERROR : cannot set line, array size = %d != image X size = %d\n",(int)line.size(),m_SizeX);
      return NULL;
   }
   
   for(int x=0;x<m_SizeX;x++){
      data[pos+x] = line[x];
   }
   
   return &(data[pos]);

}

float* CBgFits::set_reim_line( int y, vector<double>& line_re, vector<double>& line_im )
{
   int pos = y*m_SizeX;
   
   if( m_SizeX != (line_re.size()*2) ){
      printf("ERROR : cannot set line, array size = %d != image X size = %d\n",(int)(line_re.size()*2),m_SizeX);
      return NULL;
   }
   
   if( line_re.size() != line_im.size() ){
      printf("ERROR : different number of RE and IM values (%d != %d)\n",(int)(line_re.size()),(int)(line_im.size()));
      return NULL;
   }   
   
   for(int x=0;x<line_re.size();x++){
      // data[pos+x] = line[x];
      data[pos + x*2] = line_re[x];
      data[pos + x*2 + 1] = line_im[x];     
   }
   
   return &(data[pos]);
     
}

float CBgFits::value( int y, int x )
{
   // WRONG : if( pos>=0 && pos < (m_SizeX*m_SizeY) ){
   if( x>=0 && y>=0 && x<m_SizeX && y<m_SizeY ){
      int pos = y*m_SizeX + x;
      return data[pos];      
   }
   
   // was -1 , but it does not make sense as it is perfectly valid value NaN is better    
   return (0.00/0.00); // NaN is more robust way of returning None/NULL like value, checked with isnan or fpclassify(val) == FP_NAN , test code /home/msok/bighorns/software/analysis/test/nan.cpp
}

float* CBgFits::get_line( int y )
{
   if( y<0 || y >= m_SizeY ){
      printf("ERROR : requested line %d >= size = %d\n",y,m_SizeY);
      return NULL;
   }

   int pos = y*m_SizeX;         
   return &(data[pos]);
}

float* CBgFits::get_line( int y, CBgArray& buffer )
{
   buffer.alloc( GetXSize() , 0 );
   for(int x=0;x<GetXSize();x++){
      buffer[x] = getXY(x,y);
   }
   
   return get_line(y);
}

float* CBgFits::get_line( int y, float* buffer ){
   if( y<0 || y >= m_SizeY ){
      printf("ERROR : requested line %d >= size = %d\n",y,m_SizeY);
      return NULL;
   }

   int pos = y*m_SizeX;
   if( buffer ){
      if( image_type == TBYTE ){
         for(int x=0;x<GetXSize();x++){
            buffer[x] = getXY(x,y);
         }
      }else{
         memcpy(buffer,&(data[pos]),m_SizeX*sizeof(float));
      }
   }
   
   return buffer;
}

double CBgFits::GetUnixTime()
{
/*   double ret=0.00;
   HeaderRecord* pHdrSEC = GetKeyword("DTIME-FS");
   if( pHdrSEC ){
      ret = atol(pHdrSEC->Value.c_str());
      HeaderRecord* pHdrUSEC = GetKeyword("DTIME-FU");
      if( pHdrUSEC ){
         ret = ret + ((double)atol(pHdrUSEC->Value.c_str())) / (1000000.00);
      }
   }*/
   
   double ret = dtime_fs + dtime_fu/1000000.00;   
   return ret;
}

double CBgFits::GetUnixTime(int y)
{
   double ret = GetUnixTime() + y*inttime;
   return ret;
}

void CBgFits::Multiply( CBgFits& right )
{
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      data[i] = (data[i] * right.data[i]);
   }
}

double CBgFits::Sum(int y)
{
   double sum = 0.00;
   
   for(int x=0;x<m_SizeX;x++){
      int pos = y*m_SizeX + x;
      
      sum += data[pos];
   }
   
   return sum;
}

double CBgFits::Sum()
{
   double sum = 0.00;
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      sum += data[i];
   }
   
   return sum;
}

void CBgFits::AddImages( CBgFits& right, double mult_const )
{
   int size = m_SizeX*m_SizeY;
   
   double val = getXY(82,101);
   
   for(int i=0;i<size;i++){
      data[i] = (data[i] + right.data[i]) * mult_const;
   }
   
   if( gBGPrintfLevel >= BG_DEBUG_LEVEL ){
      printf("DEBUG : (%.4f + %.4f)/2 = %.4f\n",val,right.getXY(82,101),getXY(82,101));
   }   
}

void CBgFits::SEFD_XX_YY( CBgFits& right )
{
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      double xx_val = data[i];
      double yy_val = right.data[i];
      
      double out_val = 0.5*sqrt( xx_val*xx_val + yy_val*yy_val );
      data[i] = out_val;       
   }
}

void CBgFits::SEFD2AOT()
{
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      double sefd = data[i];
      
      double aot = (2.00*1380.00)/sefd;
      data[i] = aot;       
   }
}


void CBgFits::Subtract( CBgFits& right, CBgFits& diff )
{
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      diff.data[i] = (data[i] - right.data[i]);
   }
}

void CBgFits::Subtract( CBgFits& right )
{
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      data[i] = (data[i] - right.data[i]);
   }
}

void CBgFits::ComplexMag( CBgFits& right )
{
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      data[i] = (data[i]*data[i] + right.data[i]*right.data[i]);
   }
}

int CBgFits::SubtractColumn( CBgArray& column )
{
   if( m_SizeY != column.size() ){
      printf("ERROR : cannot subtract column of different size %d != %d\n",m_SizeY,(int)column.size());
      return 0;
   }
   
   for(int y=0;y<m_SizeY;y++){
      double y_value = column[y];
      
      for(int x=0;x<m_SizeX;x++){
         double new_val = getXY(x,y) - y_value;
         setXY(x,y,new_val);
      }
   }
   
   return m_SizeY;
}

int CBgFits::SubtractSpectrum( CBgArray& spectrum )
{
   if( m_SizeX != spectrum.size() ){
      printf("ERROR : cannot subtract spectrum of different size %d != %d\n",m_SizeX,(int)spectrum.size());
      return 0;
   }
   
   for(int y=0;y<m_SizeY;y++){
      for(int x=0;x<m_SizeX;x++){
         double new_val = getXY(x,y) - spectrum[x];
         setXY(x,y,new_val);
      }
   }
   
   return m_SizeX;
}

int CBgFits::DivideBySpectrum( vector<cValue>& spectrum )
{
   int bInterpol=0;
   if( m_SizeX != spectrum.size() ){
      printf("WARNING in CBgFits::DivideBySpectrum : sizes of FITS file and spectrum different %d != %d\n",m_SizeX,(int)spectrum.size());
      bInterpol=1;
   }

   for(int y=0;y<m_SizeY;y++){
      for(int x=0;x<m_SizeX;x++){
         double fits_val = getXY(x,y);
         double spec_val=0;
         if( bInterpol > 0 ){
            double freq = ch2freq(x);
            spec_val = interpolate(spectrum,freq);
         }else{
            spec_val = spectrum[x].y;
         }
         
         double new_val = fits_val / spec_val;
         setXY(x,y,new_val);
      }
   }   
   
   return m_SizeX;
}



void CBgFits::RMS( int n_count, CBgFits& right )
{
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      data[i] = sqrt( (data[i] / n_count ) - (right.data[i]*right.data[i]) );      
   
//      data[i] = (data[i] / right.data[i]);
   }
}

void CBgFits::HorFlip()
{
   CBgArray buffer;
   
   for(int y=0;y<GetYSize();y++){
      get_line(y,buffer);
      
      for(int x=0;x<buffer.size();x++){
         int x_new = GetXSize()-1-x;
         setXY(x_new,y,buffer[x]);
      }
   }
}

void CBgFits::Divide( CBgFits& right )
{
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      if ( right.data[i] != 0.00 ){
         data[i] = (data[i] / right.data[i]);
      }
   }
}


void CBgFits::Divide( int y, double value )
{
   for(int x=0;x<m_SizeX;x++){
      int pos = y*m_SizeX + x;
      
      data[pos] = data[pos] / value;
   }
}


void CBgFits::Divide( double value )
{
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      data[i] = (data[i] / value);
   }
}

void CBgFits::AvgChannels(int n_channels, CBgFits& outfits )
{
   int radius = (n_channels/2);
   

   for(int y=0;y<m_SizeY;y++){
      for(int x=0;x<m_SizeX;x++){
         int xx=(x-radius);
         if( xx <= 0 ){
            xx = 0;
         }
         if( xx >= (m_SizeX-n_channels) ){
            xx = m_SizeX-n_channels;
         }
      
         int added_channels=0;
         double sum=0;
         while( added_channels < n_channels && xx<m_SizeX ){
            sum += valXY(xx,y);
            added_channels++;
            xx++; 
         }
         double avg = (sum / added_channels);
         outfits.setXY(x,y,avg);
      }      
   }
}

bool CBgFits::Offset( double dx, double dy, CBgFits& out_fits, double multiplier )
{
   if( m_SizeX != out_fits.m_SizeX || m_SizeY != out_fits.m_SizeY ){
      printf("RESULT : image sizes differ %dx%d != %dx%d\n",m_SizeX,m_SizeY,out_fits.m_SizeX,out_fits.m_SizeY);
      return false;
   }

   int ret=0;
   int size = m_SizeX*m_SizeY;
   
   for(int y=0;y<m_SizeY;y++){
      for(int x=0;x<m_SizeY;x++){
         double val = getXY( x , y );
         int dx_int = int( double( dx ) );
         int dy_int = int( double( dy ) );
         
         
         out_fits.setXY( x + dx_int, y + dy_int, (val*multiplier) );
      }
   }

   return true;
}

int CBgFits::Compare( CBgFits& right, float min_diff, int verb, bool bIgnoreNans /*=false*/ )
{
   if( m_SizeX!=right.m_SizeX || m_SizeY!=right.m_SizeY ){
      printf("RESULT : image sizes differ %dx%d != %dx%d\n",m_SizeX,m_SizeY,right.m_SizeX,right.m_SizeY);
      return 1;
   }
   
   int ret=0;
   int size = m_SizeX*m_SizeY;
   double max_diff = -1e20;
   int max_diff_x=-1,max_diff_y=-1;
   int count_left_nans=0,count_right_nans=0;
   
   for(int i=0;i<size;i++){
      int channel = (i % m_SizeX);
      double freq = ch2freq(channel);
      
      if( isnan(data[i]) || isnan(right.data[i]) ){
         if( isnan(data[i]) ){
            count_left_nans++;
         }
         if( isnan(right.data[i]) ){
            count_right_nans++;
         }      
         
         if( bIgnoreNans ){
            ret++;
         }
      }else{      
         double fabs_diff = fabs(data[i]-right.data[i]);
      
      
         if( fabs_diff > min_diff ){
            if( verb >= 1 && gBGPrintfLevel >= BG_INFO_LEVEL ){
               // printf("%e != %e at (%d,%d) - %.2f [MHz]\n",data[i],right.data[i],(i % m_SizeX),(i / m_SizeX),freq);
               printf("%.12f != %.12f at (%d,%d) - %.2f [MHz]\n",data[i],right.data[i],(i % m_SizeX),(i / m_SizeX),freq);
            }
            ret++;
         }
      
         if( fabs_diff > max_diff ){
            max_diff = fabs_diff;
            max_diff_x = channel;
            max_diff_y = (i / m_SizeX);
         }
      }
   }

   if( count_left_nans>0 || count_right_nans>0 ){
      printf("NaNs detected in one or both images!\n");
      printf("\tLeft image has %d\n",count_left_nans);
      printf("\tRight image has %d\n",count_right_nans);
   }

   if( ret ){
      printf("RESULT : images have %d different pixels (maximum difference = %e at (%d,%d)). This includes NaNs !\n",ret,max_diff,max_diff_x,max_diff_y);
   }
   
   return ret;
}

void CBgFits::SubtractLines( int y1, int y0, const char* outfile)
{
   FILE* outf=NULL;
   if( outfile && strlen(outfile) ){
      outf = fopen(outfile,"w");
   }
   for(int x=0;x<m_SizeX;x++){
      double val1 = valXY(x,y1);
      double val0 = valXY(x,y0);
      double diff = val1 - val0;
      double freq = x*(480.000/4096.00);
      if(outf){
         fprintf(outf,"%d %.8f %.8f %.8f %.2f\n",x,diff,val1,val0,freq);      
      }
   }   
   
   if( outf ){
      fclose(outf);
   }
   
}

void CBgFits::DivideLines( int y1, int y0, const char* outfile )
{
   FILE* outf=NULL;
   if( outfile && strlen(outfile) ){
      outf = fopen(outfile,"w");
   }
   for(int x=0;x<m_SizeX;x++){
      double val1 = valXY(x,y1);
      double val0 = valXY(x,y0);
      double ratio = val1 / val0;
      double freq = x*(480.000/4096.00);
      if(outf){
         fprintf(outf,"%d %.8f %.8f %.8f %.2f\n",x,ratio,val1,val0,freq);      
      }
   }   
   
   if( outf ){
      fclose(outf);
   }
}

double CBgFits::MeanColumn( int x , double* out_rms )
{
    double mean_column=0, sum2=0.00;
    int count=0;
    
    for(int y=0;y<GetYSize();y++){
        double val = getXY(x,y);
        mean_column += val;
        sum2 += val*val;
        
        count++;    
    }
    
    double mean =  (mean_column/count);
    double rms = sqrt( sum2/count - (mean*mean) );
    
    if( out_rms ){
       (*out_rms) = rms;
    }
    
    return mean;
}

bool CBgFits::MapNoise( CBgFits& noise_mean, CBgFits& noise_rms, 
                        int iDebugLevel /*=0*/,
                        int fRadius /*=10*/,
                        int fFiducialRadiusAroundCenter /* = 1000000000*/,
                        bool bIQR /*=false*/, // calculate RMS_IQR instead of normal RMS
                        CBgFits* noise_median /*= NULL*/, CBgFits* noise_rmsiqr /*= NULL*/ )
{
  int xSize = GetXSize();
  int ySize = GetYSize();
  int x_center = xSize / 2;
  int y_center = ySize / 2;

  int all_count = 0 , not_nan = 0;  
  for(int y=0;y<ySize;y++){
     for(int x=0;x<xSize;x++){
         double dist = sqrt( (x-x_center)*(x-x_center) + (y-y_center)*(y-y_center) );

         if( dist < fFiducialRadiusAroundCenter || fFiducialRadiusAroundCenter <= 0 ){
            double mean, rms, minval, maxval, median, iqr, rms_iqr;
            int    count_points=0;

            double val = getXY(x,y);
//            printf("isnan(%.8f) = %d\n",val,isnan(val));
            if( isnan(val) == 0 && isinf(val) == 0 ){
//               printf("isnan(%.8f) = %d\n",val,isnan(val));
               GetStatRadiusAll( mean, rms, minval, maxval, median, iqr, rms_iqr, count_points, fRadius, bIQR, x, y, iDebugLevel );

//               printf("DEBUG : radius = %d : %.4f , %.4f , %.4f , %.4f , %.4f , %.4f\n", gRadius, mean, rms, minval, maxval, median, rms_iqr);

               noise_rms.setXY( x, y, rms );
               noise_mean.setXY( x, y, mean );

               if( noise_median ){
                  noise_median->setXY( x, y, median );
               }
               if( noise_rmsiqr ){
                  noise_rmsiqr->setXY( x, y, rms_iqr );
               }

               not_nan++;
            }
         }

         all_count++;
     }
     printf("y = %d\n",y);
  }

  printf("Not-NaN count = %d / %d\n",not_nan,all_count);
  
  return true;   
}

double CBgFits::GetStatBorder( double& mean, double& rms, double& minval, double& maxval, int border )
{
    return GetStat( mean, rms, minval, maxval, border, border, m_SizeX-border, m_SizeY-border );
}

double CBgFits::GetStat( double& mean, double& rms, double& minval, double& maxval, int x_start, int y_start, int x_end, int y_end  )
{
   double sum = 0.00;
   double sum2 = 0.00;
   int    cnt  = 0;
   
   minval = 1e6;
   maxval = -1e6;
   
   if( y_end < 0 ){
      y_end = m_SizeY;
   }
   if( x_end < 0 ){
      x_end = m_SizeX;
   }
  
   int nan_count = 0, total_count = 0;   
   int non_zero_count=0;
   for(int y=y_start;y<y_end;y++){
       for(int x=x_start;x<x_end;x++){
           double val = valXY(x,y);
           total_count++;
                      
           if ( isnan(val) || isinf(val) ){
              nan_count++;
              continue;
           }

           sum  += val;
           sum2 += val*val;
           cnt  += 1;                    
           
           if( val > maxval ){
               maxval = val;
           }
           if( val < minval ){
               minval = val;
           }
           if( fabs(val) > 0.0000000001 ){
              non_zero_count++;
           }
       }
   }
   
   mean = sum / cnt;
   rms  = sqrt( sum2/cnt - mean*mean );

   if( nan_count > 0 ){
      if( gBGPrintfLevel >= BG_WARNING_LEVEL ){
         printf("WARNING : %d / %d are NaN values found and ignored\n",nan_count,total_count);
      }
   }
   
//   printf("Non-zero count = %d\n",non_zero_count);
   
   return 0.00;
}

double CBgFits::GetStatRadiusAll( double& mean, double& rms, double& minval, double& maxval, 
                                  double& median, double& iqr, double& rms_iqr, int& cnt, int radius, 
                                  bool do_iqr /* = true */,
                                  int xc /*= -1*/, int yc /* = -1 */, int gDebugLevel /* = 0 */  )
{
   double sum = 0.00;
   double sum2 = 0.00;
   cnt  = 0;
   
   minval = 1e6;
   maxval = -1e6;
   iqr = 0.00;
   rms_iqr = 0.00;
   median  = 0.00;   

   int center_x = m_SizeX/2;
   int center_y = m_SizeY/2;
   
   if( xc >= 0 ){
       center_x = xc;
   }
   if( yc >= 0 ){
       center_y = yc;
   }

   int x_start = (center_x - radius);
   int y_start = (center_y - radius);
   
   int x_end   = (center_x + radius);
   int y_end   = (center_y + radius);
   
   int max_count = (y_end - y_start + 1)*(x_end - x_start + 1);
   double* values = NULL;
   if ( do_iqr ){
//      printf("DEBUG : allocating max_count = %d ( %d x %d , from %d pixels around (%d,%d) )\n",max_count,(y_end - y_start + 1),(x_end - y_start + 1),radius,center_x,center_y);
      values = new double[max_count];
   }
   
   int nan_count = 0, total_count = 0;   
   for(int y=y_start;y<y_end;y++){
       for(int x=x_start;x<x_end;x++){
           if ( x>=0 && x<m_SizeX && y>=0 && y<m_SizeY ){
              double distance = sqrt( (x-center_x)*(x-center_x) + (y-center_y)*(y-center_y) );
              
              if( distance <= radius ){           
                 double val = valXY(x,y);
                 total_count++;
                      
                 if ( isnan(val) || isinf(val) ){
                    nan_count++;
                    continue;
                 }
                 
                 if( values ){
                    if ( cnt < max_count ){
                       values[cnt] = val;
                    }else{
                       printf("ERROR : cnt = %d >= max_count allocated = %d -> skipped !!!\n",cnt,max_count);
                       continue;
                    }
                 }

                 sum  += val;
                 sum2 += val*val;
                 cnt  += 1;                    
           
                 if( val > maxval ){
                     maxval = val;
                 }
                 if( val < minval ){
                     minval = val;
                 }
              }
           }
       }
   }
   
   mean = sum / cnt;
   rms  = sqrt( sum2/cnt - mean*mean );

   if( nan_count > 0 ){
      if ( gDebugLevel > 0 ){
         printf("WARNING : %d / %d are NaN values found and ignored\n",nan_count,total_count);
      }
   }
   
   if( values ){
      int q75= int(cnt*0.75);
      int q25= int(cnt*0.25);

   
      my_sort_float( values, cnt );
      median = values[ cnt / 2 ];
      iqr = values[q75] - values[q25];
      rms_iqr = iqr / 1.35;
   
      delete [] values;
   }
   
   return 0.00;
}


double CBgFits::GetStatRadius( double& mean, double& rms, double& minval, double& maxval, int radius  )
{
   double sum = 0.00;
   double sum2 = 0.00;
   int    cnt  = 0;
   
   minval = 1e6;
   maxval = -1e6;

   int center_x = m_SizeX/2;
   int center_y = m_SizeY/2;

   int x_start = (center_x - radius);
   int y_start = (center_y - radius);
   
   int x_end   = (center_x + radius);
   int y_end   = (center_y + radius);
   
   int nan_count = 0, total_count = 0;   
   for(int y=y_start;y<y_end;y++){
       for(int x=x_start;x<x_end;x++){
           if ( x>=0 && x<m_SizeX && y>=0 && y<m_SizeY ){
              double distance = sqrt( (x-center_x)*(x-center_x) + (y-center_y)*(y-center_y) );
              
              if( distance <= radius ){           
                 double val = valXY(x,y);
                 total_count++;
                      
                 if ( isnan(val) || isinf(val) ){
                    nan_count++;
                    continue;
                 }

                 sum  += val;
                 sum2 += val*val;
                 cnt  += 1;                    
           
                 if( val > maxval ){
                     maxval = val;
                 }
                 if( val < minval ){
                     minval = val;
                 }
              }
           }
       }
   }
   
   mean = sum / cnt;
   rms  = sqrt( sum2/cnt - mean*mean );

   if( nan_count > 0 ){
      printf("WARNING : %d / %d are NaN values found and ignored\n",nan_count,total_count);
   }
   
   return 0.00;
}



double CBgFits::GetStat( CBgArray& avg_spectrum, CBgArray& rms_spectrum, 
                         int start_int, int end_int, const char* szState,
                         CBgArray* min_spectrum, CBgArray* max_spectrum,
                         double min_acceptable_value, int* out_number_of_used_integrations,
                         CBgFits* rfi_flag_fits_file )
{
   if( rfi_flag_fits_file ){
      if( GetXSize() != rfi_flag_fits_file->GetXSize() || GetYSize() != rfi_flag_fits_file->GetYSize() ){
         printf("ERROR : provided RFI flags file differs in size (%d,%d) from the analysed FITS file (%d,%d)\n",rfi_flag_fits_file->GetXSize(),rfi_flag_fits_file->GetYSize(),GetXSize(),GetYSize());
         return -100000;
      }
   }

   if( end_int < 0 ){
      end_int = m_SizeY;
   }

   long double sum=0.00,sum2=0.00,minval=10000000.00,maxval=-100000000.00;
   int size = m_SizeX*m_SizeY;
    
   int count=0,minpos=-1,maxpos=-1;
   int n_int=0;

   for(int y=start_int;y<end_int;y++){
      if( szState && szState[0] ){
         int range_idx;
         cIntRange* pRange = GetRange(y, range_idx);
         if( pRange ){
             int strcmp_ret =  strcmp(pRange->m_szName.c_str(),szState);
             if( strcmp_ret ){
                continue; // skip wrong integration type 
             }
         }else{
            continue;
         }
         
         if( gBGPrintfLevel >= BG_DEBUG_LEVEL ){
            printf("Integration %d used\n",y);
         }
      }
   
      for(int x=0;x<m_SizeX;x++){
         int i = y*m_SizeX+x;
         double val = getXY(x,y);
         
         if( rfi_flag_fits_file ){
            double flag = rfi_flag_fits_file->getXY(x,y);
//            double flag=0;
            if( flag > 0 ){
               continue;
            }
         }
         
         sum  += val;
         sum2 += (val*val);
      
         if( val > maxval ){
            maxval = val;
            maxpos = i;
         }
         if( val < minval ){
            minval = val;
            minpos = i;
         }      
         count++;
      }
      
      n_int++;
   }

   long double mean = (sum/count);
   long double mean2 = (sum2/count);
   long double avg2 = mean*mean;
   long double rms=0.00;
   if( mean2 >= avg2 ){
      rms = sqrt( mean2 - avg2 );   
   }else{
      printf("ERROR in CBgFits::GetStat mean2=%.4f < avg2=%.4f\n",(double)mean2,(double)avg2);
   }                                 

   CBgArray sum2_tab( GetXSize() , 0 ), count_tab( GetXSize(), 0 );
   avg_spectrum.alloc( GetXSize() , 0 );
   rms_spectrum.alloc( GetXSize() , 0 );
   if( min_spectrum ){
      min_spectrum->alloc( GetXSize() , 1000e9 );
   }
   if( max_spectrum ){
      max_spectrum->alloc( GetXSize() , -1000e9 );
   }
   int y_lines_counter=0;
   for(int y=start_int;y<end_int;y++){
/*      if( abs(y-1291) <= 2 ){
         continue;
      }      */      
      
      if( szState && szState[0] ){
         int range_idx;
         cIntRange* pRange = GetRange(y, range_idx);
         if( pRange ){
             int strcmp_ret =  strcmp(pRange->m_szName.c_str(),szState);
             if( strcmp_ret ){
                continue; // skip wrong integration type 
             }
         }else{
            continue;
         }
      }

      for(int x=0;x<m_SizeX;x++){
         double val = getXY(x,y);

         if( rfi_flag_fits_file ){
            double flag = rfi_flag_fits_file->getXY(x,y);
//            double flag=0;
            if( flag > 0 ){
               continue;
            }
         }                                                                     
      
         avg_spectrum[x] += val;
         sum2_tab[x] += val*val;
         count_tab[x] += 1;
      }
      y_lines_counter++;
   }
   
   
   for(int x=0;x<GetXSize();x++){
      int y_lines_count = count_tab[x];
      double mean = avg_spectrum[x] / y_lines_count;      
      
//      printf("%.2f MHz : %.4f = %.4f / %d\n",ch2freq(x),mean,avg_spectrum[x],y_lines_count);
      double mean2 = (sum2_tab[x] / y_lines_count );      
      
      if( y_lines_count <= 0 ){
         mean = 0;
         mean2 = 0;
      }

      rms_spectrum[x] = sqrt( mean2 - mean*mean );
      avg_spectrum[x] = mean;
      
      if( min_spectrum && max_spectrum ){
//         for(int y=0;y<GetYSize();y++){
         for(int y=start_int;y<end_int;y++){
            if( szState && szState[0] ){
               int range_idx;
               cIntRange* pRange = GetRange(y, range_idx);
               if( pRange ){
                   int strcmp_ret =  strcmp(pRange->m_szName.c_str(),szState);
                   if( strcmp_ret ){
                      continue; // skip wrong integration type 
                   }
               }else{
                  continue;
               }
            }
         
            double val = getXY(x,y);
            
            if( val < (*min_spectrum)[x] && val>=min_acceptable_value ){
               (*min_spectrum)[x] = val;
            }
            if( val > (*max_spectrum)[x] ){
               (*max_spectrum)[x] = val;
            }
         }         
      }
      
   }

   double inttime = GetIntTime();
   double total_inttime = inttime * n_int;

   double total_sum_test=0;
   int non_zero_count=0;
   for(int yy=0;yy<GetYSize();yy++){
      for(int xx=0;xx<GetXSize();xx++){
         double val =  getXY(xx,yy);
         total_sum_test += val;
         
         if( fabs(val) > 0.0000000001 ){
            non_zero_count++;
         }
      }
   }      

   // calculate total power of average integration in bedlam units and dBm
   double total_power = 0.00, total_power_mW = 0;
   for(int i=0;i<avg_spectrum.size();i++){
      double freq = ch2freq(i);
      total_power += avg_spectrum[i];
      total_power_mW += CBedlamSpectrometer::power2mW( freq,avg_spectrum[i]);
   }
   double total_power_dbm = mW2dbm( total_power_mW );

   if( gBGPrintfLevel >= BG_INFO_LEVEL ){
      printf("##################################### STATISTICS %d - %d #####################################\n",start_int,end_int);
      printf("Mean    = %e\n",(double)mean);
      printf("RMS     = %e\n",(double)rms);
      printf("SUM     = %.8f\n",total_sum_test);
      printf("Int count = %d\n",y_lines_counter);
      printf("MAX val = %.8f at (%d,%d)\n",(double)maxval,(maxpos%m_SizeX),(maxpos/m_SizeX));
      printf("MIN val = %.8f at (%d,%d)\n",(double)minval,(minpos%m_SizeX),(minpos/m_SizeX));
      printf("INTTIME = %d x %.8f [sec] = %.8f [sec]\n",n_int,inttime,total_inttime);
      printf("TOTAL POWER ( uxtime = %.8f ) = %.20f [?] = %.2f [dBm]\n",GetUnixTime(),total_power,total_power_dbm);
      printf("Non-zero values = %d\n",non_zero_count);
      printf("######################################################################################\n");
   }
   
   if( out_number_of_used_integrations ){
     (*out_number_of_used_integrations) = y_lines_counter;
   }
   
   return total_inttime;
}


double CBgFits::GetMaxPower( int integration, double& max_freq, int start_channel, int end_channel )
{
   double max_power=-1.00;
   max_freq = -1.00;

   if( end_channel < 0 ){
      end_channel = (m_SizeX-1);
   }
   
   for(int x=start_channel;x<end_channel;x++){
      double power = valXY(x,integration);
      if( power >= max_power ){
         max_power = power;
         max_freq = ch2freq(x);
      }
   }
   
   return max_power;

}

double CBgFits::GetTotalPower( int integration, int start_channel, int end_channel )
{
   double sum=0.00;

   if( end_channel < 0 ){
      end_channel = (m_SizeX-1);
   }
   
   for(int x=0;x<m_SizeX;x++){
      if( x>=start_channel && x<=end_channel ){      
         sum += valXY(x,integration);
      }
   }
   
   return sum;
}

double CBgFits::GetTotalPowerFreq( int integration, double start_freq, double end_freq )
{
   int start_ch = freq2ch(start_freq);
   int end_ch   = freq2ch(end_freq);
   
   return GetTotalPower( integration,start_ch,end_ch);
}


int CBgFits::Recalc( eCalcFitsAction_T action, double value )
{
   int size = m_SizeX*m_SizeY;
   
   for(int i=0;i<size;i++){
      switch( action ){

         case eInvert :            
            data[i] = 1.00 /  data[i];
            break;

         case eABS :            
            data[i] = fabs( data[i] );
            break;

         case eLog10File :
            data[i] = log10( data[i] );
            break;

         case eDBFile :
            data[i] = 10.00*log10( data[i] );
            break;

         case eLin2DB :
            data[i] = exp( (data[i] / 10) * log(10.0) );
            break;

         case eAstroRootImage :
//            data[i] = sqrt( data[i] );
//            if( data[i] > value ){
//               data[i] = value;
//            }
            if( data[i] > value ){
               data[i] = value;
            }
            break;

         case eSqrtFile :
            data[i] = sqrt( data[i] );
            break;
            
         case eDivideConst :
            data[i] = (data[i] / value);
            break;

         case eTimesConst :
            data[i] = (data[i] * value);
            break;

         case eAddConst :
            data[i] = (data[i] + value);
            break;
            
         default :
            printf("ERROR : unknown image action = %d, ignored !\n",action);
            return -1;
      }
   }                  
   
   return 1;
}

int CBgFits::GetMedianInt( CBgArray& median_int, CBgArray& rms_iqr_int )
{
   cIntRange range;
   vector<cIntRange> ranges;
   range.start_int = 0;
   range.end_int   = GetYSize()-1;
   ranges.push_back( range );  
   
   return GetMedianInt( ranges, median_int, rms_iqr_int );               
}


int CBgFits::GetMedianInt( vector<cIntRange>& int_ranges, CBgArray& median_int, CBgArray& rms_iqr_int )
{	
   double* median_tab = new double[GetYSize()];       

   if( median_int.size() != GetXSize() ){
      median_int.alloc(GetXSize());
   }
   if( rms_iqr_int.size() != GetXSize() ){
      rms_iqr_int.alloc(GetXSize());
   }

   int max_count=-1;
   for(int x=0;x<GetXSize();x++){
   
      int count=0;
      for(int i=0;i<int_ranges.size();i++){
         cIntRange& range = int_ranges[i];
      
         for(int y=range.start_int;y<=range.end_int;y++){
            median_tab[count] = valXY(x,y);
            count++;
         }            
      }         
      
      my_sort_float( median_tab, count );
      median_int[x] = median_tab[count/2];      

      int q75= (int)(count*0.75);
      int q25= (int)(count*0.25);
         
      if( count > max_count ){
         max_count = count;
      }   
         
      rms_iqr_int[x] = ( median_tab[q75] - median_tab[q25] ) / 1.35; // 2020-07-13 - division by 1.35 added !
   }
   
   delete [] median_tab;
   
   return max_count;
}

cIntRange* CBgFits::GetRange(int y,int& out_range_idx)
{
   for(int i=0;i<m_IntegrationRanges.size();i++){
      cIntRange& range = m_IntegrationRanges[i];
      if( range.start_int<=y && y<=range.end_int ){
         out_range_idx = i;
         return &range;
      }
   }
   
   return NULL;
}


eIntType CBgFits::GetIntType(int y)
{
   if( m_IntegrationRanges.size() <= 0 ){
      // if no ranges defined just return antenna :
      return eIntTypeANT;
   }

   for(int i=0;i<m_IntegrationRanges.size();i++){
      cIntRange& range = m_IntegrationRanges[i];
   
      if( range.start_int<=y && y<=range.end_int ){
         return (eIntType)range.inttype;
      }
   }  

   return eIntTypeUndefined;   
}

int CBgFits::IsAntenna(int y, int bDefaultYes )
{
   if( bDefaultYes > 0 ){
      if( m_IntegrationRanges.size() <= 0 ){
         return 1;
      }
   }

   for(int i=0;i<m_IntegrationRanges.size();i++){
      cIntRange& range = m_IntegrationRanges[i];
   
      if( range.inttype==eIntTypeANT && (range.start_int<=y && y<=range.end_int) ){
         return 1;
      }
   }  
   
   return 0;
}

int CBgFits::GetAntRanges( vector<cIntRange>& ranges )
{
   ranges.clear();
   if( m_IntegrationRanges.size() > 0 ){
      for(int i=0;i<m_IntegrationRanges.size();i++){
         cIntRange& range = m_IntegrationRanges[i];
               
         if( range.inttype==eIntTypeANT ){
            ranges.push_back(range);
         }
      }                                       
   }else{
      ranges.push_back( cIntRange(0,(GetYSize()-1)) );
   }
   
   return ranges.size();
}

int CBgFits::IsReference(int y)
{
   for(int i=0;i<m_IntegrationRanges.size();i++){
      cIntRange& range = m_IntegrationRanges[i];
   
      if( range.inttype==eIntTypeREF && (range.start_int<=y && y<=range.end_int) ){
         return 1;
      }
   }  
   
   return 0;
}

  
  
CBgFits* CBgFits::AllocOutFits( const char* fname, int _y_size, int bAddStates )
{
   int y_size = GetYSize();
   if( _y_size > 0 ){
      y_size = _y_size;
   }
   CBgFits* pOutFits = new CBgFits(fname,GetXSize(),y_size);   
   if( bAddStates ){
      pOutFits->SetKeys( GetKeys() );
   }else{
      pOutFits->SetKeysWithoutStates( GetKeys() );
   }
   pOutFits->Create(fname);
   pOutFits->WriteKeys();
   
   return pOutFits;
}

void CBgFits::ClearKeys()
{
   _fitsHeaderRecords.clear();
}

int CBgFits::SetKeysWithoutStates( std::vector<HeaderRecord>& keys )
{
   // _fitsHeaderRecords
     HeaderRecord* pStates = GetKeyword(keys,"STATES");

     vector<string> states_list;
     char szTmpList[128];
     if( pStates ){
        strcpy(szTmpList,pStates->Value.c_str());
        ParseCommaList( szTmpList , states_list );
     }

     // _fitsHeaderRecords
     for(int k=0;k<keys.size();k++){
        HeaderRecord& rec = keys[k];
                     
        const char* key = rec.Keyword.c_str();
        const char* value = rec.Value.c_str();
        const char* comment = rec.Comment.c_str();
        
        int bIsStateKey=0;
        if( strcmp(key,"STATES") == 0 ){
            bIsStateKey=1;
        }
        if( !bIsStateKey ){
           for(int i=0;i<states_list.size();i++){
              if( strstr(key,states_list[i].c_str()) ){
                 bIsStateKey=1;
                 break;
              }
           }
        }
                
        if( !bIsStateKey ){
           _fitsHeaderRecords.push_back(rec);
        }
     }
     
     return _fitsHeaderRecords.size();
}

void CBgFits::dump_max_hold( int start_int, int end_int, const char* szOutFile, int bShowFreq )
{
   float* out_line = new float[m_SizeX];
   
   for(int x=0;x<m_SizeX;x++){
      out_line[x] = -10e9;
   }
   
   for(int y=0;y<m_SizeY;y++){
      for(int x=0;x<m_SizeX;x++){
         if( valXY(x,y) > out_line[x] ){
           out_line[x] = valXY(x,y);
         }
      }
   }

   FILE* outfile = fopen(szOutFile,"w");
   for(int x=0;x<m_SizeX;x++){
       double x_val = x;
       if( bShowFreq ){
          x_val = ch2freq(x);
       }
          
      fprintf(outfile,"%.2f %e\n",x_val,out_line[x]);
   }
   fclose(outfile);
   
   delete [] out_line;        
}

void CBgFits::dump_min_hold( int start_int, int end_int, const char* szOutFile, int bShowFreq )
{
   float* out_line = new float[m_SizeX];
   
   for(int x=0;x<m_SizeX;x++){
      out_line[x] = 10e20;
   }
   
   for(int y=0;y<m_SizeY;y++){
      for(int x=0;x<m_SizeX;x++){
         if( valXY(x,y) < out_line[x] ){
           out_line[x] = valXY(x,y);
         }
      }
   }

   FILE* outfile = fopen(szOutFile,"w");
   for(int x=0;x<m_SizeX;x++){
       double x_val = x;
       if( bShowFreq ){
          x_val = ch2freq(x);
       }
          
      fprintf(outfile,"%.2f %e\n",x_val,out_line[x]);
   }
   fclose(outfile);
   
   delete [] out_line;        

}

void CBgFits::SetIntTimeKeyword( double _inttime )
{
   SetKeywordFloat("INTTIME",_inttime);
   SetKeywordFloat("EXPTIME",_inttime);
   SetKeywordFloat("CDELT2",_inttime);
   
   inttime = _inttime;
      
}

void CBgFits::PrepareBigHornsHeader( double ux_start, double _inttime, double freq_start, double delta_freq_mhz )
{
  time_t fs = (time_t)ux_start;
  int usec = (ux_start-fs)*1000000.00;
  
  if( gBGPrintfLevel >= BG_INFO_LEVEL ){
     printf("INFO : CBgFits::PrepareBigHornsHeader , freq_start = %.4f [MHz]\n",freq_start);
  }

  char szUtTime[128];
  strftime(szUtTime,80,"%Y-%m-%d %H:%M:%S",gmtime(&fs));
     
   SetKeyword("DTIME-FS",fs);
   SetKeyword("DTIME-FU",usec); 
   SetKeyword("DATE",szUtTime);
   SetKeywordFloat("INTTIME",_inttime);
   SetKeywordFloat("EXPTIME",_inttime);
                          
   SetKeyword("CRPIX1",1); // or 0.5-(1,1)=0.059MHz, 1->(1,1)=0Mhz-CORRECT, 0->(1,1)->0.117MHz
   SetKeyword("CTYPE1","Frequency");
   SetKeyword("CUNIT1","MHz");
   SetKeywordFloat("CRVAL1",freq_start);
   SetKeywordFloat("CDELT1",delta_freq_mhz);

   SetKeyword("CRPIX2",1); // or 0.5-(1,1)=0.059MHz, 1->(1,1)=0Mhz-CORRECT, 0->(1,1)->0.117MHz
   SetKeyword("CTYPE2","Time");
   SetKeyword("CUNIT2","sec");
   SetKeywordFloat("CDELT2",_inttime);                                                                                   
   
   inttime = _inttime;
}

void CBgFits::PrepareBigHornsHeaderTransposed( double ux_start, double _inttime, double freq_start, double delta_freq_mhz )
{
  time_t fs = (time_t)ux_start;
  int usec = (ux_start-fs)*1000000.00;

  char szUtTime[128];
  strftime(szUtTime,80,"%Y-%m-%d %H:%M:%S",gmtime(&fs));
     
   SetKeyword("DTIME-FS",fs);
   SetKeyword("DTIME-FU",usec); 
   SetKeyword("DATE",szUtTime);
   SetKeywordFloat("INTTIME",_inttime);
   SetKeywordFloat("EXPTIME",_inttime);
                          
   SetKeyword("CRPIX2",1); // or 0.5-(1,1)=0.059MHz, 1->(1,1)=0Mhz-CORRECT, 0->(1,1)->0.117MHz
   SetKeyword("CTYPE2","Frequency");
   SetKeyword("CUNIT2","MHz");
   SetKeywordFloat("CRVAL2",freq_start);
   SetKeywordFloat("CDELT2",delta_freq_mhz);

   SetKeyword("CRPIX1",1); // or 0.5-(1,1)=0.059MHz, 1->(1,1)=0Mhz-CORRECT, 0->(1,1)->0.117MHz
   SetKeyword("CTYPE1","Time");
   SetKeyword("CUNIT1","sec");
   SetKeywordFloat("CDELT1",_inttime);                                                                                   
   
   inttime = _inttime;
}



double CBgFits::GetChannelWidth()
{
   HeaderRecord* pHeader = GetKeyword("CDELT1");
   if( pHeader ){
      return atof(pHeader->Value.c_str());
   }
   
   return -1;
}

void CBgFits::Normalize( CBgArray& median_int )
{
   for(int y=0;y<GetYSize();y++){
      float* line = get_line(y);
      
      for(int x=0;x<GetXSize();x++){
         line[x] = line[x] / median_int[x];
      }
   }
}

int CBgFits::FindValue( double value, double delta, eFindValueType_T type /*=eFindValueExact*/ )
{
   int ret=0;

   for(int y=0;y<GetYSize();y++){
      float* line = get_line(y);
      
      for(int x=0;x<GetXSize();x++){
      
         if( type == eFindValueExact ){
            if( fabs(line[x]-value) < delta ){
               printf("(%d,%d) = %8f\n",x,y,line[x]);
               ret++;
            }
         }
         if( type == eFindValueSmaller ){
            if( line[x] < value ){
               printf("(%d,%d) = %8f\n",x,y,line[x]);
               ret++;
            }
         }
         if( type == eFindValueLarger ){
            if( line[x] >= value ){
               printf("(%d,%d) = %8f\n",x,y,line[x]);
               ret++;
            }
         }
      }
   }
   
   return ret;
}

void CBgFits::SetNaN()
{
   for(int y=0;y<GetYSize();y++){
      for(int x=0;x<GetXSize();x++){
//         setXY(x,y,FP_NAN);
         setXY(x,y,0.00/0.00); // NaN see https://en.cppreference.com/w/cpp/numeric/math/FP_categories
      }
   }
}

void CBgFits::SetValue(double value)
{
   for(int y=0;y<GetYSize();y++){
      for(int x=0;x<GetXSize();x++){
         setXY(x,y,value);
      }
   }
}

int CBgFits::CalcMedian( vector<string>& fits_list, CBgFits& out_rms, int bDoAverage )
{   
   int ySize=-1;
   int xSize=-1;
   vector<CBgFits*> fits_tab;
   for(int i=0;i<fits_list.size();i++){
      CBgFits* fits = new CBgFits( fits_list[i].c_str() );
      if( fits->ReadFits( fits_list[i].c_str() ) ){
         printf("ERROR : could not read first fits file %s\n",fits_list[i].c_str());
         exit(-1);
      }
      
      if( ySize < 0 ){
          ySize = fits->GetYSize();
          xSize = fits->GetXSize();
      }else{
         if( ySize != fits->GetYSize() ){
            printf("ERROR : not all fits files have equal ySize - %d != %d (file %s)\n",ySize,fits->GetYSize(),fits_list[i].c_str());
            return -1;
         }
      }
      fits_tab.push_back( fits );
      
/*      if( i==0 ){
         Realloc(fits.GetXSize(),fits_list.size()*fits.GetYSize());
         reset_lines_counter();
      }*/
            
//      for(int y=0;y<fits.GetYSize();y++){
//         add_line( fits.get_line(y), fits.GetXSize() );
//      }                                
   }
   
   if( gBGPrintfLevel >= BG_INFO_LEVEL ){
      printf("INFO : Re-sizing current image to (%d,%d)\n",xSize,ySize);
   }
   Realloc(xSize,ySize);
   out_rms.Realloc(xSize,ySize);
   
   CBgFits tmp_fits( xSize, fits_list.size() );
   for(int y=0;y<ySize;y++){
      for(int i=0;i<fits_list.size();i++){
         tmp_fits.set_line(i,(fits_tab[i])->get_line(y));
      }
      
      CBgArray median_int, rms_iqr_int;
      if( bDoAverage > 0 ){
         tmp_fits.GetStat( median_int, rms_iqr_int );
      }else{
         tmp_fits.GetMedianInt( median_int, rms_iqr_int );
      }
      set_line(y,median_int);
      out_rms.set_line(y,rms_iqr_int);
   }

   // clean memory 
   for(int i=0;i<fits_list.size();i++){
      delete (fits_tab[i]);
   }
   
   return 1;
}

int CBgFits::IsFlagged( int integration )
{
   std::vector<HeaderRecord>& keys = GetKeys();
      
   for(int k=0;k<keys.size();k++){
      HeaderRecord& key = keys[k];
               
      if( strcasecmp(key.Keyword.c_str(),"flag")==0 ){
         int keyval = atol(key.Value.c_str());
         if( keyval == (integration+1) ){
            return 1;
         }
      }  
   }
                                                                        
   return 0;
}
                                                                           

int CBgFits::IsRFI_OK( int integration, double& out_total_power, double& max_ch_power_dbm, double& max_freq, double& local_threshold, double& orbcomm_total_power, int& out_rejection_reason )
{
   out_rejection_reason = REJECTION_NOT_REJECTED;
   local_threshold = -1;
   int ret=TRUE;
   double uxtime = GetUnixTime( integration );
   
   out_total_power = GetTotalPower(integration);

   double total_power_max_value = CTotalPowerList::GetTotalPowerThreshold(uxtime);
   if( total_power_max_value > 0 ){
      if( out_total_power > total_power_max_value ){
         ret = FALSE;
         out_rejection_reason |=  REJECTION_TOTAL_POWER;
      }
   }
   
   if( CTotalPowerList::gTotalPowerCuts.size() > 0 ){
      for(int i=0;i<CTotalPowerList::gTotalPowerCuts.size();i++){
         cValue& cut = CTotalPowerList::gTotalPowerCuts[i];
         
         double total_power = GetTotalPowerFreq( integration, cut.x, cut.y );
         if( total_power > cut.z ){
            ret = FALSE;
            out_rejection_reason |= REJECTION_TOTAL_POWER_FREQ;
         }
      }
   }
   
   // check of maximum power in a single channel to skip data affected by noise floor change due to too much power in a single tone :
   int bighorns_max_channel = freq2ch(BIGHORNS_MAX_FREQ_MHZ); // check max power up to 360 MHz only, ignore everything above, as it was not properly calibrated and it is suppressed by the filters 
   double max_power = GetMaxPower(integration,max_freq,0,bighorns_max_channel);
   max_ch_power_dbm = mW2dbm( max_power/CBedlamSpectrometer::spectrum_response_model(max_freq) );
   if( max_ch_power_dbm > CTotalPowerList::gMaxChannelPower ){
      // maximum power in a single channel too high 
      out_rejection_reason |= REJECTION_CHANNEL_POWER;
      ret = FALSE;
   }
   
   // ORBCOMM TOTAL POWER :
   double orbcomm_total_power_bedlam = GetTotalPowerFreq( integration, 137.1, 138.5 );
   orbcomm_total_power = CBedlamSpectrometer::power2dbm( (137.1+138.5)/2.00 , orbcomm_total_power_bedlam );
   if( orbcomm_total_power > CTotalPowerList::gMaxChannelPower ){
      out_rejection_reason |= REJECTION_ORBCOMM_POWER;
      ret = FALSE;
   }
   
   if( CTotalPowerList::gLocalMedianSigma.size() > 0 ){
      double local_median, local_sigma;
      CTotalPowerList::gLocalMedianSigma.interpolate_both( uxtime, local_median, local_sigma  );
      local_threshold = local_median + local_sigma * CTotalPowerList::gLocalTotalPowerCutThreshold;
      
      if( out_total_power > local_threshold ){
         out_rejection_reason |= REJECTION_TOTAL_POWER_LOCALCUT; 
         ret = FALSE;
      }
   }
   
   if( !IsFlagged(integration) ){
      
   }
   
   return ret;
}

int CBgFits::FlagFile()
{
   CBgFits* pRFIMask = GetRFIMask();
   if( pRFIMask ){
      int ret=0;
      for(int y=0;y<GetYSize();y++){
         ret += FlagInt(y,pRFIMask);
      }
      return ret;
   }
   
   return 0;
}

int CBgFits::FlagInt( int y )
{
   CBgFits* pRFIMask = GetRFIMask();
   if( pRFIMask ){
      float* spec_buffer = get_line(y);
      float* mask_line = pRFIMask->get_line(y); 

      for(int l=0;l<GetXSize();l++){
         if( mask_line[l] > 0 ){
            // RFI channel :
            spec_buffer[l] = BIGHORNS_RFI_VALUE; // -1000
         }
      }                                                                                                                                                                  
       
      return 1;
   }
   
   return 0;
}

int CBgFits::FlagInt( int y, CBgFits* pRFIMask  )
{
   if( pRFIMask ){
      float* spec_buffer = get_line(y);
      float* mask_line = pRFIMask->get_line(y); 

      for(int l=0;l<GetXSize();l++){
         if( mask_line[l] > 0 ){
            // RFI channel :
            spec_buffer[l] = BIGHORNS_RFI_VALUE; // -1000
         }
      }                                                                                                                                                                  
       
      return 1;
   }
   
   return 0;
}


CBgFits* CBgFits::GetRFIMask()
{
   if( strlen(GetFileName()) && strlen(gInAOFlaggerDir.c_str()) ){
      string expected_mask_filename=gInAOFlaggerDir.c_str(),szFitsBaseName;
      getbasename_new(GetFileName(),szFitsBaseName);
      expected_mask_filename += szFitsBaseName.c_str();
      expected_mask_filename += "_flag.fits";
      if( !m_pRFIMask ){
         m_pRFIMask = new CBgFits(GetXSize(),GetYSize());
      }
      
      if( !m_pRFIMask->GetFileName() || strcmp(expected_mask_filename.c_str(),m_pRFIMask->GetFileName()) ){
         // change of fits file -> read new mask file
         if( m_pRFIMask->ReadFits( expected_mask_filename.c_str() ) ){
            printf("WARNING : could not read RFI mask file %s\n",expected_mask_filename.c_str());
            return NULL;
            //                    exit(-1);
         }
         printf("INFO : RFI mask file %s read OK\n",expected_mask_filename.c_str());
      }                                                                                                                                                                    
   }   
      
   return m_pRFIMask;
}

int CBgFits::SaveAsByte( const char* outfile )
{
   if( outfile && outfile[0] ){
      MyFile::CreateDir(outfile);
      
      long naxes[2] = { m_SizeX, m_SizeY };   /* image is 300 pixels wide by 200 rows */
      long naxis    = 2;
      long fpixel   = 1; /* first pixel to write      */
      int bitpix_char=8;
      int status=0;
      fitsfile* fptr;
      
      string szFitsFileToOverwrite; // with ! mark added to overwrite if file exists (otherwise ERROR occures), see http://www.aip.de/~weber/doc/fitsio/cfitsiohtml/node62.html
      szFitsFileToOverwrite = "!";
      szFitsFileToOverwrite += outfile;
      if (fits_create_file(&fptr, szFitsFileToOverwrite.c_str(), &status)){ /* create new FITS file */
         if (status) fits_report_error(stderr, status);            
         return( status );
      }
                
      /* Write the required keywords for the primary array image */
      if ( fits_create_img(fptr,  bitpix_char, naxis, naxes, &status) ){
         if (status) fits_report_error(stderr, status);
         return( status );
      }
      
      int nelements = naxes[0] * naxes[1];          /* number of pixels to write */

      char* tmp_data = new char[nelements];
      for(int i=0;i<nelements;i++){
         tmp_data[i] = 0;
         if( data[i] > 0 ){
            tmp_data[i] = 1;
         }
      }
                                                                     
      /* Write the array of long integers (after converting them to short) */
      int ret = fits_write_img(fptr, TBYTE, fpixel, nelements, tmp_data, &status);
      delete tmp_data;

      if( ret )
         return( status );
         
      
         
//      SetKeyValues(); // set values like inttime etc to strings keyword values 

      WriteKeys(fptr); // write fits keys 

      fits_close_file(fptr, &status);            /* close the file */
      fptr = NULL;
      printf("SUCCESS : written flag file in TBYTE format to file %s\n",szFitsFileToOverwrite.c_str());
      
      return( status );        
   }
   
   return -1;
}


double CBgFits::FitPoly( int y, double fit_min_freq, double fit_max_freq, double& A, double& B )
{
   double* fit_spec_freq = new double[GetXSize()];
   double* fit_spec_pwr = new double[GetXSize()];
                 
   int cnt=0;
   for(int x=0;x<=GetXSize();x++){
      double freq = ch2freq(x);                    
                    
      if( fit_min_freq<=freq && freq<=fit_max_freq ){
         fit_spec_freq[cnt] = freq;
         fit_spec_pwr[cnt]  = valXY(x,y);
         cnt++;
      }
   }
                 
   double aa,bb,cc;
   CMyFit::FitLine( fit_spec_freq, fit_spec_pwr, cnt, aa, bb, cc );
                     
   A = (-aa/bb);
   B = (-cc/bb);
                 
   double fit_chi2=0.00;
   double tau = GetIntTime();
   double bandwidth = GetChannelWidth();
   for(int x=0;x<=GetXSize();x++){
      double freq = ch2freq(x);
                                     
      if( fit_min_freq<=freq && freq<=fit_max_freq ){
         double fit_val = A*freq + B;
                       
         double diff = (fit_val - valXY(x,y));
         double err  = valXY(x,y)/sqrt(tau*bandwidth);
         fit_chi2 += (diff/err)*(diff/err);
      }
   }
                 
                 
   delete [] fit_spec_freq;
   delete [] fit_spec_pwr;                    

   return fit_chi2;
}


int CBgFits::ZeroWrongValues( double limit )
{
   int count_bad=0;
   
   for(int y=0;y<m_SizeY;y++){
      for(int x=0;x<m_SizeX;x++){
          double val = valXY(x,y);
          if( fabs(val) > limit ){
             setXY(x,y,0);
             count_bad++;
          }
      }
   }

   return count_bad;            
}

int CBgFits::FixBadValues( double minValueOK, double maxValueOK )
{
   int count_bad=0;
   for(int y=0;y<m_SizeY;y++){
      for(int x=0;x<m_SizeX;x++){
         double val = valXY(x,y);
         
         if( val > maxValueOK ){
            setXY(x,y,maxValueOK );
         }
         if( val < minValueOK ){
            setXY(x,y,minValueOK );
         }
      }
   }
   
   return count_bad;
}

void CBgFits::MeanLines( CBgArray& mean_lines, CBgArray& rms_lines )
{
   vector<int> counter;
   
   mean_lines.assign( GetYSize() , 0 );
   rms_lines.assign( GetYSize() , 0 );
   counter.assign( GetYSize(), 0 );
   
   for(int y=0;y<GetYSize();y++){
      double sum2 = 0.00;
      for(int x=0;x<GetXSize();x++){
         double value = getXY(x,y);
         
         if( !isnan(value) && !isinf(value) ){
            mean_lines[y] += value;
            sum2          += value*value;
            counter[y]    += 1;
         }
      }
      
      mean_lines[y] = mean_lines[y] / counter[y];
      rms_lines[y]  = sqrt( (sum2/counter[y]) - (mean_lines[y]*mean_lines[y]) );
   }   
}


void CBgFits::Transpose( CBgFits& out_fits_t )
{
   for(int ch=0;ch<m_SizeX;ch++){
      for(int t=0;t<m_SizeY;t++){
         double val = getXY( ch, t );
         out_fits_t.setXY( t, ch, val );
      }      
   }
   
   // void CBgFits::PrepareBigHornsHeaderTransposed( double ux_start, double _inttime, double freq_start, double delta_freq_mhz )
   // PrepareBigHornsHeaderTransposed( 
   out_fits_t.SetKeyword("CRPIX2",1); // or 0.5-(1,1)=0.059MHz, 1->(1,1)=0Mhz-CORRECT, 0->(1,1)->0.117MHz
   out_fits_t.SetKeyword("CTYPE2","Frequency");
   out_fits_t.SetKeyword("CUNIT2","MHz");
   out_fits_t.SetKeywordFloat("CRVAL2",start_freq);
   out_fits_t.SetKeywordFloat("CDELT2",delta_freq);

   out_fits_t.SetKeyword("CRPIX1",1); // or 0.5-(1,1)=0.059MHz, 1->(1,1)=0Mhz-CORRECT, 0->(1,1)->0.117MHz
   out_fits_t.SetKeyword("CTYPE1","Time");
   out_fits_t.SetKeyword("CUNIT1","sec");
   out_fits_t.SetKeywordFloat("CDELT1",inttime);

}

void CBgFits::ReadCRValues()
{
   crval1 = 0.00;   // start physical value
   cdelt1 = 1.00; // delta of physical value
   crpix1 = 1.00; // which pixel is the start 
   crval2 = 0.00;   // start physical value
   cdelt2 = 1.00; // delta of physical value
   crpix2 = 1.00; // which pixel is the start 
   // HeaderRecord* CBgFits::GetKeyword(const char *keyword)
   HeaderRecord* pKey = NULL;
   pKey = GetKeyword( "CRVAL1" );
   if( pKey ){
     crval1 = atof( pKey->Value.c_str() );
   }
   pKey = GetKeyword( "CDELT1" );
   if( pKey ){
     cdelt1 = atof( pKey->Value.c_str() );
   }

   pKey = GetKeyword( "CRPIX1" );
   if( pKey ){
     crpix1 = atof( pKey->Value.c_str() );
   }

   pKey = GetKeyword( "CRVAL2" );
   if( pKey ){
     crval2 = atof( pKey->Value.c_str() );
   }

   pKey = GetKeyword( "CDELT2" );
   if( pKey ){
     cdelt2 = atof( pKey->Value.c_str() );
   }

   pKey = GetKeyword( "CRPIX2" );
   if( pKey ){
     crpix2 = atof( pKey->Value.c_str() );
   }
}
