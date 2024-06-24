/** @file  PciAcqPX4.cpp
    @brief PX1500 PCI Acquisition Recording Utility

    This application demonstrates how to do a PX1500 PCI acquisition
    recording with a single PX1500 device. 
*/
// TESTS:
// -n TESTED on page 6 of /home/msok/Desktop/EDA/loogbook/Pulsar_Observations_VCS/obs_vcs_0437.odt
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>

#include <bg_globals.h>

// std :
#include <string>
using namespace std;

#include "sighorns.h"
#include "spectrometer.h"
// #include "bedlam.h"

// A quick rough equivalent to a certain other platform's GetTickCount
static void usage();
static void parse_cmdline(int argc, char * argv[]);

string filename;
int gBedlamFormat=0;
string gOutFile="out.txt";
int gVerb=0;
int gSkipNFirst=-1;
int gNormalize=1;
string gOutBinFile="out.bin";
string gOutBinFloatFile="out_float.bin";
string gOutputPowerFile="";
string gOutputPowerFits="";
int gOutCoarseChannel=109;
int gDumpNFineChannels=N_FINE_CH_PER_BAND;
int gSkipExtra=0;
int gPfbTaps=0;
double gFileStartUxTime=0;
int gOutputAll=0;
int gNoFitsFile=0;
long int g_infile_size_bytes = -1;
int gNoBinaryFile=0;

void print_parameters()
{
   printf("##############################################\n");
   printf("PARAMETERS:\n");
   printf("##############################################\n");
   printf("Input binary file (voltage samples) = %s (size = %ld bytes)\n",filename.c_str(),g_infile_size_bytes);
   printf("Output all files            = %d (except FITS = %d)\n",gOutputAll,gNoFitsFile);
   printf("Output binary file          = %s\n",gOutBinFile.c_str());
   printf("Output float binary file    = %s\n",gOutBinFloatFile.c_str());   
   printf("Input file start uxtime     = %.2f\n",gFileStartUxTime);
   printf("Output START coarse channel = %d\n",gOutCoarseChannel);
   printf("# fine channels to dump     = %d\n",gDumpNFineChannels);
   printf("All output files            = %d\n",gOutputAll);
   printf("Output power file           = %s\n",gOutputPowerFile.c_str());
   printf("Output power fits           = %s\n",gOutputPowerFits.c_str());
   printf("Skip extra                  = %d\n",gSkipExtra);
   printf("PFB taps                    = %d\n",gPfbTaps);
   printf("PFB coefficients file       = %s\n",CSpectrometer::gPfbCoeffFile.c_str());
   printf("Dump channel                = %d\n",CSpectrometer::m_DumpChannel);
   printf("Polarisations in file       = %d\n",CSpectrometer::m_PolsInFile);
   printf("Polarisation to analyse     = %d (only makes sense when 2 pols in file)\n",CSpectrometer::m_Pol);
   printf("Number of bits              = %d\n",CSpectrometer::m_nBits);
   printf("Maximum number of bytes to process = %d\n",CSpectrometer::m_MaxBytesToProcess);
   printf("Voltage samples txt file    = %s\n",CSpectrometer::m_szVoltageDumpFile.c_str());
   printf("No binary files (only fits) = %d\n",gNoBinaryFile);
   printf("##############################################\n");

}

