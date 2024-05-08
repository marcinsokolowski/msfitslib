#include "spectrometer.h"
#include "sighorns.h"
#include <bg_fits.h>
#include <libnova_interface.h>
#include <vector.h>

// for file size:
#include <sys/stat.h>
#include <sys/types.h>
// #include <fcntl.h>
#include <bg_fits.h>

#include <complex>

int CSpectrometer::m_DebugNSpectra=10;
string CSpectrometer::gPfbCoeffFile;
vector<double> CSpectrometer::gPfbCoeffs;  
int CSpectrometer::m_DumpChannel=-1;   
int CSpectrometer::m_PolsInFile=1;
int CSpectrometer::m_Pol=-1;
int CSpectrometer::m_nBits=8; // normal 1 byte per sample 
int CSpectrometer::m_MaxBytesToProcess=-1;
string CSpectrometer::m_szVoltageDumpFile;

double CSpectrometer::m_EDA_ElectricalLenM=140.00; // 140m of EDA electrical length (assuming BIGHORNS=0m)
int    CSpectrometer::m_GeometryCorrection=0;      // no geometry correction

CSpectrometer::CSpectrometer()
{
}

CSpectrometer::~CSpectrometer()
{
}

int CSpectrometer::doFFT( unsigned char* data_fft, int in_count, double* spectrum, double* spectrum_re, double* spectrum_im, int& out_count, double norm )
{
   double* in = (double*)malloc ( sizeof ( double ) * in_count );
   for( int i = 0; i < in_count; i++ ){
      in[i] = data_fft[i];
   }
   
   int ret = doFFT( in, in_count, spectrum, spectrum_re, spectrum_im, out_count, norm  );

   free(in);
   return ret;            
}

void CSpectrometer::fft_shift( std::complex<float>* in, int in_count, vector< std::complex<float> >& out )
{
   out.assign( in_count, 0 );
   
   int xSize = in_count;
   int center_freq_x = int( xSize/2 );
   
   int is_odd = 0;
   if ( (xSize%2) == 1 ){
      is_odd = 1;
   }
   
   for(int x=0;x<=center_freq_x;x++){ // check <= -> <
      out[center_freq_x+x] = in[x];
   }
   for(int x=(center_freq_x+is_odd);x<xSize;x++){
      out[x-(center_freq_x+is_odd)] = in[x];
   }
} 

void CSpectrometer::fft_shift( double* in, int in_count, vector<double>& out )
{
   out.assign( in_count, 0 );
   
   int xSize = in_count;
   int center_freq_x = int( xSize/2 );
   
   int is_odd = 0;
   if ( (xSize%2) == 1 ){
      is_odd = 1;
   }
   
   for(int x=0;x<=center_freq_x;x++){ // check <= -> <
      out[center_freq_x+x] = in[x];
   }
   for(int x=(center_freq_x+is_odd);x<xSize;x++){
      out[x-(center_freq_x+is_odd)] = in[x];
   }
} 

void CSpectrometer::fft_shift( vector<double> in, vector<double>& out )
{
   int xSize = in.size();
   out.assign( xSize , 0.00 );
   int center_freq_x = int( xSize/2 );
   
   int is_odd = 0;
   if ( (xSize%2) == 1 ){
      is_odd = 1;
   }
   
   for(int x=0;x<=center_freq_x;x++){ // check <= -> <
      out[center_freq_x+x] = in[x];
   }
   for(int x=(center_freq_x+is_odd);x<xSize;x++){
      out[x-(center_freq_x+is_odd)] = in[x];
   }
} 



int CSpectrometer::doFFT( std::complex<float>* in, int in_count, double* spectrum, std::complex<float>* spectrum_reim, int& out_count, double norm )    
{
/*
    Set up an array to hold the transformed data,
    get a "plan", and execute the plan to transform the IN data to
    the OUT FFT coefficients.
             */

    // WARNING : was as below 
    int nc = in_count; // WAS : ( in_count / 2 ) + 1;
                
    // fftw_plan fftw_plan_r2r_1d(int n, double *in, double *out,  fftw_r2r_kind kind, unsigned flags);         
//    double* out = (double*)malloc ( sizeof ( double ) * in_count );
//    memset(out,'\0',sizeof ( double ) * in_count );

    // output array in complex to complex FFT is the same size as the input one
    // don't believe check maths or example in https://people.sc.fsu.edu/~jburkardt/c_src/fftw_test/fftw_test.c
    fftw_complex* in_cx = (fftw_complex*)fftw_malloc( sizeof ( fftw_complex ) * in_count );                      
    fftw_complex* out_cx = (fftw_complex*)fftw_malloc( sizeof ( fftw_complex ) * in_count );
    memset(out_cx,'\0',sizeof ( fftw_complex ) * nc );
    
    for(int i=0;i<in_count;i++){
        in_cx[i][0] = in[i].real();
        in_cx[i][1] = in[i].imag();
    }
    
    int flags = 0;
    //  flags |= FFTW_EXHAUSTIVE;
    flags |= FFTW_ESTIMATE;
    fftw_plan plan_cx = fftw_plan_dft_1d( in_count, in_cx, out_cx,  FFTW_FORWARD, flags );
    fftw_execute ( plan_cx );
                                   
    for(int i=0;i<nc;i++){
       double re = out_cx[i][0] / norm;
       double im = out_cx[i][1] / norm;
//       double power = sqrt(re*re+im*im);
       double power = re*re+im*im;
       
       spectrum[i] = fabs(power);
       spectrum_reim[i] = std::complex<float>( re, im );
    }
    out_count = nc; // was nc - 1
                   
          
   // cleaning :   
   // see : http://www.fftw.org/fftw3_doc/Using-Plans.html
   fftw_destroy_plan( plan_cx );
//   fftw_cleanup(void);

   fftw_free(out_cx);
   fftw_free(in_cx);
//   free(out);

   return 1;
}

int CSpectrometer::doFFT( double* in, int in_count, double* spectrum, double* spectrum_re, double* spectrum_im, int& out_count, double norm )    
{
/*
    Set up an array to hold the transformed data,
    get a "plan", and execute the plan to transform the IN data to
    the OUT FFT coefficients.
             */
    // printf("DEBUG : norm = %.8f, in_count = %d\n",norm,in_count);             
    int nc = ( in_count / 2 ) + 1;
                
    // fftw_plan fftw_plan_r2r_1d(int n, double *in, double *out,  fftw_r2r_kind kind, unsigned flags);         
//    double* out = (double*)malloc ( sizeof ( double ) * in_count );
//    memset(out,'\0',sizeof ( double ) * in_count );
                      
    fftw_complex* out_cx = (fftw_complex*)fftw_malloc( sizeof ( fftw_complex ) * nc );
    memset(out_cx,'\0',sizeof ( fftw_complex ) * nc );
    int flags = 0;
    //  flags |= FFTW_EXHAUSTIVE;
    flags |= FFTW_ESTIMATE;
    fftw_plan plan_cx = fftw_plan_dft_r2c_1d( in_count, in, out_cx,  flags);
    fftw_execute ( plan_cx );
                                   
    for(int i=0;i<nc;i++){
       double re = out_cx[i][0] / norm;
       double im = out_cx[i][1] / norm;
//       double power = sqrt(re*re+im*im);
       double power = re*re+im*im;
       
       spectrum[i] = fabs(power);
       spectrum_re[i] = re;
       spectrum_im[i] = im;
    }
    out_count = nc -1 ;
                   
          
   // cleaning :   
   // see : http://www.fftw.org/fftw3_doc/Using-Plans.html
   fftw_destroy_plan( plan_cx );
//   fftw_cleanup(void);
   fftw_free(out_cx);
//   free(out);

   return 1;
}

