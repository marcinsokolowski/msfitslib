#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>


#include <bg_globals.h>
#include <bg_fits.h>
#include <bg_array.h>
#include <bg_defines.h>
#include <cvalue_vector.h>
#include <bg_bedlam.h>
#include <bg_units.h>
#include <bg_total_power.h>

#include <vector>
using namespace std;

//
#include <myfile.h>
#include <myparser.h>
#include <mystring.h>
#include <mystrtable.h>
#include <basestructs.h>
#include <myprogress.h>
#include <myfits.h>
// #include <mydate.h>
// time_t get_gmtime_from_string( const char* szGmTime ); // mydate.h has some conflict ...

time_t timegm_local( struct tm *tm )
{
        return timegm( tm );
}


time_t get_gmtime_from_string_local( const char* szGmTime , const char* format="%Y%m%d_%H%M%S" )
{
   struct tm gmtime_tm;
   // sscanf( szGmTime, "%.4u%.2u%.2u_%.2u%.2u%.2u", &gmtime_tm.tm_year,&gmtime_tm.tm_mon,
   // &gmtime_tm.tm_mday,&gmtime_tm.tm_hour,&gmtime_tm.tm_min,&gmtime_tm.tm_sec);

   strptime( szGmTime, format, &gmtime_tm );

   // gmtime_tm.tm_year -= 1900;
   // gmtime_tm.tm_mon--;
   time_t ret = timegm_local( &gmtime_tm );
        // time_t ret = timegm( &gmtime_tm );
   return ret;
}


enum eOutUnits { eOutUnitBedlamPower=0, eOutUnitmW=1, eOutUnitDBM=2 };
eOutUnits gOutUnit=eOutUnitBedlamPower;

const char* get_unit_desc( eOutUnits unit )
{
   switch( unit )   
   {
      case eOutUnitBedlamPower :
         return "BEDLAM";
      case eOutUnitmW :
         return "mW";
      case eOutUnitDBM :
         return "dBm";
      default :
         break;
   }
   
   return "UNKNOWN";
}

// acc5859_ch0_20170217_022101_099522.fits
// time_t file_start_uxtime = filename2ux( fits_list[i].c_str() );
time_t filename2ux( const char* fitsfile )
{
   const char* ptr = strstr(fitsfile,"acc");
   
   if( !ptr ){
      printf("WARNING : prefix acc not found in file name %s -> trying specdump\n",fitsfile);fflush(stdout);
       ptr = strstr(fitsfile,"specdump");
       
       if( !ptr ){
          printf("WARNING : prefix specdump not found in file name %s -> returning 0\n",fitsfile);fflush(stdout);
          return 0;
       }
   }
   
   char szDTM[128];
   memset(szDTM,'\0',128);
   strncpy(szDTM,ptr+12,15);
   time_t ux = get_gmtime_from_string_local(szDTM);
   
   return ux;
}
        

string out_file;
int gStartChannel=0;
int gEndChannel=-1;
double gStartFreq=-1000;
double gEndFreq=-1000;
CValueVector gExcludedFreqRanges;

string gLastProcessed;
string gLastFileStampfile="total_power_last_processed_file.txt";
int gStartFromLast=0;

// integration time stamp:
string gIntTimeStampFile;
int gTimeStampIntCol=1;    // default columns for MEDIAN.txt file 
int gTimeStampUxTimeCol=3; // default columns for MEDIAN.txt file
CValueVector gIntTimeStamps;
int gCalcMeanTotalPower=0;

// DB 
int gSQL=0;
int gORBCOMM=0;

int gMinX=0;
int gMaxX=-1;

int gDebug=1;

string gState; // dumps total power of a given state only 

// maximum rms chunk 
int gMaxRMSChunk=-1;

// power limits 
double gMinPower = -1e20;
double gMaxPower = -1;

// uxtime limits :
double gStartUX=-1;
double gEndUX=-1;

enum eOutColumns {eOutColumnsAll=0, eOutColumnsTimePower=12 };
eOutColumns gOutColumns = eOutColumnsAll;
string szOutOption="w";
int gShowMaxPower=0;

double gCalcMedianSigmaNSecAround = -1; // 1800 sec OK

// Set to >0 in order to exclude given number of brightest channels: 
int gExcludeNBrightestChannels=-1;

// KURTOSIS:
int gCalcKurtosis=0;

// fit polN
int gFitPolN=-1;
double gFitPolN_MinFreq=0;
double gFitPolN_MaxFreq=480;

int gWriteNow=0;

int gDoSQR=0;

/*int freq2channel( double freq )
{
   int ch = (int)(freq*(4096/480.00));
   
   return ch;
}*/

// total power cut threshold specified for different time ranges :
// RFI rejection :
double gTotalPowerMaxValue=25e9;
vector<cTotPowerCut> gTotalPowerThreshRanges;

double GetTotalPowerThreshold( double uxtime )
{
   if( gTotalPowerThreshRanges.size() > 0 ){
      for(int i=0;i<gTotalPowerThreshRanges.size();i++){
         cTotPowerCut& cut = gTotalPowerThreshRanges[i];
                         
         if( cut.min_uxtime<=uxtime && uxtime<=cut.max_uxtime ){
            return cut.threshold;
         }
      }
   }
                                                                     
   return gTotalPowerMaxValue;
}

                                                                        
                                                                        

double calc_rmses(vector<cTotalPowerInfo>& total_power_list, double inttime );

