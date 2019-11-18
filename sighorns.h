#ifndef _SIGHORNS_H__
#define _SIGHORNS_H__

#include <stdlib.h>
#ifdef _HAS_PX1500_
#include <px1500.h>     // PX1500 library header
#else
#define PX4CHANSEL_SINGLE_CH1 0
#define HPX4 void*
typedef unsigned char px4_sample_t;
#endif
#include <string>
using namespace std;

/// PX1500 board number (serial or 1-based index) to use
#define MY_PX4400_BRD_NUM  1
#define DMA_XFER_SAMPLES   (2 * 1048576)

// FFT :
#define N_SAMPLES 65536
#define N_CHANNELS 32768 // N_SAMPLES/2 + 1 
// #define N_CHANNELS 8192

// #define N_SAMPLES 16384
// #define N_CHANNELS 8193

// void* run_packet_reader(void *args);

time_t get_dttm();

class CSigHorns
{
protected :
   HPX4 hBrd;
   px4_sample_t *dma_buf1p;
   px4_sample_t *dma_buf2p;
    
public :
  int m_bConnected;
  double dAcqRate;
  int m_MaxTransferCount;
  int m_Channel;
  string m_szBinFileBaseName;

  // processing :
  static int m_nDumpsPerBinFile;
  static int m_bFFTSamplesCount; // numer of samples used for FFT / PFB or spectrometer -> N_channels = m_bFFTSamplesCount/2 
  static int m_nAccum; // number of accumulations 
  static int gVerb;
  static int m_bDoProcessing;

  // constants , parameters , settings descriptions :
  const char* get_voltage_range_desc( int res );

  CSigHorns( double dacqrate=960.00, int max_transfer_count=10, const char* bin_fname="test_", int channel=PX4CHANSEL_SINGLE_CH1 );
  ~CSigHorns();

  // signal handling int sig)  
  static void catch_ctrlc( int sig);
  static int m_bDoStopAcq;
 

  // connect to card :  
  int Connect();
  int ConnectAndInit();
  
  // hardware initailization functions :
  int Init();
  int Arm();
  // Memory DMA :
  int AllocDMA();

  // showing settings :
  int DumpSettings();
    
  // running data acquisition :
  int RunDAQ();  

  // fast data reading :
  void* packet_reader(void *args);
  static void* run_packet_reader(void *args);
  void* consumer(void *args);
  static void* run_consumer(void *args);

  // data on-line processing :
  int doFFT( unsigned char* data_fft, int in_count, double* spectrum, int out_count );
  int ProcessSamples( px4_sample_t* data, int count, double* acc_buffer=NULL );

  // cleaning and exiting :
  int ClearAndExit( const char* szErrMsg=NULL );
  
};

#endif