int CSpectrometer::fileFFT( const char* binfile, double* acc_spec, const char* out_bin_file, int out_coarse_channel, int skip_extra, const char* out_power_file, const char* out_power_fits, int n_out_channels, 
                            time_t file_ux_start , long int infile_size_bytes, const char* out_float_file )
{
   FILE* f = fopen(binfile, "rb");
   if( !f ){
      printf("ERROR : could not open file %s\n",binfile);
      return -1;
   }
   FILE* out_binary_f = NULL;
   if( out_bin_file && strlen(out_bin_file) ){
      out_binary_f = fopen(out_bin_file,"wb");
   }

   FILE* out_power_f = NULL;
   float* ptr_out_power_buffer = NULL;
   if( out_power_file && strlen(out_power_file) ){
      out_power_f = fopen(out_power_file,"wb");
   }
   
   float* ptr_out_float_buffer = NULL;
   FILE* out_float_f = NULL;
   if( out_float_file && strlen(out_float_file) ){
      out_float_f = fopen( out_float_file,"wb");
   }
   
   int n_written_power_bytes = 0;
   
   // 
   CBgFits* pOutPowerFits = NULL;
   int fits_file_index=0; // max_size_Y = 16384 
   int outFitsSizeY = -1;
   int maxFitsSize=32768; // was 16384;
//   int maxFitsSize=1600; // - TEST ONLY 
   time_t ux_start = file_ux_start;
   double _inttime = double(N_SAMPLES)/double(MWA_CLOCK_HZ);
   double freq_resolution_Hz = double(MWA_CLOCK_HZ/2) / double(N_CHANNELS);
   double freq_start_hz = (out_coarse_channel * N_FINE_CH_PER_COARSE) * freq_resolution_Hz;
   double freq_start_mhz = freq_start_hz/1e6;
   double delta_freq_mhz = (freq_resolution_Hz)/1e6;
   printf("DEBUG : freq. resolution = %.2f [Hz]\n",freq_resolution_Hz);
   printf("Setting FITS header in output file %s to values:\n",out_power_fits);
   printf("\tINTTIME    = %.8f [sec]\n",_inttime);
   printf("\tFREQ_START = %.2f [MHz]\n",freq_start_mhz);
   printf("\tDELTA_FREQ = %.2f [MHz]\n",delta_freq_mhz);
                                                               
   if( out_power_fits && strlen(out_power_fits) ){
      if( infile_size_bytes <= (long int)0 ){
         // TODO : implemented 2 versions for 64 and 32 bits :
         struct stat64 buf; 
         int ret = stat64( binfile, &buf );
         printf("stat returned %d\n",ret);
         if( ret < 0 ){
            perror("stat");
         }
         infile_size_bytes = (long int)buf.st_size;                      
         printf("Size of input file %s = %ld bytes\n",binfile,infile_size_bytes);
         if( infile_size_bytes <= 0 ){
            printf("ERROR: incorrect file size %d <= 0 !!! -> cannot continue \n",(int)infile_size_bytes);
            exit(-1);
         }
      }
      
      outFitsSizeY = infile_size_bytes/((long int)N_SAMPLES);
      if( outFitsSizeY > maxFitsSize ){
         printf("WARNING : calculated fits size = %d spectra truncated to limit size of %d spectra\n",outFitsSizeY,maxFitsSize);
         outFitsSizeY = maxFitsSize;
      }
      
      printf("Size of input file %s = %ld bytes -> size of output fits file = (%d,%d)\n",binfile,infile_size_bytes,n_out_channels,outFitsSizeY);
      pOutPowerFits = new CBgFits(n_out_channels,outFitsSizeY);
      pOutPowerFits->PrepareBigHornsHeader( (time_t)ux_start, _inttime, freq_start_mhz, delta_freq_mhz );
   }
   if( out_power_f || pOutPowerFits ){
      ptr_out_power_buffer = new float[n_out_channels];
   }
   if( out_float_f ){
      ptr_out_float_buffer = new float[n_out_channels*2];
   }
   
   int n=0;
   int n_samples_to_read = N_SAMPLES*m_PolsInFile;
   unsigned char* buffer = new unsigned char[n_samples_to_read]; // only reading buffer must be of larger size t accomodate both pols !
   if( skip_extra > 0 ){
      n = fread(buffer, sizeof(unsigned char), skip_extra, f);
      printf("Skipped extra %d bytes\n",n);
   }

   short* out_bin_buffer = new short[n_out_channels]; // 24 x 32 * 4 
   double buffer_double[N_SAMPLES];
   double acc_spec_re[N_CHANNELS],acc_spec_im[N_CHANNELS];
   double spectrum[N_SAMPLES],spectrum_re[N_SAMPLES],spectrum_im[N_SAMPLES];
   memset(acc_spec,'\0',sizeof(double)*N_CHANNELS);  
   memset(acc_spec_re,'\0',sizeof(double)*N_CHANNELS);  
   memset(acc_spec_im,'\0',sizeof(double)*N_CHANNELS);  
   int n_integr=0;
   int n_channels=N_CHANNELS;   
   int idx=0;
   
   char szCHANNEL_OUTFILE[1024];
   FILE* outf_channel = NULL;
   if( m_DumpChannel >= 0 ){
      sprintf(szCHANNEL_OUTFILE,"power_vs_time_channel%05d.txt",m_DumpChannel);
      outf_channel = fopen(szCHANNEL_OUTFILE,"w");
      printf("INFO : writing channel %d power to file %s\n",m_DumpChannel,szCHANNEL_OUTFILE);
   }

   printf("CSpectrometer::fileFFT : number of polarisations in file = %d , using polarisation = %d, n_samples_to_read = %d\n",m_PolsInFile,m_Pol,n_samples_to_read);
   int n_written=0;   
   int n_total_bytes_processed=0;
   FILE* samples_txt_file = NULL;
   if( strlen(m_szVoltageDumpFile.c_str()) > 0 ){
      samples_txt_file = fopen(m_szVoltageDumpFile.c_str(),"w");
   }
   
   while( (n = fread(buffer, sizeof(unsigned char), n_samples_to_read, f)) > 0 ){ // unsigned
       // printf("DEBUG : %d %d %d\n",((char*)buffer)[0],((char*)buffer)[1],((char*)buffer)[2]);
       if( n == n_samples_to_read ){
          if( m_nBits != 8 ){
             if ( m_nBits != 4 ){
                printf("ERROR : at the moment only 4 or 8 bits data are handled !\n");
                return -1;
             }
             
             if( m_nBits == 4 ){
                for(int l=0;l<n_samples_to_read;l++){
                   if( m_Pol > 0 ){
                      buffer[l]=(buffer[l] & 0xF0); // Y POL is MORE SIGNIFICANT 4bits
                   }else{
                       buffer[l]=((buffer[l] & 0x0F) << 4); // X POL is LESS SIGNIFICANT 4bits, but they must be shifted to become MORE SIGNIFICANT (as data was saved in sighorns_voltage_dump)
                   }                                                                                 
                   
                   // buffer[l] = buffer[l] - 16; // TEST
                }
             }
          }
 
          if( m_PolsInFile <= 1 ){
             // just 1 polarisation in the binary file, not need to de-interleave etc
             for(int k=0;k<n_samples_to_read;k++){             
                 buffer_double[k] = (double)(buffer[k]) - 128; //  - 128 to shift from 0-255 -> -128 - +127
                 
                 // TEST for normal char 
                 // buffer_double[k] = double(((char*)buffer)[k]);
                 
//               buffer_double[k] = (char)(buffer[k]);
//             if( k ==0 && idx<=5 ){
//                printf("%d : %.8f\n",k,buffer_double[k]);
//             }
             }          
          }else{
             // when >1 pol in file -> samples are interleaved so only those from requested polarisation have to be taken :
             int out_idx=0;
             for(int k=0;k<n_samples_to_read;k++){
                if( (k%2) == m_Pol ){
                   buffer_double[out_idx] = (double)(buffer[k]) - 128; //  - 128 to shift from 0-255 -> -128 - +127 
                   out_idx++;
                }
             }
//             printf("%d of %d polarisations used, used %d samples\n",m_Pol,m_PolsInFile,out_idx);
          }
 
          if( samples_txt_file ){         
             for(int i=0;i<N_SAMPLES;i++){
                // fprintf(samples_txt_file,"%d\n",(buffer[i]-128));
                fprintf(samples_txt_file,"%.1f\n",buffer_double[i]);
             }
          }
          
//          printf("DEBUG : %.2f , %.2f , %.2f\n",buffer_double[0],buffer_double[1],buffer_double[2]);          
          doFFT( buffer_double, N_SAMPLES, spectrum, spectrum_re, spectrum_im, n_channels, sqrt(n_channels)/2);
          
          for(int i=0;i<n_channels;i++){
             acc_spec[i] += spectrum[i];
             acc_spec_re[i] += spectrum_re[i];
             acc_spec_im[i] += spectrum_im[i];             
          }
          
          // printf("DEBUG : %.1f %.1f %.1f ... %.1f -> %.8f %.8f ... %.8f -> %.8f\n",buffer_double[0],buffer_double[1],buffer_double[2],buffer_double[511],spectrum[0],spectrum[1],spectrum[256],acc_spec[100]);
          
          n_integr++;
          
          if( outf_channel ){
             if( m_DumpChannel < n_channels ){
                fprintf(outf_channel,"%d %.8f %.8f %.8f\n",idx,spectrum_re[m_DumpChannel],spectrum_im[m_DumpChannel],spectrum[m_DumpChannel]);
             }
          }
          
          if( out_coarse_channel>=0 ){
//         char val_re = ((char*)(&val))[0];
//         char val_im = ((char*)(&val))[1];
               
//             ((char*)(&out_bin_buffer))[0] = spectrum_re[i];
//             ((char*)(&out_bin_buffer))[1] = spectrum_im[i];
             int start = out_coarse_channel*N_FINE_CH_PER_COARSE;             
             for(int out_ch=0;out_ch<n_out_channels;out_ch++){             
                int ch = out_ch + start - 64;
                // int ch = out_ch + start; // just a test 
                short& out_short = out_bin_buffer[out_ch];
                
                ((char*)(&out_short))[0] = spectrum_re[ch];
                ((char*)(&out_short))[1] = spectrum_im[ch];
                
                if( ptr_out_power_buffer ){
                   ptr_out_power_buffer[out_ch] = spectrum[ch];
                }
                if( ptr_out_float_buffer ){
                   ptr_out_float_buffer[2*out_ch] = spectrum_re[ch];
                   ptr_out_float_buffer[2*out_ch+1] = spectrum_im[ch];
                }               
                
                if ( m_DebugNSpectra > 0 && idx<m_DebugNSpectra && (out_ch==0 || out_ch==64 || out_ch==120)){
                   if( out_ch==0 ){
                      printf("Spectrum %d : re/im ch%d(%d) = %.2f/%.2f (%d/%d), ",idx,out_ch,ch,spectrum_re[ch],spectrum_im[ch],((char*)(&out_short))[0],((char*)(&out_short))[1]);fflush(stdout);
                   }else{
                      printf("re/im ch%d(%d) = %.2f/%.2f (%d/%d), ",out_ch,ch,spectrum_re[ch],spectrum_im[ch],((char*)(&out_short))[0],((char*)(&out_short))[1]);fflush(stdout);
                      if( out_ch == 120 ){
                         printf("\n");
                      }
                   }
                }
             }
             int ret = 0;
             if ( out_binary_f ) {
                ret = fwrite(out_bin_buffer, sizeof(short), n_out_channels, out_binary_f );                                  
             }
             n_written += ret*sizeof(short);
             
             if( out_power_f ){
                int ret2 = fwrite( ptr_out_power_buffer, sizeof(float), n_out_channels, out_power_f );
                n_written_power_bytes += ret2;
             }
             if( ptr_out_float_buffer && out_float_f ){
                int ret2 = fwrite( ptr_out_float_buffer, sizeof(float), n_out_channels*2, out_float_f );                
             }
             if( pOutPowerFits ){
                int n_lines = pOutPowerFits->add_line( ptr_out_power_buffer , n_out_channels );
                if( n_lines >= outFitsSizeY ){
                   // save fits here and re-init lines counter:
                   char szOutFitsName[1024];
                   sprintf(szOutFitsName,out_power_fits,fits_file_index);
                   printf("INFO : output fits file written to file %s\n",szOutFitsName);fflush(stdout);
                   pOutPowerFits->WriteFits(szOutFitsName);
//                   pOutPowerFits->reset_lines_counter();
                   
                   delete pOutPowerFits;                   
                   // printf("Creating new FITS File (%d,%d)\n",n_out_channels,outFitsSizeY);
                   pOutPowerFits = new CBgFits(n_out_channels,outFitsSizeY);
                   pOutPowerFits->PrepareBigHornsHeader( (time_t)ux_start, _inttime, freq_start_mhz, delta_freq_mhz );
                         
                   fits_file_index++;
                }
             }
          }
          
          n_total_bytes_processed += n;
          
          if( m_MaxBytesToProcess > 0 ){
             // if required check limit number of bytes to be processed:
             if( n_total_bytes_processed > m_MaxBytesToProcess ){
                printf("Processed %d bytes > limit = %d -> no more data will be processed\n",n_total_bytes_processed,m_MaxBytesToProcess);fflush(stdout);
                break;
             }
          }
       }
       
       idx++;
   }        
   fclose(f);
   
   if( samples_txt_file ){
      printf("Samples writted to file %s\n",m_szVoltageDumpFile.c_str());
      fclose(samples_txt_file);
   }
   
   if( out_binary_f ){
      fclose(out_binary_f);
      printf("Total number of bytes written to binnary file = %d (expected %d)\n",n_written,(n_out_channels*2*idx));
   }
   
   if( out_float_f ){
      fclose(out_float_f);
   }
   
//   for(int i=0;i<n_channels;i++){
//      printf("%d %e\n",i,acc_spec[i]);
//      double power = sqrt( acc_spec_re[i]*acc_spec_re[i] + acc_spec_im[i]*acc_spec_im[i]);
//      printf("%d %e\n",i,power);
//   }
   
   if( outf_channel ){
      fclose(outf_channel);
   }
   if( out_power_f ){
      fclose(out_power_f);
      printf("INFO : written %d bytes into output power file %s\n",n_written_power_bytes,out_power_file);
   }
   if( ptr_out_power_buffer ){
      delete [] ptr_out_power_buffer;
   }   
   if( ptr_out_float_buffer ){
      delete [] ptr_out_float_buffer;      
   }
   if( out_bin_buffer ){
      delete [] out_bin_buffer;
   }
   if (buffer ){
      delete [] buffer;
   }
   
   if( pOutPowerFits && pOutPowerFits->get_lines_counter()>0 ){
      char szOutFitsName[1024];
      sprintf(szOutFitsName,out_power_fits,fits_file_index);
      pOutPowerFits->set_ysize();
      pOutPowerFits->WriteFits(szOutFitsName);
      printf("INFO : output fits file written to file %s\n",szOutFitsName);
   }

   return n_integr;
}