void usage()
{
   printf("total_power LISTFILE_OR_SINGLE_FITSFILE -h -f OUTFILE -s %d -e %d -l FREQ_LOWER -u FREQ_UPPER -m %d -x %d -t INTTYPE -d -a MIN_POWER -b MAX_POWER -g\n",gStartChannel,gEndChannel,gMinX,gMaxX);
   printf("Use -l / -u to specify range in frequency units [MHz] not in channels -s/-e\n");
   printf("-m MINIMUM_Y - to specify y range\n");
   printf("-x MAXIMUM_Y - to specify y range, -1 means to last integration (SizeY of fits file)\n");
   printf("-t STATE_NAME - to specify integration type ANT, TERM or so (according to FITS file) only this type will be dumped [default not specified]\n");
   printf("-d : increases debug level\n");
   printf("-f OUTFILE : output file [default totalpower_vs_time.txt]\n");
   printf("-c COLUMNS : columns list starting from 0, example -c 12 (to print only UNIXTIME POWER), default ALL\n");
   printf("-g : append to existing file\n");
   printf("-o START_UX [default not set]\n");
   printf("-p END_UX [default not set]\n");
   printf("-q PARAM=VALUE :\n");
   printf("\t\t\texclude=136,138 : in order to exclude certain frequency range\n");
   printf("\t\t\tcalc_median_sigma_n_sec_around=-1 : enable calculation of local median/sigma_iqr around every unix time to calculate local total power threshold [default disabled, good value 1800 , 900 - is a bit affected by local ORBCOMM peaks etc]\n");
   printf("\t\t\tunit=dbm : be default bedlam units are used, possible options = dbm , mW, bedlam, raw\n");
   printf("\t\t\ttotal_power_cut_range=MIN_UXTIME,MAX_UXTIME,THRESHOLD - specifies total power cut threshold for specific time ranges [default same cut %e for all the data] - IT'S ONLY USED FOR LOCAL MEDIAN !!!\n",gTotalPowerMaxValue);
   printf("\t\t\tlast_processed=last.fits - specify last processed file to skip all files before it [by default read from file %s]\n",gLastFileStampfile.c_str());
   printf("\t\t\tstart_from_last=%d - if start from last processed file [default = %d]\n",gStartFromLast,gStartFromLast);
   printf("\t\t\ttotal_power_stampfile=%s - file where last processed fits file is saved [default %s]\n",gLastFileStampfile.c_str(),gLastFileStampfile.c_str());
   printf("\t\t\t\t\t\tWARNING !!! : verify columns as by default they are %d and %d for the MEDIAN.txt file (use total_power_stampfile_intcol and total_power_stampfile_uxcol!!!\n",gTimeStampIntCol,gTimeStampUxTimeCol);
   printf("\t\t\ttotal_power_stampfile_intcol=%d : integration column in total_power_stampfile\n",gTimeStampIntCol);   
   printf("\t\t\ttotal_power_stampfile_uxcol=%d : integration column in total_power_stampfile\n",gTimeStampUxTimeCol);   
   printf("\t\t\tshow_max_power=%d - if include frequency of maximum power channel and the maximum power [default %d]\n",gShowMaxPower,gShowMaxPower);
   printf("\t\t\texclude_n_birghtest=%d - if exclude N brightest channels [default disbled = %d]\n",gExcludeNBrightestChannels,gExcludeNBrightestChannels);
   printf("\t\t\tkurtosis=%d - enable calculation of kurtosis [default %d]\n",gCalcKurtosis,gCalcKurtosis);
   
   printf("\t\t\tfit_pol_n=%d - enable fitting of polN to spectra in a given range [default %d]\n",gFitPolN,gFitPolN);
   printf("\t\t\tfit_pol_n_min_freq=%.2f - minimum frequency to fit pol N\n",gFitPolN_MinFreq);
   printf("\t\t\tfit_pol_n_max_freq=%.2f - minimum frequency to fit pol N\n",gFitPolN_MaxFreq);
   printf("\t\t\tint_time_stamp_file=int_timestamp.txt : time stamp (unix time) of each integration [default not set]\n");
   printf("\t\t\tmean_total_power=%d : to average over channels [default = %d]\n",gCalcMeanTotalPower,gCalcMeanTotalPower);
   printf("\t\t\twrite_now=%d : to write output file all the time (not at the very end). Does not print statistics, but saves memory [default %d]\n",gWriteNow,gWriteNow);
   printf("\t\t\tsql=%d : generate SQL file to be loaded into the database [default %d]\n",gSQL,gSQL);
   printf("\t\t\torbcomm=%d : calculate power in ORBCOMM band (137.1 - 138.05 MHz) , default %d\n",gORBCOMM,gORBCOMM);
   printf("\t\t\tdo_sqr=%d : SQR(power) default = %d\n",gDoSQR,gDoSQR);
   exit(-1);
}