int main(int argc, char* argv[])
{
  if( argc<=1 || strncmp(argv[1],"-h",2)==0 ){
     usage();
  }

  filename = argv[1];

  parse_cmdline( argc , argv );
  print_parameters();

  double acc_spectrum[N_SAMPLES];
  int nintegr=0;
  CSpectrometer spectrometer;
//  if( gBedlamFormat > 0 ){
//     nintegr = CBedlamSpectrometer::process_voltages( filename.c_str(), acc_spectrum, gSkipNFirst, 1, 1e9, 0, -1, 0, gVerb );
//  }else{
     if( gPfbTaps <= 0 ){
        nintegr = spectrometer.fileFFT( filename.c_str(), acc_spectrum, gOutBinFile.c_str(), gOutCoarseChannel, gSkipExtra, gOutputPowerFile.c_str(), gOutputPowerFits.c_str(), gDumpNFineChannels, (time_t)gFileStartUxTime, g_infile_size_bytes, gOutBinFloatFile.c_str() );
     }else{
        nintegr = spectrometer.filePFB( filename.c_str(), acc_spectrum, gOutBinFile.c_str(), gOutCoarseChannel, gSkipExtra, gPfbTaps );
     }
//  }
  printf("Accumulated bedlam spectrum of %d integrations :\n",nintegr);

  if( gNormalize > 0 ){
     for(int i=0;i<N_CHANNELS;i++){
        acc_spectrum[i] = acc_spectrum[i] / nintegr;
     }
  }
  
  FILE* outf = NULL;
  if(strlen(gOutFile.c_str())>0){
     outf = fopen(gOutFile.c_str(),"w");
  }
  for(int i=0;i<N_CHANNELS;i++){     
     if( gVerb>0 ){
        printf("%d %e\n",i,acc_spectrum[i]);
     }

     if( outf ){
        fprintf(outf,"%d %e\n",i,acc_spectrum[i]);           
     }
  }
  fclose(outf);


  return 0;
}