int CSpectrometer::filePFB( const char* binfile, double* acc_spec, const char* out_bin_file, int out_coarse_channel, int skip_extra, int n_taps )
{
   if( strlen(CSpectrometer::gPfbCoeffFile.c_str()) ){
      if( CSpectrometer::gPfbCoeffs.size() <= 0 ){
         printf("Reading PFB coefficients from file %s\n",CSpectrometer::gPfbCoeffFile.c_str());
         CBgFits tmp_fits( CSpectrometer::gPfbCoeffFile.c_str() );
         if( tmp_fits.ReadFits( CSpectrometer::gPfbCoeffFile.c_str() ) != 0 ){
            printf("Could not read PFB coefficients fits file %s -> exiting now\n",CSpectrometer::gPfbCoeffFile.c_str());
            exit(-1);
         }
         
         CSpectrometer::gPfbCoeffs.clear();
         for(int i=0;i<tmp_fits.GetXSize();i++){
            CSpectrometer::gPfbCoeffs.push_back( tmp_fits.valXY(i,0) );
         }
         printf("Read %d PFB coefficients from file %s , first and last 10 are :\n",(int)CSpectrometer::gPfbCoeffs.size(),CSpectrometer::gPfbCoeffFile.c_str() );
         for(int i=0;i<10;i++){
            printf("\t%.8f\n",CSpectrometer::gPfbCoeffs[i]);
         }
         printf("Last 10 :\n");
         for(int i=0;i<10;i++){
            printf("\t%.8f\n",CSpectrometer::gPfbCoeffs[CSpectrometer::gPfbCoeffs.size()-10+i]);
         }
         
      }
   }

   char szCHANNEL_OUTFILE[1024];
   FILE* outf_channel = NULL;
   if( m_DumpChannel >= 0 ){
      sprintf(szCHANNEL_OUTFILE,"power_vs_time_channel%05d.txt",m_DumpChannel);
      outf_channel = fopen(szCHANNEL_OUTFILE,"w");
      printf("INFO : writing channel %d power to file %s\n",m_DumpChannel,szCHANNEL_OUTFILE);
   }

   FILE* f = fopen(binfile, "rb");
   if( !f ){
      printf("ERROR : could not open file %s\n",binfile);
      return -1;
   }
   FILE* out_binary_f = NULL;
   if( out_bin_file && strlen(out_bin_file) ){
      out_binary_f = fopen(out_bin_file,"wb");
   }
   
   int n=0;
   unsigned char buffer[N_SAMPLES];
   if( skip_extra > 0 ){
      n = fread(buffer, sizeof(unsigned char), skip_extra, f);
      printf("Skipped extra %d bytes\n",n);
   }
   
   int pfb_buffer_size = N_SAMPLES*n_taps;
   double* pfb_buffer = new double[pfb_buffer_size]; // contains several (n_taps) portions of the N_SAMPLES sample sets (untouched - not multiplied by any coefficients !)
   double* tmp_buffer = new double[pfb_buffer_size]; 
   int pfb_idx=0;

   short out_bin_buffer[N_FINE_CH_PER_BAND]; // 24 x 32 * 4 
   double buffer_double[N_SAMPLES];
   double acc_spec_re[N_CHANNELS],acc_spec_im[N_CHANNELS];
   double spectrum[N_SAMPLES],spectrum_re[N_SAMPLES],spectrum_im[N_SAMPLES];
   memset(acc_spec,'\0',sizeof(double)*N_CHANNELS);  
   memset(acc_spec_re,'\0',sizeof(double)*N_CHANNELS);  
   memset(acc_spec_im,'\0',sizeof(double)*N_CHANNELS);  
   int n_integr=0;
   int n_channels=N_CHANNELS;   
   int idx=0;

   int n_written=0;   
   while( (n = fread(buffer, sizeof(unsigned char), N_SAMPLES, f)) > 0 ){
       if( n == N_SAMPLES ){
          for(int k=0;k<N_SAMPLES;k++){             
              buffer_double[k] = (double)(buffer[k]) - 128; //  - 128;
//               buffer_double[k] = (char)(buffer[k]);

//              printf("%d %.8f ???\n",k+pfb_idx*N_SAMPLES,buffer_double[k]);
          }          
 
          // fill PFB buffers:
          if( pfb_idx < n_taps ){
             for(int i=0;i<N_SAMPLES;i++){
                pfb_buffer[pfb_idx*N_SAMPLES+i] = buffer_double[i];
             }
             pfb_idx++;
          }else{
             // buffer is filled -> shift by one N_SAMPLES 
//             for(int i=0;i<(N_SAMPLES*(n_taps-1));i++){
//                pfb_buffer[i] = pfb_buffer[i + N_SAMPLES];
//             }
///             printf("memcpy ?\n");
             memcpy( tmp_buffer , pfb_buffer+N_SAMPLES , sizeof(double)*(N_SAMPLES*(n_taps-1)) );
             memcpy( pfb_buffer , tmp_buffer, sizeof(double)*(N_SAMPLES*(n_taps-1)) );
             for(int i=0;i<N_SAMPLES;i++){
                pfb_buffer[ (N_SAMPLES*(n_taps-1)) + i ] = buffer_double[i];
             }
          }     
          if( pfb_idx < n_taps ){
             continue;
          }
          
          
          // fill buffer for FFT :
          for(int i=0;i<N_SAMPLES;i++){
             buffer_double[i] = 0;
          }
          
          // multiply by a sinc function:
          // TF1* pFunc = new TF1("user","sin(x*2*3.1415/65536)/(x*2*3.1415/65536)",-3*65536,+3*65536);
          // TF1* pFunc = new TF1("user","sin(x*2*3.1415/(2*65536))/(x*2*3.1415/(2*65536))",-6*65536,+6*65536);                   
          for(int i=0;i<N_SAMPLES*n_taps;i++){
//          for(int i=0;i<N_SAMPLES;i++){
             double x = i - N_SAMPLES*(n_taps/2);
             
             double gPeriodsInPfbSinc = (6.0/5.0);
             double norm = ((n_taps/2)/gPeriodsInPfbSinc)*N_SAMPLES; // to have 3 periods inside the whole samples on each side from 0 
             double coeff = 0.57*( sin(x*2*M_PI/(norm))/(x*2*M_PI/(norm)) );
             if( CSpectrometer::gPfbCoeffs.size() > 0 ){
                if(  ((int)CSpectrometer::gPfbCoeffs.size()) == ((int)(N_SAMPLES*n_taps)) ){
                   if( idx == 0 && i==0 ){
                      printf("Using file coefficients - sizes OK !\n");
                   }
                   coeff = CSpectrometer::gPfbCoeffs[i];
                }else{
                   if( idx == 0 && i==0 ){
                      printf("WARNING : pfb file %s provided but does not match option -t %d sizes %d != %d\n",CSpectrometer::gPfbCoeffFile.c_str(),n_taps,(int)CSpectrometer::gPfbCoeffs.size(),N_SAMPLES*n_taps);
                      printf("WARNING : option should be -t %d\n",(int)(CSpectrometer::gPfbCoeffs.size()/N_SAMPLES));                      
                   }
                }
             }
             if( x == 0 ){
//                printf("x = 0 at i = %d -> coeff := 1\n",i);
                coeff=1;
             }
             
             tmp_buffer[i] = pfb_buffer[i] * coeff; // store pfb_buffer x SINC-coefficients into tmp_buffer
//             printf("%d %.8f\n",i,coeff);
             
             int ii = (i % N_SAMPLES);
             buffer_double[ii] += tmp_buffer[i]; // add weighted samples to buffer_double 
             if( ii ==0 && idx==0 ){
                printf("%d : %.8f -> %.8f\n",i,pfb_buffer[i],buffer_double[ii]);
             }

//             printf("%d %.8f\n",i,pfb_buffer[i]);          
          }
//          for(int i=0;i<N_SAMPLES;i++){
//             printf("%d %.8f\n",i,buffer_double[i]);
//             buffer_double[i] = pfb_buffer[ N_SAMPLES*6 + i ];
//             buffer_double[i] = buffer_double[i] / n_taps;
//          }
//          exit(0);
                             
          
 
          doFFT( buffer_double, N_SAMPLES, spectrum, spectrum_re, spectrum_im, n_channels, sqrt(n_channels)/2);
          
          for(int i=0;i<n_channels;i++){
             acc_spec[i] += spectrum[i];
             acc_spec_re[i] += spectrum_re[i];
             acc_spec_im[i] += spectrum_im[i];             
          }
          n_integr++;

          if( outf_channel ){
             if( m_DumpChannel < n_channels ){
                fprintf(outf_channel,"%d %.8f %.8f %.8f\n",idx,spectrum_re[m_DumpChannel],spectrum_im[m_DumpChannel],spectrum[m_DumpChannel]);
             }
          }

          
          if( out_binary_f && out_coarse_channel>=0 ){
//         char val_re = ((char*)(&val))[0];
//         char val_im = ((char*)(&val))[1];
               
//             ((char*)(&out_bin_buffer))[0] = spectrum_re[i];
//             ((char*)(&out_bin_buffer))[1] = spectrum_im[i];
             int start = out_coarse_channel*N_FINE_CH_PER_COARSE;             
             for(int out_ch=0;out_ch<N_FINE_CH_PER_BAND;out_ch++){             
                int ch = out_ch + start - 64;
                short& out_short = out_bin_buffer[out_ch];
                
                ((char*)(&out_short))[0] = spectrum_re[ch];
                ((char*)(&out_short))[1] = spectrum_im[ch];
                
                if ( m_DebugNSpectra > 0 && idx<m_DebugNSpectra && (out_ch==0 || out_ch==64 || out_ch==120)){
                   if( out_ch==0 ){
                      printf("Spectrum %d : re/im ch%d(%d) = %.2f/%.2f (%d/%d), ",idx,out_ch,ch,spectrum_re[ch],spectrum_im[ch],((char*)(&out_short))[0],((char*)(&out_short))[1]);fflush(stdout);
                   }else{
                      printf("re/im ch%d(%d) = %.2f/%.2f (%d/%d), ",out_ch,ch,spectrum_re[ch],spectrum_im[ch],((char*)(&out_short))[0],((char*)(&out_short))[1]);fflush(stdout);
                      if( out_ch == 120 ){
                         printf("\n");
                      }
                   }
                }
             }
             int ret = fwrite(out_bin_buffer, sizeof(short), N_FINE_CH_PER_BAND, out_binary_f );                                  
             n_written += ret*sizeof(short);
          }
       }
       
       idx++;
   }        
   fclose(f);
   
   if( out_binary_f ){
      fclose(out_binary_f);
      printf("Total number of bytes written to binnary file = %d (expected %d)\n",n_written,(N_FINE_CH_PER_BAND*2*idx));
   }
   
//   for(int i=0;i<n_channels;i++){
//      printf("%d %e\n",i,acc_spec[i]);
//      double power = sqrt( acc_spec_re[i]*acc_spec_re[i] + acc_spec_im[i]*acc_spec_im[i]);
//      printf("%d %e\n",i,power);
//   }

   if( pfb_buffer ){
      delete [] pfb_buffer;
   }
   if( outf_channel ){
      fclose(outf_channel);
   }

   
   return n_integr;
}