void print_parameters()
{
   printf("#####################################\n");
   printf("PARAMETERS :\n");
   printf("#####################################\n");
   printf("out_file         = %s\n",out_file.c_str());
   printf("start from last  = %d\n",gStartFromLast);
   printf("last processed   = %s\n",gLastProcessed.c_str());
   printf("stamp file (for last processed) = %s\n",gLastFileStampfile.c_str());
   printf("start channel    = %d\n",gStartChannel);
   printf("end channel      = %d\n",gEndChannel);
   printf("minimum X        = %d\n",gMinX);
   printf("maximum X        = %d\n",gMaxX);
   printf("Integration type = %s\n",gState.c_str());
   printf("Power range      = %e - %e\n",gMinPower,gMaxPower);
   printf("Out columns      = %d\n",gOutColumns);
   printf("UX time range    = %.2f - %.2f\n",gStartUX,gEndUX);
   printf("Calc median/sigma %.1f sec around each point\n",gCalcMedianSigmaNSecAround);
   printf("Number of exclusion ranges = %d\n",(int)gExcludedFreqRanges.size());
   if( gExcludedFreqRanges.size() > 0 ){
      printf("\tExcluded frequency ranges :\n");
      for(int i=0;i<gExcludedFreqRanges.size();i++){
         printf("\t\t\t%.2f - %.2f MHz\n",gExcludedFreqRanges[i].x,gExcludedFreqRanges[i].y);
      }
   }
   printf("Output unit      = %s\n",get_unit_desc(gOutUnit));
   printf("Show max power   = %d\n",gShowMaxPower);
   printf("Find local median/iqr around = %.2f [sec]\n",gCalcMedianSigmaNSecAround);
   if( gCalcMedianSigmaNSecAround > 0 ){
      if( gTotalPowerThreshRanges.size() ){
         printf("Total power threshold in time ranges :\n");
         for(int i=0;i<gTotalPowerThreshRanges.size();i++){
            cTotPowerCut& cut = gTotalPowerThreshRanges[i];
            printf("\t%.4f - %.4f : %e\n",cut.min_uxtime,cut.max_uxtime,cut.threshold);
         }
      }                                          
   }
   printf("Exclude N birghtest = %d\n",gExcludeNBrightestChannels);
   printf("Calculate kurtosis  = %d\n",gCalcKurtosis);
   printf("Fit pol N           = %d\n",gFitPolN);
   printf("Fit pol N freq range = %.2f - %.2f [MHz]\n",gFitPolN_MinFreq,gFitPolN_MaxFreq);
   printf("Integration time stamp file = %s (read %d points using columns %d and %d)\n",gIntTimeStampFile.c_str(),int(gIntTimeStamps.size()),gTimeStampIntCol,gTimeStampUxTimeCol);
   printf("Mean total power    = %d\n",gCalcMeanTotalPower);
   printf("Write now           = %d\n",gWriteNow);
   printf("Generate SQL file   = %d\n",gSQL);
   printf("Include ORBCOMM     = %d\n",gORBCOMM);
   printf("gDoSQR              = %d\n",gDoSQR);
   printf("#####################################\n");   
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "dhgf:s:e:l:u:m:x:t:a:b:c:o:p:q:";
   int opt,opt_param,i;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
      switch (opt) {
         case 'a':
            if( optarg ){
               gMinPower = atof(optarg);
            }
            break;
         case 'b':
            if( optarg ){
               gMaxPower = atof(optarg);
            }
            break;
         case 'c':
            if( optarg ){
               int cols = atol(optarg);
               if( cols > 0 ){
                  gOutColumns = (eOutColumns)cols;
               }
            }
            break;
         case 'f':
            if( optarg ){
               out_file = optarg;
            }
            break;
         case 'g':
            szOutOption = "a+";
            break;
         case 'm':
            if( optarg ){
               gMinX = atol(optarg);
            }
            break;
         case 'x':
            if( optarg ){
               gMaxX = atol(optarg);
            }
            break;
         case 's':
            if( optarg ){
               gStartChannel = atol(optarg);
            }
            break;
         case 'e':
            if( optarg ){
               gEndChannel = atol(optarg);
            }
            break;
         case 'l':
            if( optarg ){
//               gStartChannel = freq2channel( atof(optarg) );
                 gStartFreq = atof(optarg);
            }
            break;   
         case 't':
            if( optarg ){
               gState = optarg;
            }
            break;   
         case 'u':
            if( optarg ){
//               gEndChannel = freq2channel( atof(optarg) );
                 gEndFreq = atof(optarg);
            }
            break;   
         case 'o':
            if( optarg ){
               gStartUX = atof(optarg);
            }
            break;   
         case 'p':
            if( optarg ){
               gEndUX = atof(optarg);
            }
            break;   
         case 'q':
            if( optarg ){
               MyParser pars = optarg;
               CEnvVar envvar;
               if( pars.GetVarAndValue(envvar) ){
                  if( strcmp(envvar.szName.c_str(),"exclude")==0 ){
                     // envvar.szValue.c_str()
                     MyParser szCut = envvar.szValue.c_str();
                     CMyStrTable items;
                     szCut.GetItems(items);
                     if( items.size() >= 2 ){
                        cValue freq_range;
                        freq_range.x = atof(items[0].c_str());
                        freq_range.y = atof(items[1].c_str());
                        gExcludedFreqRanges.push_back(freq_range);
                     }                                                               
                  }
                  if( strcmp(envvar.szName.c_str(),"calc_median_sigma_n_sec_around")==0 ){
                     gCalcMedianSigmaNSecAround = atof(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"last_processed")==0 ){
                     gLastProcessed = envvar.szValue.c_str();
                     gStartFromLast = 1;
                  }
                  if( strcmp(envvar.szName.c_str(),"total_power_stampfile")==0 ){
                     gLastFileStampfile = envvar.szValue.c_str();
                  }
                  if( strcmp(envvar.szName.c_str(),"start_from_last")==0 ){
                     gStartFromLast = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"show_max_power")==0 ){
                     gShowMaxPower = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"unit")==0 ){
                     if( strcasecmp(envvar.szValue.c_str(),"dbm") == 0 ){
                        gOutUnit = eOutUnitDBM;
                     }
                     if( strcasecmp(envvar.szValue.c_str(),"mW") == 0 ){
                        gOutUnit = eOutUnitmW;
                     }
                     if( strcasecmp(envvar.szValue.c_str(),"bedlam") == 0 || strcasecmp(envvar.szValue.c_str(),"raw") == 0 ){
                        gOutUnit = eOutUnitBedlamPower;
                     }
                  }
                  if( strcmp(envvar.szName.c_str(),"exclude_n_birghtest")==0 ){
                     gExcludeNBrightestChannels = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"kurtosis")==0 ){
                     gCalcKurtosis = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"fit_pol_n")==0 ){
                     gFitPolN = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"fit_pol_n_min_freq")==0 ){
                     gFitPolN_MinFreq = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"fit_pol_n_max_freq")==0 ){
                     gFitPolN_MaxFreq = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"mean_total_power")==0 ){
                     gCalcMeanTotalPower = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"write_now")==0 ){
                     gWriteNow = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"sql")==0 ){
                     gSQL = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"orbcomm")==0 ){
                     gORBCOMM = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"do_sqr")==0 ){
                     gDoSQR = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"total_power_stampfile_intcol")==0 ){
                      gTimeStampIntCol = atol(envvar.szValue.c_str());
                  }
                  if( strcmp(envvar.szName.c_str(),"total_power_stampfile_uxcol")==0 ){
                      gTimeStampUxTimeCol = atol(envvar.szValue.c_str());
                  }                  
                  if( strcmp(envvar.szName.c_str(),"total_power_cut_range")==0 ){
                     MyParser szCut = envvar.szValue.c_str();
                     CMyStrTable items;
                     szCut.GetItems(items);
                     if( items.size() != 3 ){
                        printf("ERROR : could not parse total_power_cut_range option = |%s| number of items %d != 3\n",envvar.szValue.c_str(),(int)items.size());
                        exit(-1);
                     }
                     cTotPowerCut cut;
                     cut.min_uxtime = atof(items[0].c_str());
                     cut.max_uxtime = atof(items[1].c_str());
                     cut.threshold = atof(items[2].c_str());
                     gTotalPowerThreshRanges.push_back(cut);
                  }                                                                                                                                                                                                                                                                                                                        
                  if( strcmp(envvar.szName.c_str(),"int_time_stamp_file")==0 ){
                     gIntTimeStampFile = envvar.szValue.c_str();
                  }

               }
            }
            break;   
         case 'h':
            usage();
            break;
         case 'd':
            gDebug++;
            break;
         default:  
            fprintf(stderr,"Unknown option %c\n",opt);
            usage();
      }
   }

   change_ext(out_file.c_str(),"last_file",gLastFileStampfile);
}
                                                        
                                                                 
       
                                                                                                                  