void usage()
{
   printf("eda_spectrometer FILE.dat -b -o OUTFILE.txt -v -e OUTBINFILE.dat -t PFB_TAPS -p PFB_COEF_FITS -s SAVE_CHANNEL -a OUTPUT_POWER_FILE.bin -f OUTPUT_POWER_FITS.fits -w NUMBER_OF_FINE_CH -z -y FILE_SIZE_BYTES -u UNIXTIME_OF_FILE_START -g -k POLARISATION -K polarisations_in_file\n");   
   printf("-b : binary file is in BEDLAM voltages format with a unixtime stamp\n");
   printf("-o OUTFILE : output file\n");
   printf("-v : increases verbosity level\n");
   printf("-e OUTBINFILE.dat : name of output binary file\n");
   printf("-c COARSE_CHANNEL : coarse channel number to output to binary file\n");
   printf("-x SKIP_EXTRA : skip extra number of samples [default 0]\n");
   printf("-t PFB_TAPS : default 0 - simple FFT is used not PFB\n");
   printf("-p PFB_COEF_FITS : fits file with PFB coefficients\n");
   printf("-s CHANNEL : save time series in channel\n");
   printf("-a OUTPUT_POWER_FILE.bin : for 0437 and other pulsars (for pulsar team) - binary FLOAT\n");
   printf("-f OUTPUT_POWER_FITS.fits : for 0437 and other pulsars (for pulsar team) - FITS FILE (FLOAT) for testing/viewing\n");
   printf("-u UNIXTIME_OF_FILE_START : unixtime of file start\n");
   printf("-y FILE_SIZE_BYTES : file size in bytes [default %d -> automatic detection some times does not work well]\n",(int)g_infile_size_bytes);
   printf("-w NUMBER_OF_FINE_CH : number of fine channels to dump [default %d]\n",N_FINE_CH_PER_BAND);
   printf("-z : to output all the files (.fft , .bin (re/im of FFT) , _MAG.bin (power of FFT) , _MAG.fits (power of FFT in FITS)\n");   
   printf("-k POL_IDX : polarisation to look at, only makes sense when -K 2 (or >1) [default %d]\n",CSpectrometer::m_Pol);
   printf("-K POLS_IN_FILE : number of polarisations in file [default %d]\n",CSpectrometer::m_PolsInFile);
   printf("-n NUMBER OF BITS [default 8 - full bytes]\n");
   printf("-m MAX_NUMBER_OF_BYTES_TO_PROCESS : maximum number of bytes to process if <=0 -> ALL [default %d - means ALL]\n",CSpectrometer::m_MaxBytesToProcess);
   printf("-d SAMPLES_OUT_TXT_FILE : name of file to dump raw voltage samples [default not specified = disabled]\n");
   printf("-l : no binary files (just fits files)\n");
   printf("-g : no FITS files (disable generation of FITS file)\n");
   printf("-G GEO_CORR_SIGN : sign of geometrical correction. It also enable Geo-Correction when != 0 [default %d]\n",CSpectrometer::m_GeometryCorrection);
   
   exit(-1);
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "lzvhbo:e:c:x:t:p:s:a:f:w:u:y:gk:K:n:m:d:G:";
   int opt;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
      switch (opt) {
         case 'b':
            gBedlamFormat=1;
            break;
            
         case 'G':
            CSpectrometer::m_GeometryCorrection = atol( optarg );
            break;            
            
         case 'h':
            usage();
            break;
         case 'o':
            if( optarg ){
               gOutFile=optarg;
            }
            break;

         case 'x':
            if( optarg ){
               gSkipExtra = atol(optarg);
            }
            break;
            
         case 'e':
            if( optarg ){
               gOutBinFile=optarg;
            }
            break;

         case 'c':
            if( optarg ){
               gOutCoarseChannel = atol(optarg);
            }
            break;

         case 'd':
            if( optarg ){
               CSpectrometer::m_szVoltageDumpFile = optarg;
            }
            break;

         case 's':
            if( optarg ){
               CSpectrometer::m_DumpChannel = atol(optarg);
            }
            break;
            
         case 't':
            if( optarg ){
               gPfbTaps = atol(optarg);
            }
            break;

         case 'p':
            if( optarg ){
               CSpectrometer::gPfbCoeffFile = optarg;
            }
            break;

         case 'k':
            if( optarg ){
               CSpectrometer::m_Pol = atol(optarg);            
            }
            break;

         case 'K':
            if( optarg ){
               CSpectrometer::m_PolsInFile = atol(optarg);            
            }
            break;

         case 'n':
            if( optarg ){
               CSpectrometer::m_nBits = atol(optarg);            
            }
            break;

         case 'm':
            if( optarg ){
               CSpectrometer::m_MaxBytesToProcess = atol(optarg);            
            }
            break;

         case 'a':
            if( optarg ){
               gOutputPowerFile = optarg;
            }
            break;
            
         case 'f':
            if( optarg ){
               gOutputPowerFits = optarg;
            }
            break;
            
         case 'u':
            if( optarg ){
               gFileStartUxTime = atof(optarg);
            }
            break;
            
         case 'w':
            if( optarg ){
               gDumpNFineChannels = atol(optarg);
            }
            break;

         case 'y':
            if( optarg ){
               g_infile_size_bytes = atol(optarg);
            }
            break;
            
         case 'v':
            gVerb++;
            break;

         case 'z':
            gOutputAll = 1;
            break;

         case 'g':
            gNoFitsFile = 1;
            break;

         case 'l':
            gNoBinaryFile = 1;
            break;

         default:   
            fprintf(stderr,"Unknown option %c\n",opt);
            usage();
      }
   }
   
   if( gOutputAll > 0 ){
      printf("WARNING : Required output is all types of files (it may take a lot of space)\n");
      
      add_postfix( filename.c_str(), ".fft", gOutFile );
      add_postfix( filename.c_str(), ".bin", gOutBinFile );
      add_postfix( filename.c_str(), "_float.bin", gOutBinFloatFile );
      add_postfix( filename.c_str(), "_MAG.bin", gOutputPowerFile );
      add_postfix( filename.c_str(), "_MAG_%05d.fits", gOutputPowerFits );
      
      if( gNoBinaryFile > 0 ){
         gOutBinFile = "";
         gOutputPowerFile = "";
      }
      if( gNoFitsFile > 0 ){
         gOutputPowerFits = "";
      }
   }
   
   if( CSpectrometer::m_nBits != 8 ){
      if( CSpectrometer::m_nBits != 4 ){
         printf("ERROR : option -n works currently with either 4 or 8 bits and other data formats are not supported !\n");
         exit(-1);
      }
   }
}
                               