int CSpectrometer::dumpSignatecBinFile( const char* binfile, int dump_idx, int n_samples )
{
   FILE* f = fopen(binfile, "rb");
   if( !f ){
      printf("ERROR : could not open file %s\n",binfile);
      return -1;
   }

   unsigned char* buffer = new unsigned char[n_samples];
   int n=0;
   int idx=0;
   int dumped=0;
   
   while( (n = fread(buffer, sizeof(unsigned char), n_samples, f)) > 0 ){
       if( dump_idx<0 || idx==dump_idx ){
          for(int k=0;k<n;k++){
             printf("%d\n",(int)buffer[k]);
          }    
          dumped++;      
       }
       idx++;
   }        
   fclose(f);
   
   
   return dumped;

}

int CSpectrometer::CorrelateBinary( const char* eda_file, const char* bighorns_file, 
                                    vector<double>& avg_power, vector<double>& avg_eda_power,
                                    vector<double>& cross_power_re, vector<double>& cross_power_im,
                                    int spectrum_size, int bytes_per_channel, CBgFits* pCrossPowerFullTimeRes,
                                    int nIntegrations /*=20000*/  )
{
   // date2date -ut2ux=20171129_205406
   double start_uxtime = 1511988846.1021090000; // see calculation in /home/msok/Desktop/MWA/students/2019/SummerStudents/Archana/logbook/2019_2020_Archana.odt
                                            // was 1511988846; 
                                            // check if start time is really where ANTENNA starts ! not the start of the RECORDING !!! This seems to be wrong !!!
   double time_step    = 0.1 / 1000.0; // seconds = 0.1ms
   double hyda_ra_deg  = 9.30158333*15.00;
   double hyda_dec_deg = -12.09555556;
   double unix_time     = start_uxtime;
//   double geo_long_mwa = 116.670815; // nominal MWA position (The Stone at the Donga)
//   double geo_lat_mwa  = -26.703319;

   double geo_long_mwa = 116.6717555000; // (116.671254 + 116.672257)/2.00 - mean of the EDA and BIGHORNS from dropbox link
   double geo_lat_mwa =  -26.7029685000; // (-26.702868+(-26.703069))/2.00 - mean of the EDA and BIGHORNS from dropbox link
   double C_ms = 299792458.00;
   double C_mhz = C_ms / 1000000.00;
      

   bool   bGeoCorrection = ( CSpectrometer::m_GeometryCorrection != 0 );
   bool   bCableCorrectAll = false; // do not correct all spectra just mean RE/IM spectrum
   
   // https://www.dropbox.com/home/students/2019/SummerStudents/Archana/MS_memo_series/201801/geo_corr?preview=BIGHORNS_position_RW.txt
   // Interaction point of Bighorns changes with frequency ! - see Adrian's Hybrid Interferometry paper (2015) : https://ui.adsabs.harvard.edu/abs/2017ITAP...65.3967S/abstract
   double H_bighorns = 2491.1 / 1000.00; // mm
   double R_bighorns = 750.00 / 1000.00 ; // mm
   double tan_alpha = R_bighorns / H_bighorns;
   double cable_len_eda = CSpectrometer::m_EDA_ElectricalLenM;
//   double z_bighorns = -0.827 + 0.5 + H_bighorns - C_mhz/(2.00*M_PI*freq_mhz*tan_alpha); // 0.5m for concrete blocks 
   // this might help to fix Z(freq) dependence, but may not be sufficient then full Jones treatment of Bighorns x EDA correlation might be required !
   
//   CCmnVector vecBighornsLocation( 43.658, 50.009, z_bighorns ); // Bighorns Antenna position 
   CCmnVector vecEdaLocation( 143.441, 27.773, -3.045 );     // EDA-1 center position
//   CCmnVector vecBaseline( (43.658 - 143.441)/2.00, (50.009 - 27.773)/2.00, (-0.827-(-3.045))/2.00 ); // CCmnVector has not operator- implemented ...
   

//   CCmnVector vecBaseline( (43.658 - 143.441), (50.009 - 27.773), (z_bighorns-(-3.045)) ); // CCmnVector has not operator- implemented ...
   

   printf("DEBUG : CorrelateBinary\n");
   FILE* eda_f = fopen(eda_file, "rb");
   if( !eda_f ){
      printf("ERROR : could not open file %s\n",eda_file);
      return -1;
   }
   printf("Opened EDA file %s\n",eda_file);
   
   FILE* bighorns_f = fopen(bighorns_file, "rb");
   if( !bighorns_f ){
      printf("ERROR : could not open file %s\n",bighorns_file);
      return -1;
   }
   printf("Opened BIGHORNS file %s\n",bighorns_file);

   avg_power.assign( spectrum_size, 0 );
   avg_eda_power.assign( spectrum_size, 0 );
   cross_power_re.assign( spectrum_size, 0 );
   cross_power_im.assign( spectrum_size, 0 );

   int single_spectrum_size = spectrum_size*bytes_per_channel; // assuming sizeof(short)
   unsigned char* eda_buffer = (unsigned char*)(new char[single_spectrum_size]);
   unsigned char* bighorns_buffer = (unsigned char*)(new char[single_spectrum_size]);

   int n_eda=0;
   int index=0;
   while( (n_eda = fread(eda_buffer, bytes_per_channel, spectrum_size, eda_f)) > 0 ){
      int n_bighorns = fread(bighorns_buffer, bytes_per_channel, spectrum_size, bighorns_f);
      // void radec2azh( double ra, double dec, time_t unix_time, double& out_azim, double& out_alt );
      double hyda_az_deg, hyda_alt_deg;
//      void radec2azh( double ra, double dec, time_t unix_time, double geo_long_deg, double geo_lat_deg, double& out_az, double& out_alt );
      radec2azh( hyda_ra_deg, hyda_dec_deg, unix_time, geo_long_mwa, geo_lat_mwa, hyda_az_deg, hyda_alt_deg );
      double hyda_za_deg = (90.00 - hyda_alt_deg);
      double hyda_za_rad = hyda_za_deg*(M_PI/180.00);
      double hyda_az_rad = hyda_az_deg*(M_PI/180.00);
      CCmnVector en_HydA( sin(hyda_za_rad)*sin(hyda_az_rad), sin(hyda_za_rad)*cos(hyda_az_rad) , cos(hyda_za_rad) );
      
      if( index == 0 || index == 1000 ){
         printf("DEBUG(index=%d) : eda[0] = %d eda[1000] = %d , bighorns[0] = %d bighorns[1000] = %d\n",index,eda_buffer[0],eda_buffer[1000],bighorns_buffer[0],bighorns_buffer[1000]);fflush(stdout);
      }

      
       if( pCrossPowerFullTimeRes ){
          if( index >= pCrossPowerFullTimeRes->GetYSize() ){
//             if( pCrossPowerFullTimeRes->GetYSize()  >= 51200 ){
//                printf("WARNING : maximum size exceeded -> breaking the loop\n");
//                break;
//             }
             printf("INFO : doubling size of pCrossPowerFullTimeRes image ( %ld -> %ld lines) - on new Ubuntu >= 18 program crashes at 51000 or close\n",pCrossPowerFullTimeRes->GetYSize(),2*pCrossPowerFullTimeRes->GetYSize() );fflush(stdout);
             pCrossPowerFullTimeRes->Realloc( pCrossPowerFullTimeRes->GetXSize(), 2*pCrossPowerFullTimeRes->GetYSize() );                          
          }         
      }
   
      for(int i=0;i<spectrum_size;i++){
         double freq_mhz = ((double)i)*0.01; // 10kHz channels 

         // Bighorns Z has to be calculated for each frequency channel separately as the interaction point depends on the height Z ~ is there circumference = lamda -> 2PiR = lamda 
         double delta = C_mhz/(2.00*M_PI*freq_mhz*tan_alpha);
         double z_bighorns = -0.827 + 0.5 + H_bighorns - delta; // 0.5m for concrete blocks 
         CCmnVector vecBighornsLocation( 43.658, 50.009, z_bighorns ); // Bighorns Antenna position
         CCmnVector vecBaseline( (43.658 - 143.441), (50.009 - 27.773), (z_bighorns-(-3.045)) ); // CCmnVector has not operator- implemented ...
// Strange test          CCmnVector vecBaseline( (43.658 - 143.441), (50.009 - 27.773), (z_bighorns - 1.36) ); // TEST 
         double geo_delay_distance = CCmnVector::V_scalar_product( en_HydA, vecBaseline );
         
      
         short eda_fft_reim = ((short*)eda_buffer)[i];         
         double val_re = (char)((unsigned char*)(&eda_fft_reim))[0];
         double val_im = (char)((unsigned char*)(&eda_fft_reim))[1];

         short bighorns_fft_reim = ((short*)bighorns_buffer)[i];         
         double val2_re = (char)((unsigned char*)(&bighorns_fft_reim))[0];
         double val2_im = (char)((unsigned char*)(&bighorns_fft_reim))[1];
         
         if( index == 0 || index==100 ){
            printf("Integration %d , channel = %d : EDA = (%.1f / %.1f) , BIGHORNS = (%.1f / %.1f)\n",index,i,val_re,val_im,val2_re,val2_im);
         }

         double cable_len_bighorns = 0; //  + delta; // meters + delta where we assume that the spireal arms act like a piece of cable when interaction point changes its vertical position 
         double phi_rad_bighorns = 2.00*M_PI*(freq_mhz/C_mhz)*cable_len_bighorns;
         double sin_phi_bighorns = sin(phi_rad_bighorns);
         double cos_phi_bighorns = cos(phi_rad_bighorns);
         double val2_re_new = val2_re*cos_phi_bighorns - val2_im*sin_phi_bighorns;
         double val2_im_new = val2_re*sin_phi_bighorns + val2_im*cos_phi_bighorns;
         val2_re = val2_re_new;
         val2_im = val2_im_new;
         
         // geometric correction :
         if ( bGeoCorrection ){
            double phi_rad_bighorns_geo = CSpectrometer::m_GeometryCorrection*2.00*M_PI*(freq_mhz/C_mhz)*geo_delay_distance; // tested both signs
            double sin_phi_bighorns_geo = sin(phi_rad_bighorns_geo);
            double cos_phi_bighorns_geo = cos(phi_rad_bighorns_geo);
            double val3_re_new = val2_re*cos_phi_bighorns_geo - val2_im*sin_phi_bighorns_geo;
            double val3_im_new = val2_re*sin_phi_bighorns_geo + val2_im*cos_phi_bighorns_geo;
            val2_re = val3_re_new;
            val2_im = val3_im_new;
            
            if( (index % 1000) == 0 && (i>=15000 && i<=15010) ){
               printf("DEBUG (time_index = %d , channel = %d) : geo_delay_distance = %.8f [m] -> phase = %.8f [deg]\n",index,i,geo_delay_distance,phi_rad_bighorns_geo*(180.00/M_PI));
            }
         }
         
/*
         // experimental - additional phenomenological term from parabola fit :

//cd /home/msok/Desktop/MWA/students/2019/SummerStudents/Archana/logbook/20200213_C++/Cable140m/Cable+140m/GeoMinus_FullWithZvsFreq
//root / fit parabola 
//p0                        =     -1086.69   +/-   18.3686     
//p1                        =     0.195676   +/-   0.00322282  
//p2                        = -8.19657e-06   +/-   1.37242e-07 
         
/         double fitted_A = -8.19657e-06;
//         double fitted_B = 0.195676;
//         double fitted_C = -1086.69;         
//         double extra_len_term = ((2.00*fitted_A*freq_mhz + fitted_B) / ( 2.00*M_PI*freq_mhz ))*C_mhz;
//         cable_len_eda = cable_len_eda - extra_len_term;      
        
*/
         if( bCableCorrectAll ){         
            double phi_rad_eda = 2.00*M_PI*(freq_mhz/C_mhz)*cable_len_eda;
            double sin_phi_eda = sin(phi_rad_eda);
            double cos_phi_eda = cos(phi_rad_eda);

            double val_re_new = val_re*cos_phi_eda - val_im*sin_phi_eda;
            double val_im_new = val_re*sin_phi_eda + val_im*cos_phi_eda;
         
            val_re = val_re_new;
            val_im = val_im_new;
         }

/*         if( index==1 ){
            printf("channel=%d (%.2f MHz) : Phase due to cables BIGHORNS = %.2f [rad], EDA = %.2f [rad]\n",i,freq_mhz,phi_rad_bighorns,phi_rad_eda);
         }*/

         
         double product_re = val_re*val2_re + val_im*val2_im;
         double product_im = val_im*val2_re - val_re*val2_im;
         
         if( pCrossPowerFullTimeRes ){
            // double cross_power = sqrt( product_re*product_re + product_im*product_im );
            // TEST 
            double cross_power = val2_re*val2_re + val2_im*val2_im;
            pCrossPowerFullTimeRes->setXY( i, index, cross_power );
         }
            
         // TODO : check in cotter !!!
         // double cable_len_diff = -5000.00; // meters difference between EDA and BIGHORNS cables 
         // double phi_rad = 2.00*M_PI*(freq_mhz/300.00)*cable_len_diff;
         // double sin_phi = sin(phi_rad);
         // double cos_phi = cos(phi_rad);
         //  double new_re = product_re*cos_phi - product_im*sin_phi;
         // double new_im = product_re*sin_phi + product_im*cos_phi;
 
         // cable correction :
         /*int gDoCableCorr=1;
         if( gDoCableCorr > 0 ){
            product_re = new_re;
            product_im = new_im;        
         }*/
            
         cross_power_re[i] += product_re;
         cross_power_im[i] += product_im;         
            
         double power = val2_re*val2_re + val2_im*val2_im;
         avg_power[i] += power;
         
         double power_eda = val_re*val_re + val_im*val_im;
         avg_eda_power[i] += power_eda;
      }      

      if( nIntegrations > 0 ){
         if( index >= nIntegrations ){
            printf("WARNING : index=%d -> stopping here !\n",nIntegrations);
            break;
         }
      }
      
      index++;
      unix_time += time_step;
   }
   fclose(eda_f);
   fclose(bighorns_f);

   if( pCrossPowerFullTimeRes ){   
      printf("Setting size of full resulting output file to %d\n",index);
      pCrossPowerFullTimeRes->SetYSize(index);
   }
   
   if( avg_power.size()!=cross_power_re.size() || avg_power.size()!=cross_power_im.size() ){
      printf("ERROR : wrong size of output array !!!\n");
      exit(-1);
   }
   
   for(int i=0;i<((int)avg_power.size());i++){
      double freq_mhz = ((double)i)*0.01; // 10kHz channels
   
      avg_power[i] = avg_power[i] / index;
      avg_eda_power[i] = avg_eda_power[i] / index;            
      cross_power_re[i] = cross_power_re[i] / index;
      cross_power_im[i] = cross_power_im[i] / index;
      
      // cable correction applied to mean :
      complex<double> cross_power( cross_power_re[i] , cross_power_im[i] );
      
      if( !bCableCorrectAll ){
         // only if not applied to every single spectrum earlier :
         double phi_rad_eda = 2.00*M_PI*(freq_mhz/C_mhz)*cable_len_eda;
         complex<double> phase_corr_cable( cos(phi_rad_eda) , sin(phi_rad_eda) );  
         cross_power = cross_power * phase_corr_cable;
      }
      
      cross_power_re[i] = cross_power.real();
      cross_power_im[i] = cross_power.imag();
   }

   delete [] eda_buffer;
   delete [] bighorns_buffer;

   return index; // returns number of averaged spectra 
}