int main(int argc,char* argv[])
{
  string list="fits_list";
  if( argc >= 2 ){
     list = argv[1];
  }

  // parse command line :
  parse_cmdline(argc,argv);
  
  vector<string> fits_list;

  if( strstr(list.c_str(),".fits") ){
     fits_list.push_back( list.c_str() );
     if( strlen(out_file.c_str()) == 0 ){
        change_ext( list.c_str(), "tpwr", out_file );
     }
  }else{
     if( bg_read_list(list.c_str(),fits_list) <= 0 ){
        printf("ERROR : could not read list file %s\n",list.c_str());
        exit(-1);
     }else{
        for(int i=0;i<fits_list.size();i++){
           printf("%i %s\n",i,fits_list[i].c_str());
        }
     }
  }

  // if output file name not set earlier :
  if( strlen(out_file.c_str()) == 0 ){
     out_file="totalpower_vs_time.txt";
  }
  
//  gStartFreq
  CBgFits first_fits( fits_list[0].c_str() );
  if( fits_list.size() > 0 ){
     if( first_fits.ReadFits( fits_list[0].c_str() ) ){
        printf("ERROR : could not read first fits file %s\n",fits_list[0].c_str());
        exit(-1);
     }                         
  }
  if( gStartFreq > 0 ){
     gStartChannel = first_fits.freq2ch( gStartFreq );
     printf("Start freq = %.2f MHz -> start channel = %d\n",gStartFreq,gStartChannel);
  }
  if( gEndFreq > 0 ){
     gEndChannel = first_fits.freq2ch( gEndFreq );
     printf("End   freq = %.2f MHz -> end channel = %d\n",gEndFreq,gEndChannel);
  }

//string gIntTimeStampFile;
// CValueVector gIntTimeStamps;  
  if( strlen(gIntTimeStampFile.c_str()) > 0 ){
     int int_col=gTimeStampIntCol,time_col=gTimeStampUxTimeCol; // for MEDIAN.txt file
     gIntTimeStamps.read_file( gIntTimeStampFile.c_str(), 0, int_col, time_col );
     printf("Read %d integrations time stmaps from file %s\n",int(gIntTimeStamps.size()),gIntTimeStampFile.c_str());
  }
  
  print_parameters();

  if( fits_list.size() > 0 ){
     double start_fits_time = first_fits.dtime_fs + ((double)first_fits.dtime_fu)/1000000.00;
     if( gEndChannel < 0 ){
        gEndChannel = first_fits.GetYSize()-1;
        printf("End channel set to %d - based on fits files dimensions\n",gEndChannel);
     }

     long double avg_total_sum=0.00;
     int int_count=0;
     int total_processed=0,total_skipped=0,total_accepted=0;
     CTotalPowerList total_power_list;

     int start_idx=0;
     if( gStartFromLast > 0 ){
        if( strlen(gLastProcessed.c_str()) <= 0 ){
           // check file contents if not overwritten by parameters :
           if( MyFile::DoesFileExist(gLastFileStampfile.c_str()) ){
              vector<string> stamp_file_info;           
              if( bg_read_list(gLastFileStampfile.c_str(),stamp_file_info) > 0 ){
                 if( stamp_file_info.size() > 0 ){
                    gLastProcessed = stamp_file_info[0].c_str();
                    printf("Last processed file = %s (read from %s)\n",gLastProcessed.c_str(),gLastFileStampfile.c_str());
                 }
              }
           }                                                                                              
        }
        if( strlen(gLastProcessed.c_str())>0 ){
           printf("Last processed file specified = %s\n",gLastProcessed.c_str());
           for(int i=0;i<fits_list.size();i++){
              if( strstr( fits_list[i].c_str() , gLastProcessed.c_str() )  ){
                 printf("Last processed file %s identified at index = %d\n",gLastProcessed.c_str(),i);
                 start_idx= i + 1;
                 break;
              }
           }
        } 
     }        
     
     printf("Opening file %s ...",out_file.c_str());fflush(stdout);
     FILE* outf = fopen(out_file.c_str(),szOutOption.c_str());
     if( !outf ){
        printf("ERROR : could not open file %s for writing -> exiting now\n",out_file.c_str());
        exit(-1);
     }
     if( gWriteNow > 0 ){
        if( gSQL > 0 ){
           // nothing special - for now there are INSERTS 
        }else{
           if( gOutColumns == eOutColumnsAll ){
              fprintf(outf,"# SECOND_SINCE_START UXTIME TOTAL_POWER FITS FITS_INT_IDX STATE RMS_LOCAL AVG_LOCAL TMP_VALUE MAX_POWER_SINGLE_CHANNEL MAX_POWER_SINGLE_CHANNEL[dBm] MAX_POWER_FREQ LOCAL_MEDIAN LOCAL_SIGMA_IQR RMS_TOTAL_POWER KURTOSIS FIT_CHI2\n");
           }else{
              if( gShowMaxPower > 0 ){
                 fprintf(outf,"# UXTIME TOTAL_POWER MAX_POWER_FREQ MAX_POWER\n");
              }else{
                 fprintf(outf,"# UXTIME TOTAL_POWER\n");
              }                                                            
           }           
        }
     }
        
     printf("Starting processing from index = %d\n",start_idx);
     for(int i=start_idx;i<fits_list.size();i++){
        printf("-------------------------------------------------------------- %s --------------------------------------------------------------\n",fits_list[i].c_str());
        CBgFits fits( fits_list[i].c_str() );     
        
        // acc5859_ch0_20170217_022101_099522.fits
        time_t file_start_uxtime = 0; 
  
        if( fits.ReadFits( fits_list[i].c_str() ) ){
           printf("ERROR : could not read fits file %s on the list -> skipped\n",fits_list[i].c_str());           
        }else{
            if( gWriteNow > 0 ){
               if (gSQL > 0 ){
                  file_start_uxtime = filename2ux( fits_list[i].c_str() );                  
                  fprintf(outf,"INSERT INTO DataFile (start_uxtime,file_name) VALUES (%ld,'%s');\n",file_start_uxtime,fits_list[i].c_str() );
               }
            }
                                                                                
        
           int fits_processed=0,fits_skipped=0,fits_accepted=0;
           printf("OK : fits file %s read ok\n",fits_list[i].c_str());
           int xSize = fits.GetXSize();
           int ySize = fits.GetYSize();
            
           double fits_time = fits.dtime_fs + ((double)fits.dtime_fu)/1000000.00;          
           double t = fits_time - start_fits_time;
           int end_x=xSize;
           if( gMaxX > 0 ){
              end_x = gMaxX;
           }

           if( gDebug > 0 ){
              printf("DEBUG : dumping total_power of columns in range %d - %d\n",gMinX,end_x);
           }
           
           for(int x=gMinX;x<end_x;x++){
              fits_processed++;
              int counter=0;
              long double total_sum=0.00,total_sum2=0.00,kurtosis=0.00;;
              long double total_sum_bedlam=0.00;
              double t_int = fits_time + fits.inttime*x;
              if( gIntTimeStamps.size() > 0 ){
                 t_int = gIntTimeStamps.get_uxtime_fast(x);
                 if( t_int <= 0 ){
                    printf("WARNING : could not find unixtime for integration %d\n",x);
                 }
              }
              
              int end=gEndChannel;
              if( (ySize-1) < end ){
                 end = (ySize-1);
              }

              double maxTotalPowerByExclusion=-1;
              if( gExcludeNBrightestChannels>0 ){
                 int size_array = end-gStartChannel+1;
                 double* power_array = new double[size_array];
                 memset(power_array,'\0',sizeof(double)*size_array);
                 for(int y=gStartChannel;y<=end;y++){
                    double freq = fits.ch2freq(y);
                    if( gExcludedFreqRanges.size() > 0 ){
                       if( gExcludedFreqRanges.IsFreqExcluded(freq) > 0 ){
                          if( gDebug > 3 ){
                             printf("Channel %d / %.2f MHz excluded !\n",x,freq);
                          }
                          continue;
                       }
                    }
                 
                    double power_bedlam = fits.valXY(x,y);
                    if( gDoSQR > 0 ){
                       power_bedlam = power_bedlam*power_bedlam;
                    }
                    power_array[x-gStartChannel] = power_bedlam;                    
                 }                                  
                 my_sort_float(power_array,size_array);
                 maxTotalPowerByExclusion = power_array[size_array-gExcludeNBrightestChannels-1];
                 delete [] power_array;
              }

              // calculate mean (for kurtosis)
              double mu_mean = 0.00;
              int mu_mean_cnt=0;
              
              if( gCalcKurtosis >0 ){
                 for(int y=gStartChannel;y<=end;y++){
                    double freq = fits.ch2freq(y);
                    if( gExcludedFreqRanges.size() > 0 ){
                       if( gExcludedFreqRanges.IsFreqExcluded(freq) > 0 ){
                          if( gDebug > 3 ){
                             printf("Channel %d / %.2f MHz excluded !\n",y,freq);
                          }
                          continue;
                       }
                    }
                 
                    double power_bedlam = fits.valXY(x,y);
                    if( gDoSQR > 0 ){
                       power_bedlam = power_bedlam*power_bedlam;
                    }
                    double power_in_units = power_bedlam;
                 
                    if( power_bedlam > maxTotalPowerByExclusion && maxTotalPowerByExclusion>0 ){
                       // if required to skip N brightest channels 
                       continue;
                    }                 
                    if( gOutUnit != eOutUnitBedlamPower ){
                       power_in_units = CBedlamSpectrometer::power2mW( freq, power_bedlam );
                    }
                    
                    mu_mean += power_in_units;
                    mu_mean_cnt++;
                 }
                 
                 if( mu_mean_cnt > 0 ){
                    mu_mean = mu_mean / mu_mean_cnt;
                 }                 
              }
              
              double fit_chi2 = 0.00;
              if( gFitPolN >= 0 ){
/*                 double A,B;
                 fit_chi2 = fits.FitPoly( y, fits.ch2freq(gStartChannel), fits.ch2freq(end), A, B );
                 if( gDebug >= 2 ){
                    printf("FitPoly : %s %d %.8f %.8f %.8f\n",fits.GetFileName(),y,A,B,fit_chi2);
                 }*/
                 printf("ERROR : this option is not implemented in this version\n");
                 exit(-1);
              }

              
              double maxChannelPower=-1,maxChannelFreq=-1;
              int channel_count=0;
              for(int y=gStartChannel;y<=end;y++){
                 double freq = fits.ch2freq(y);
                 if( gExcludedFreqRanges.size() > 0 ){
                    if( gExcludedFreqRanges.IsFreqExcluded(freq) > 0 ){
                       if( gDebug > 3 ){
                          printf("Channel %d / %.2f MHz excluded !\n",y,freq);
                       }
                       continue;
                    }
                 }
                 
                 double power_bedlam = fits.valXY(x,y);
//                 printf("DEBUG_FITS = %.8f\n",power_bedlam);
                 if( gDoSQR > 0 ){
                    power_bedlam = power_bedlam*power_bedlam;
                 }
                 double power_in_units = power_bedlam;
                 
                 if( power_bedlam > maxTotalPowerByExclusion && maxTotalPowerByExclusion>0 ){
                    // if required to skip N brightest channels 
                    continue;
                 }
                 
                 if( gOutUnit != eOutUnitBedlamPower ){
                    power_in_units = CBedlamSpectrometer::power2mW( freq, power_bedlam );
//                    printf("DEBUG0 - mW ???\n");
                 }                                 
                 total_sum += power_in_units;
//                 printf("DEBUG1 (x,y) = (%d,%d) : %.8f -> %.8f (units = %d)\n",x,y,double(power_in_units),double(total_sum),gOutUnit);
                 total_sum2 += power_in_units*power_in_units;
                 long double kdiff = (power_in_units - mu_mean);
                 kurtosis += kdiff*kdiff*kdiff*kdiff;
                 counter++;
                 total_sum_bedlam += power_bedlam;
                 channel_count++;
                 
                 if( freq <= BIGHORNS_MAX_FREQ_MHZ ){
                    if( power_bedlam > maxChannelPower ){
                       maxChannelPower = power_bedlam;
                       maxChannelFreq  = freq;
                    }
                 }
              }
//              printf("DEBUG : %.8lf\n",total_sum_bedlam);
              
              double orbcomm_sum=0;
              if( gORBCOMM > 0 ){
                 for(int y=0;y<=ySize;y++){
                    double freq = fits.ch2freq(y);
                    
                    if( freq>=137.1 && freq<=138.05 ){
                       double power_bedlam = fits.valXY(x,y);
                       if( gDoSQR > 0 ){
                          power_bedlam = power_bedlam*power_bedlam;
                       }
                       orbcomm_sum += power_bedlam;
                    }
                 }                                  
              }
              
              if( gCalcMeanTotalPower > 0 ){
                 total_sum = total_sum / channel_count;
              }

              int state=-1;
              if( fits.GetRangesCount() > 0 ){
                 int idx;
                 cIntRange* pRange = fits.GetRange(x,idx);

                 if( pRange ){
                    state = pRange->inttype;
                 }

                 if( strlen(gState.c_str()) ){                 
                    if( !pRange || !strstr(pRange->m_szName.c_str(),gState.c_str()) ){
                       fits_skipped++;
                       if( gDebug > 1 ){
                          if( pRange ){
                             printf("Integration %d of type %s skipped, does not match %s\n",x,pRange->m_szName.c_str(),gState.c_str());
                          }else{
                             printf("Integration %d, not defined type !\n",x);
                          }
                       }
                       continue;
                    }
                 }                 
              } // end of loop over channels 
              
              if( gMinPower > -1e12 ){
                 if( total_sum_bedlam < gMinPower )
                    continue;
              }
              if( gMaxPower > 0 ){
                 if( total_sum_bedlam > gMaxPower )
                    continue;
              }
              
              if( gStartUX > 0 ){
                 if( t_int < gStartUX )
                    continue;
              }
              if( gEndUX > 0 ){
                 if( t_int > gEndUX )
                    continue;
              }

              // KURTOSIS:
              kurtosis = kurtosis/((long double)counter);
              long double rms = sqrt( (total_sum2/counter) - (total_sum/counter)*(total_sum/counter) );
              kurtosis = (kurtosis/(rms*rms*rms*rms) - 3 ); // no -3 
              
              cTotalPowerInfo tpwr_info;
              tpwr_info.total_sum = total_sum;
              tpwr_info.rms_total_sum = (double)rms;
              tpwr_info.total_sum_bedlam = total_sum_bedlam;
              if( gOutUnit == eOutUnitDBM ){
                 tpwr_info.total_sum = mW2dbm( total_sum );
              }
              
              tpwr_info.kurtosis  = kurtosis;
              tpwr_info.t_int     = t_int;
              tpwr_info.t         = t;             
              tpwr_info.fits_file = fits_list[i].c_str();
              tpwr_info.ok        = state;   
              tpwr_info.fits_int  = x;
              tpwr_info.rms_local = -1;
              tpwr_info.maxPowerSingleChannel = maxChannelPower;
              tpwr_info.maxPowerFreq = maxChannelFreq;
              tpwr_info.fit_chi2 = fit_chi2;
              tpwr_info.orbcommPower = orbcomm_sum;

//              fprintf(outf,"%.8f %.20f %.20Lf %s %d %d\n",t,t_int,total_sum,fits_list[i].c_str(),y,state);
              if( gWriteNow > 0 ){
                 // fprintf(outf,"# SECOND_SINCE_START UXTIME TOTAL_POWER FITS FITS_INT_IDX STATE RMS_LOCAL AVG_LOCAL TMP_VALUE MAX_POWER_SINGLE_CHANNEL MAX_POWER_SINGLE_CHANNEL[dBm] MAX_POWER_FREQ LOCAL_MEDIAN LOCAL_SIGMA_IQR RMS_TOTAL_POWER KURTOSIS FIT_CHI2\n");
                 double power_dbm = mW2dbm( tpwr_info.maxPowerSingleChannel/CBedlamSpectrometer::spectrum_response_model(tpwr_info.maxPowerFreq) );
                 double local_sigma_iqr = 0.00, local_median = 0.00;
                 if (gSQL > 0 ){
                    fprintf(outf,"INSERT INTO TotalPower (file_start_uxtime,integration,unixtime,total_power,total_power_orbcomm,total_power_max,total_power_max_freq) VALUES (%ld,%d,%.4f,%.4Lf,%.4f,%.4f,%.4f);\n",
                       file_start_uxtime, tpwr_info.fits_int, tpwr_info.t_int, tpwr_info.total_sum, tpwr_info.orbcommPower, tpwr_info.maxPowerSingleChannel, tpwr_info.maxPowerFreq );
                 }else{
                    if( gOutColumns == eOutColumnsAll ){
                       fprintf(outf,"%.8f %.20f %.20Lf %s %d %d %.8f %.8f %d %e %.3f %.3f %.20f %.20f %.20Lf %.8f %.8f\n",
                              tpwr_info.t,
                              tpwr_info.t_int,
                              tpwr_info.total_sum,
                              tpwr_info.fits_file.c_str(),
                              tpwr_info.fits_int,
                              tpwr_info.ok,
                              tpwr_info.rms_local,
                              tpwr_info.avg_local,tpwr_info.tmp_value,tpwr_info.maxPowerSingleChannel,power_dbm,tpwr_info.maxPowerFreq,local_median,local_sigma_iqr,tpwr_info.rms_total_sum,tpwr_info.kurtosis,tpwr_info.fit_chi2);
                    }else{
                       double max_power_out = tpwr_info.maxPowerSingleChannel;
                       if( gOutUnit == eOutUnitDBM ){
                          max_power_out = mW2dbm( tpwr_info.maxPowerSingleChannel/CBedlamSpectrometer::spectrum_response_model(tpwr_info.maxPowerFreq) );
                       }
                       if( gShowMaxPower > 0 ){
                          fprintf(outf,"%.20f %.20Lf %.20f %.20f\n",tpwr_info.t_int,tpwr_info.total_sum,tpwr_info.maxPowerFreq,max_power_out);
                       }else{
                          fprintf(outf,"%.20f %.20Lf\n",tpwr_info.t_int,tpwr_info.total_sum);
                       }
                    }
                 }
              }else{
                 // may require a lot of memory ...
                 total_power_list.push_back(tpwr_info);
              }
              t += fits.inttime;
              
              avg_total_sum += total_sum;
              int_count++;
              fits_accepted++;
           } 

           total_accepted += fits_accepted;
           total_skipped += fits_skipped;
           total_processed += fits_processed;
           
           printf("Fits statistics :\n");
           printf("-----------------\n");
           printf("\tAccepted = %d / %d\n",fits_accepted,fits_processed);           
           printf("\tSkipped  = %d\n",fits_skipped);
           printf("\n");
           printf("Total statistics :\n");
           printf("-----------------\n");
           printf("\tAccepted = %d / %d\n",total_accepted,total_processed);
           printf("\tSkipped  = %d\n",total_skipped);
        
        
           gLastProcessed = fits_list[i].c_str();
        }     
     }

     char szLastFileStampfile_tmp[1024];
     sprintf(szLastFileStampfile_tmp,"%s.tmp",gLastFileStampfile.c_str());     
     MyOFile out_last_processed(szLastFileStampfile_tmp,"w");
     out_last_processed.Printf("%s\n",gLastProcessed.c_str());
     out_last_processed.Close();
     
     int test_file_size = MyFile::GetFileSize( szLastFileStampfile_tmp );     
     printf("Size of test file %s = %d bytes\n",szLastFileStampfile_tmp,test_file_size);
     if( test_file_size > 5 ){
        // file looks ok so there seem to be enough space on device :
        char szCmd[2048];
        sprintf(szCmd,"mv %s %s",szLastFileStampfile_tmp,gLastFileStampfile.c_str());        
        int ret = system(szCmd);
        printf("Executed command : %s , returned %d\n",szCmd,ret);
     }else{
        printf("ERROR : could not write %s file to disk (it is < 5 bytes !) -> probably no space on device !!!\n",szLastFileStampfile_tmp);
     }
  
     // calculates local rms-es 
     if( gWriteNow <= 0 ){
         // if writing output file at the very end (DEFAULT):
        printf("\n\n");
        printf("--------------------------------------------------- GLOBAL STATISTICS ---------------------------------------------------\n");fflush(stdout);
        calc_rmses(total_power_list,first_fits.inttime);
     
        CMyProgressBar bar(0,total_power_list.size());     
        if( gOutColumns == eOutColumnsAll ){
           fprintf(outf,"# SECOND_SINCE_START UXTIME TOTAL_POWER FITS FITS_INT_IDX STATE RMS_LOCAL AVG_LOCAL TMP_VALUE MAX_POWER_SINGLE_CHANNEL MAX_POWER_SINGLE_CHANNEL[dBm] MAX_POWER_FREQ LOCAL_MEDIAN LOCAL_SIGMA_IQR RMS_TOTAL_POWER KURTOSIS FIT_CHI2\n");
        }else{
           if( gShowMaxPower > 0 ){
              fprintf(outf,"# UXTIME TOTAL_POWER MAX_POWER_FREQ MAX_POWER\n");
           }else{
              fprintf(outf,"# UXTIME TOTAL_POWER\n");
           }
        }
        for(int i=0;i<total_power_list.size();i++){
           cTotalPowerInfo& tpwr_info = total_power_list[i];
        
           if( gOutColumns == eOutColumnsAll ){
              double local_median=tpwr_info.total_sum,local_sigma_iqr=0.00;
              if( gCalcMedianSigmaNSecAround > 0 ){
                 double max_total_power = GetTotalPowerThreshold(tpwr_info.t_int);
                 local_median = total_power_list.GetLocalMedianBothSides( tpwr_info.t_int, gCalcMedianSigmaNSecAround, local_sigma_iqr, max_total_power );
              }
        
              double power_dbm = mW2dbm( tpwr_info.maxPowerSingleChannel/CBedlamSpectrometer::spectrum_response_model(tpwr_info.maxPowerFreq) );
              fprintf(outf,"%.8f %.20f %.20Lf %s %d %d %.8f %.8f %d %e %.3f %.3f %.20f %.20f %.20Lf %.8f %.8f\n",tpwr_info.t,tpwr_info.t_int,tpwr_info.total_sum,tpwr_info.fits_file.c_str(),tpwr_info.fits_int,tpwr_info.ok,
                           tpwr_info.rms_local,tpwr_info.avg_local,tpwr_info.tmp_value,tpwr_info.maxPowerSingleChannel,power_dbm,tpwr_info.maxPowerFreq,local_median,local_sigma_iqr,tpwr_info.rms_total_sum,tpwr_info.kurtosis,tpwr_info.fit_chi2);
           }else{
              double max_power_out = tpwr_info.maxPowerSingleChannel;
              if( gOutUnit == eOutUnitDBM ){
                 max_power_out = mW2dbm( tpwr_info.maxPowerSingleChannel/CBedlamSpectrometer::spectrum_response_model(tpwr_info.maxPowerFreq) );
              }
              if( gShowMaxPower > 0 ){
                 fprintf(outf,"%.20f %.20Lf %.20f %.20f\n",tpwr_info.t_int,tpwr_info.total_sum,tpwr_info.maxPowerFreq,max_power_out);
              }else{
                 fprintf(outf,"%.20f %.20Lf\n",tpwr_info.t_int,tpwr_info.total_sum);
              }
           }
        
        
           bar.Update(i);
        }
     }else{ 
        printf("WARNING : no global statistics calculated as output file is written as analysis go (gWriteNow=%d) -> set -q write_now=0 if need to change it\n",gWriteNow);
     }
     fclose(outf);

     avg_total_sum = (avg_total_sum/int_count);
     printf("Average total sum = %.20Lf\n",avg_total_sum);    
  }
     
}