int CSpectrometer::CorrelateBinarySimple( const char* eda_file, const char* bighorns_file, 
                                    vector<double>& avg_power, vector<double>& avg_eda_power,
                                    vector<double>& cross_power_re, vector<double>& cross_power_im,
                                    int spectrum_size, int bytes_per_channel, CBgFits* pCrossPowerFullTimeRes,
                                    int nIntegrations /*=20000*/ , int n_pols /*=2*/, int pol /*=0*/ )
{
   printf("DEBUG : CorrelateBinary\n");
   FILE* eda_f = fopen(eda_file, "rb");
   if( !eda_f ){
      printf("ERROR : could not open file %s\n",eda_file);
      return -1;
   }
   printf("Opened EDA file %s\n",eda_file);
   
   FILE* bighorns_f = fopen(bighorns_file, "rb");
   if( !bighorns_f ){
      printf("ERROR : could not open file %s\n",bighorns_file);
      return -1;
   }
   printf("Opened BIGHORNS file %s\n",bighorns_file);

   avg_power.assign( spectrum_size, 0 );
   avg_eda_power.assign( spectrum_size, 0 );
   cross_power_re.assign( spectrum_size, 0 );
   cross_power_im.assign( spectrum_size, 0 );

   int single_spectrum_size = spectrum_size*bytes_per_channel*n_pols; // assuming sizeof(short)
   unsigned char* eda_buffer = (unsigned char*)(new char[single_spectrum_size]);
   unsigned char* bighorns_buffer = (unsigned char*)(new char[single_spectrum_size]);

   int n_eda=0;
   int index=0;
   while( (n_eda = fread(eda_buffer, bytes_per_channel, spectrum_size*n_pols, eda_f)) > 0 ){
      int n_bighorns = fread(bighorns_buffer, bytes_per_channel, spectrum_size*n_pols, bighorns_f);
      if( index == 0 || index == 1000 ){
         printf("DEBUG(index=%d) : eda[0] = %d eda[1000] = %d , bighorns[0] = %d bighorns[1000] = %d\n",index,eda_buffer[0],eda_buffer[1000],bighorns_buffer[0],bighorns_buffer[1000]);fflush(stdout);
      }

      
       if( pCrossPowerFullTimeRes ){
          if( index >= pCrossPowerFullTimeRes->GetYSize() ){
//             if( pCrossPowerFullTimeRes->GetYSize()  >= 51200 ){
//                printf("WARNING : maximum size exceeded -> breaking the loop\n");
//                break;
//             }
             printf("INFO : doubling size of pCrossPowerFullTimeRes image ( %ld -> %ld lines) - on new Ubuntu >= 18 program crashes at 51000 or close\n",pCrossPowerFullTimeRes->GetYSize(),2*pCrossPowerFullTimeRes->GetYSize() );fflush(stdout);
             pCrossPowerFullTimeRes->Realloc( pCrossPowerFullTimeRes->GetXSize(), 2*pCrossPowerFullTimeRes->GetYSize() );                          
          }         
      }
   
      for(int i=0;i<spectrum_size; i++ ){
         double freq_mhz = ((double)i)*0.01; // 10kHz channels 
      
         short eda_fft_reim = ((short*)eda_buffer)[i*n_pols+pol]; // *n_pols to only select 0 polarisation 
         double val_re = (char)((unsigned char*)(&eda_fft_reim))[0];
         double val_im = (char)((unsigned char*)(&eda_fft_reim))[1];

         short bighorns_fft_reim = ((short*)bighorns_buffer)[i*n_pols+pol]; // *n_pols to only select 0 polarisation 
         double val2_re = (char)((unsigned char*)(&bighorns_fft_reim))[0];
         double val2_im = (char)((unsigned char*)(&bighorns_fft_reim))[1];
         
         if( index == 0 || index==100 ){
            printf("Integration %d , channel = %d : EDA = (%.1f / %.1f) , BIGHORNS = (%.1f / %.1f)\n",index,i,val_re,val_im,val2_re,val2_im);
         }

         double cable_len_bighorns = 0; //  + delta; // meters + delta where we assume that the spireal arms act like a piece of cable when interaction point changes its vertical position 
/*         double sin_phi_bighorns = sin(phi_rad_bighorns);
         double cos_phi_bighorns = cos(phi_rad_bighorns);
         double val2_re_new = val2_re*cos_phi_bighorns - val2_im*sin_phi_bighorns;
         double val2_im_new = val2_re*sin_phi_bighorns + val2_im*cos_phi_bighorns;
         val2_re = val2_re_new;
         val2_im = val2_im_new;*/
         
         double product_re = val_re*val2_re + val_im*val2_im;
         double product_im = val_im*val2_re - val_re*val2_im;
         
         if( pCrossPowerFullTimeRes ){
            // double cross_power = sqrt( product_re*product_re + product_im*product_im );
            // TEST 
            double cross_power = val2_re*val2_re + val2_im*val2_im;
            pCrossPowerFullTimeRes->setXY( i, index, cross_power );
         }
            
         cross_power_re[i] += product_re;
         cross_power_im[i] += product_im;         
            
         double power = val2_re*val2_re + val2_im*val2_im;
         avg_power[i] += power;
         
         double power_eda = val_re*val_re + val_im*val_im;
         avg_eda_power[i] += power_eda;
      }      

      if( nIntegrations > 0 ){
         if( index >= nIntegrations ){
            printf("WARNING : index=%d -> stopping here !\n",nIntegrations);
            break;
         }
      }
      
      index++;
   }
   fclose(eda_f);
   fclose(bighorns_f);

   if( pCrossPowerFullTimeRes ){   
      printf("Setting size of full resulting output file to %d\n",index);
      pCrossPowerFullTimeRes->SetYSize(index);
   }
   
   if( avg_power.size()!=cross_power_re.size() || avg_power.size()!=cross_power_im.size() ){
      printf("ERROR : wrong size of output array !!!\n");
      exit(-1);
   }
   
   for(int i=0;i<((int)avg_power.size());i++){
      double freq_mhz = ((double)i)*0.01; // 10kHz channels
   
      avg_power[i] = avg_power[i] / index;
      avg_eda_power[i] = avg_eda_power[i] / index;            
      cross_power_re[i] = cross_power_re[i] / index;
      cross_power_im[i] = cross_power_im[i] / index;
      
      // cable correction applied to mean :
      complex<double> cross_power( cross_power_re[i] , cross_power_im[i] );
      
/*      if( !bCableCorrectAll ){
         // only if not applied to every single spectrum earlier :
         double phi_rad_eda = 2.00*M_PI*(freq_mhz/C_mhz)*cable_len_eda;
         complex<double> phase_corr_cable( cos(phi_rad_eda) , sin(phi_rad_eda) );  
         cross_power = cross_power * phase_corr_cable;
      }*/
      
      cross_power_re[i] = cross_power.real();
      cross_power_im[i] = cross_power.imag();
   }

   delete [] eda_buffer;
   delete [] bighorns_buffer;

   return index; // returns number of averaged spectra 
}


int CSpectrometer::CorrelateBinaryFloat( const char* eda_file, const char* bighorns_file, 
                                    vector<double>& avg_power, vector<double>& avg_eda_power,
                                    vector<double>& cross_power_re, vector<double>& cross_power_im,
                                    int spectrum_size, CBgFits* pCrossPowerFullTimeRes )
{
   printf("DEBUG : CorrelateBinaryFloat\n");
   FILE* eda_f = fopen(eda_file, "rb");
   if( !eda_f ){
      printf("ERROR : could not open file %s\n",eda_file);
      return -1;
   }
   printf("Opened EDA file %s\n",eda_file);
   
   FILE* bighorns_f = fopen(bighorns_file, "rb");
   if( !bighorns_f ){
      printf("ERROR : could not open file %s\n",bighorns_file);
      return -1;
   }
   printf("Opened BIGHORNS file %s\n",bighorns_file);

   avg_power.assign( spectrum_size, 0 );
   avg_eda_power.assign( spectrum_size, 0 );
   cross_power_re.assign( spectrum_size, 0 );
   cross_power_im.assign( spectrum_size, 0 );

   int single_spectrum_size = spectrum_size*2;
   float* eda_buffer = new float[single_spectrum_size];
   float* bighorns_buffer = new float[single_spectrum_size];

   int n_eda=0;
   int index=0;
   while( (n_eda = fread(eda_buffer, sizeof(float), single_spectrum_size, eda_f)) > 0 ){
      int n_bighorns = fread(bighorns_buffer, sizeof(float), single_spectrum_size, bighorns_f);
      
      if( index == 0 || index == 1000 ){
         printf("DEBUG(index=%d) : eda[0] = %.2f eda[1000] = %.2f , bighorns[0] = %.2f bighorns[1000] = %.2f\n",index,eda_buffer[0],eda_buffer[1000],bighorns_buffer[0],bighorns_buffer[1000]);fflush(stdout);
      }
      
       if( pCrossPowerFullTimeRes ){
          if( index >= pCrossPowerFullTimeRes->GetYSize() ){
             printf("INFO : doubling size of pCrossPowerFullTimeRes image\n");fflush(stdout);
             pCrossPowerFullTimeRes->Realloc( pCrossPowerFullTimeRes->GetXSize(), 2*pCrossPowerFullTimeRes->GetYSize() );
          }         
      }
   
      for(int i=0;i<spectrum_size;i++){
         double freq_mhz = ((double)i)*0.01; // 10kHz channels 
      
         double val_re = eda_buffer[2*i];
         double val_im = eda_buffer[2*i+1];

         double val2_re = bighorns_buffer[2*i];
         double val2_im = bighorns_buffer[2*i+1];

         double cable_len_bighorns = 0; // meters 
         double phi_rad_bighorns = 2.00*M_PI*(freq_mhz/300.00)*cable_len_bighorns;
         double sin_phi_bighorns = sin(phi_rad_bighorns);
         double cos_phi_bighorns = cos(phi_rad_bighorns);
         double val2_re_new = val2_re*cos_phi_bighorns - val2_im*sin_phi_bighorns;
         double val2_im_new = val2_re*sin_phi_bighorns + val2_im*cos_phi_bighorns;
         val2_re = val2_re_new;
         val2_im = val2_im_new;


         double cable_len_eda = CSpectrometer::m_EDA_ElectricalLenM; // meters - >= 250m change the direction of the fringes !
         double phi_rad_eda = 2.00*M_PI*(freq_mhz/300.00)*cable_len_eda;
         double sin_phi_eda = sin(phi_rad_eda);
         double cos_phi_eda = cos(phi_rad_eda);

         double val_re_new = val_re*cos_phi_eda - val_im*sin_phi_eda;
         double val_im_new = val_re*sin_phi_eda + val_im*cos_phi_eda;
         
         val_re = val_re_new;
         val_im = val_im_new;

/*         if( index==1 ){
            printf("channel=%d (%.2f MHz) : Phase due to cables BIGHORNS = %.2f [rad], EDA = %.2f [rad]\n",i,freq_mhz,phi_rad_bighorns,phi_rad_eda);
         }*/

         
         double product_re = val_re*val2_re + val_im*val2_im;
         double product_im = val_im*val2_re - val_re*val2_im;
         
         if( pCrossPowerFullTimeRes ){
            double cross_power = sqrt( product_re*product_re + product_im*product_im ); // CROSS POWER 
            // TEST 
            // double cross_power = val2_re*val2_re + val2_im*val2_im; // BIGHORNS
            // EDA double cross_power = val_re*val_re + val_im*val_im; // EDA 
            pCrossPowerFullTimeRes->setXY( i, index, cross_power );
         }
            
         // TODO : check in cotter !!!
         // double cable_len_diff = -5000.00; // meters difference between EDA and BIGHORNS cables 
         // double phi_rad = 2.00*M_PI*(freq_mhz/300.00)*cable_len_diff;
         // double sin_phi = sin(phi_rad);
         // double cos_phi = cos(phi_rad);
         //  double new_re = product_re*cos_phi - product_im*sin_phi;
         // double new_im = product_re*sin_phi + product_im*cos_phi;
 
         // cable correction :
         /*int gDoCableCorr=1;
         if( gDoCableCorr > 0 ){
            product_re = new_re;
            product_im = new_im;        
         }*/
            
         cross_power_re[i] += product_re;
         cross_power_im[i] += product_im;         
            
         double power = val2_re*val2_re + val2_im*val2_im;
         avg_power[i] += power;
         
         double power_eda = val_re*val_re + val_im*val_im;
         avg_eda_power[i] += power_eda;
      }      
      
      index++;
   }
   fclose(eda_f);
   fclose(bighorns_f);

   if( pCrossPowerFullTimeRes ){   
      printf("Setting size of full resulting output file to %d\n",index);
      pCrossPowerFullTimeRes->SetYSize(index);
   }
   
   if( avg_power.size()!=cross_power_re.size() || avg_power.size()!=cross_power_im.size() ){
      printf("ERROR : wrong size of output array !!!\n");
      exit(-1);
   }
   
   for(int i=0;i<((int)avg_power.size());i++){
      avg_power[i] = avg_power[i] / index;
      avg_eda_power[i] = avg_eda_power[i] / index;
      cross_power_re[i] = cross_power_re[i] / index;
      cross_power_im[i] = cross_power_im[i] / index;
   }

   delete [] eda_buffer;
   delete [] bighorns_buffer;

   return index; // returns number of averaged spectra 
}