double calc_rmses(vector<cTotalPowerInfo>& total_power_list, double inttime )
{
   int i=0;
   double sum=0;
   double sum2=0.00;
   int count=0.00;
   double total_sum=0;
   double total_sum2=0;
   int total_count=0;
   cTotalPowerInfo* prev = NULL;
   int start_int=-1;
   
   if( total_power_list.size() <= 1 ){
      return 0.00;
   }
   
   while(i<total_power_list.size()){
     cTotalPowerInfo& curr = total_power_list[i];
   
     if( prev && ( abs(curr.t_int-prev->t_int)>2*inttime || (gMaxRMSChunk>0 && abs(i-start_int)>=gMaxRMSChunk) ) ){
        // change of fits file -> calculate local RMS 
        curr.avg_local = (sum/count);
        double avg2 = (sum2/count);
        curr.rms_local = sqrt(avg2-curr.avg_local*curr.avg_local);
        curr.tmp_value = count;
        total_sum += curr.rms_local;
        total_sum2 += curr.rms_local*curr.rms_local;
        total_count++;
        
        // new chunk started :
        sum=0.00;
        sum2=0.00;
        count=0;
        start_int=i;
     }
     
     sum += curr.total_sum;
     sum2 += curr.total_sum*curr.total_sum;
     count++;

     prev = &curr;
     i++;
   }

   double rms_avg = sqrt((sum2/count)-(sum/count)*(sum/count));
   double rms_rms = 0.00;
   
   if( total_count > 0 ){
      rms_avg = total_sum/total_count;
      rms_rms = sqrt(total_sum2/total_count - rms_avg*rms_avg);
   }

   printf("<RMS_local> = %.8f , RMS of RMS_local = %.8f using %d detected stays on given state\n",rms_avg,rms_rms,total_count);
   
   return rms_avg;
}