int CSpectrometer::IntegrateFFT_Power( const char* binfile, vector<double>& avg_spectrum, int spectrum_size, int bytes_per_channel )
{
   FILE* f = fopen(binfile, "rb");
   if( !f ){
      printf("ERROR : could not open file %s\n",binfile);
      return -1;
   }
   printf("IntegrateFFT_Power : accumulating spectra from file %s with spectrum size = %d of %d bytes per channel\n",binfile,spectrum_size,bytes_per_channel);
   avg_spectrum.assign( spectrum_size, 0 );

   int single_spectrum_size = spectrum_size*bytes_per_channel; // assuming sizeof(short)
   unsigned char* buffer = (unsigned char*)(new char[single_spectrum_size]);

   int n_written=0,n=0;
   int index=0;
//   while( (n = fread(buffer, sizeof(unsigned char), single_spectrum_size, f)) > 0 ){
   while( (n = fread(buffer, bytes_per_channel, spectrum_size, f)) > 0 ){
      for(int i=0;i<spectrum_size;i++){
         short fft_reim = ((short*)buffer)[i];
         
         char re = ((unsigned char*)(&fft_reim))[0];
         char im = ((unsigned char*)(&fft_reim))[1];
         
         double power = re*re + im*im;
         avg_spectrum[i] += power;
      }      
      
      index++;
   }
   fclose(f);
   
   for(int i=0;i<((int)avg_spectrum.size());i++){
      avg_spectrum[i] = avg_spectrum[i] / index;
   }

   return n_written;   
      
}

int CSpectrometer::SkipNSpectraAndSaveMSpectra( const char* binfile, const char* out_bin_file, int skip_n_spectra, int save_n_spectra, int spectrum_size, int bytes_per_channel )
{
   FILE* f = fopen(binfile, "rb");
   if( !f ){
      printf("ERROR : could not open file %s\n",binfile);
      return -1;
   }
   FILE* out_binary_f = NULL;
   if( out_bin_file && strlen(out_bin_file) ){
      out_binary_f = fopen(out_bin_file,"wb");
   }
   
   printf("CSpectrometer::SkipNSpectraAndSaveMSpectra : requested to save %d spectra from file %s starting at %d spectrum (# fine channels = %d , bytes per channel = %d)\n",save_n_spectra,binfile,skip_n_spectra,spectrum_size,bytes_per_channel);


   int single_spectrum_size = spectrum_size*bytes_per_channel; // assuming sizeof(short)
   unsigned char* buffer = (unsigned char*)(new char[single_spectrum_size]);

   int n_written=0,n=0,n_written_bytes=0;
   int index=0;
   while( (n = fread(buffer, sizeof(unsigned char), single_spectrum_size, f)) > 0 ){
      if( index >= skip_n_spectra ){
         int ret = fwrite(buffer, sizeof(unsigned char), single_spectrum_size, out_binary_f );
         printf("Written %d spectrum to file %s\n",n_written,out_bin_file);
         n_written++;
         n_written_bytes += ret;
         if(  n_written > save_n_spectra ){
            printf("Saved %d spectra -> exiting the loop now\n",n_written);
            break;
         }
      }
      index++;
   }
   fclose(f);
   fclose(out_binary_f);

   printf("written %d lines (%d bytes) to file %s\n",n_written,n_written_bytes,out_bin_file);
   return n_written;   

}

int CSpectrometer::SkipNSec_and_SaveMSec( const char* binfile, const char* out_bin_file, double skip_n_sec, double save_n_sec, int skip_extra )
{
   FILE* f = fopen(binfile, "rb");
   if( !f ){
      printf("ERROR : could not open file %s\n",binfile);
      return -1;
   }
   FILE* out_binary_f = NULL;
   if( out_bin_file && strlen(out_bin_file) ){
      out_binary_f = fopen(out_bin_file,"wb");
   }
   

   unsigned char buffer[N_SAMPLES];
   // double mwa_clock = MWA_CLOCK_HZ;
   double line_time = ((double)N_SAMPLES)/((double)MWA_CLOCK_HZ);
   printf("Line time = %.10f [sec] = %.4f [msec]\n",line_time,line_time*1000.00);fflush(stdout);
   int lines_to_skip = skip_n_sec / line_time;
   int lines_to_write = save_n_sec / line_time;
   printf("Need to skip %d lines of %.8f sec each -> %.8f [sec]\n",lines_to_skip,line_time,skip_n_sec);fflush(stdout);
   printf("Then write %d lines of of %.8f sec each -> %.8f [sec]\n",lines_to_write,line_time,save_n_sec);fflush(stdout);

   int n_written=0,n_skipped=0,n=0;   
   
   while( (n = fread(buffer, sizeof(unsigned char), N_SAMPLES, f)) > 0 ){
       if( n == N_SAMPLES ){
          if( n_skipped < lines_to_skip ){
             n_skipped++;
          }else{
             if( n_skipped == lines_to_skip ){
                printf("Skipped %d sample-lines -> starting to write to output file\n",n_skipped);
                n_skipped++;
                if( skip_extra > 0 ){
                   int n_skip_extra = fread(buffer, sizeof(unsigned char), skip_extra, f);
                   printf("Skipped extra %d samples (read = %d)\n",skip_extra,n_skip_extra);
                   n = fread(buffer, sizeof(unsigned char), N_SAMPLES, f);
                }
             }
             if( n_written < lines_to_write ){
                int ret = fwrite(buffer, sizeof(unsigned char), N_SAMPLES, out_binary_f );
                if( ret != N_SAMPLES ){
                   printf("ERROR : while writing %d bytes to output file %s\n",N_SAMPLES,out_bin_file);fflush(stdout);
                }
                n_written++;
             }else{
                printf("Written %d lines to output file -> exiting the loop\n",n_written);
                break;
             }
          }
       }
   }
   fclose(f);
   fclose(out_binary_f);

   printf("written %d lines of %.2f [msec] -> total time = %.8f [sec]\n",n_written,line_time*1000.00,(n_written*line_time));
   return n_written;   
}
