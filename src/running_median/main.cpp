#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <bg_globals.h>
#include <bg_fits.h>
#include <bg_array.h>
#include <libnova_interface.h>

#include <vector>
using namespace std;

#include <basestring.h>
#include <myfile.h>
#include <myparser.h>
#include <mystrtable.h>
#include <weather_station.h>

int gVerb=1;

BaseString text_file     = "B0950.txt";
BaseString out_file_base = "B0950_median";
BaseString out_file      = "B0950_median.txt";
BaseString out_file_diff = "B0950_median_subtr.txt";
BaseString gParam;

enum eAvgType_T { eMedian=0, eMean=1 };
eAvgType_T gAvgType=eMedian;
int gHalfMedianPoints=30;
bool gIncludeCentral=false;
double gMaxDiffInRMSIQR = -10000;
int gYCol=1;
int gRunningRMSCount=20;
int gMedianDirection=0; // means +/- 5 values around current value


double gStepThresholdInRunningRMSs = 10;
double gRMSChangeThresholdInRunningRMSs = 10;

void usage()
{
   printf("running_median text_file out_medianfile -a -n N_HALF_MEDIAN_POINTS -f MAX_DIFF_IN_SIGMA\n");
   printf("-a : use mean instead of default median\n");
   printf("-i : include data point under test (central point) [default not include]\n");
   printf("-n N_HALF_MEDIAN_POINTS : half of number of median points +/- N_HALF_MEDIAN_POINTS around the value [default %d]\n",gHalfMedianPoints);
   printf("-Y Y_COLUMN : y column [default %d]\n",gYCol);
   printf("-R COUNT_IN_RUNNING_RMS : number of values in running RMS [default %d]\n",gRunningRMSCount);
   printf("-D Median direction : 0 - both before and after, -1 only before, +1 only after\n");
   exit(-1);
}

void print_parameters()
{
   printf("#####################################\n");
   printf("PARAMETERS :\n");
   printf("#####################################\n");
   printf("input file      = %s\n",text_file.c_str());
   printf("out   file base = %s\n",out_file_base.c_str());   
   printf("out file median = %s\n",out_file.c_str());
   printf("out file median subtractd = %s\n",out_file_diff.c_str());
   printf("average type = %s\n",(gAvgType==eMedian ? "MEDIAN" : "MEAN"));
   printf("Median/average radius = %d\n",gHalfMedianPoints);
//   printf("Test frequency range = %.2f - %.2f [MHz]\n",gMinFreq,gMaxFreq);
   printf("Include central = %d\n",gIncludeCentral);
   printf("Excise were different larger than %.4f * RMS_IQR\n",gMaxDiffInRMSIQR);
   printf("Y column        = %d\n",gYCol);
   printf("Running RMS count = %d\n",gRunningRMSCount);
   printf("Median direction  = %d\n",gMedianDirection);
   printf("Thresholds for a step :\n");
   printf("\tThreshold for difference = %.2f\n",gStepThresholdInRunningRMSs);
   printf("\tThreshold for RMS change = %.2f\n",gRMSChangeThresholdInRunningRMSs);
   printf("#####################################\n");   
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "haq:n:if:Y:R:D:";
   int opt,opt_param,i;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
      switch (opt) {
         case 'f':
            gMaxDiffInRMSIQR = atof( optarg );
            break;

         case 'i':
            gIncludeCentral = true;
            break;

         case 'a':
            gAvgType=eMean;
            break;

         case 's':
            if( optarg ){
//               gMinFreq = atof(optarg);
            }
            break;

         case 'D':
            if( optarg ){
               gMedianDirection = atol(optarg);
            }
            break;

         case 'n':
            if( optarg ){
               gHalfMedianPoints = atol(optarg);
            }
            break;

         case 'Y':
            if( optarg ){
               gYCol = atol(optarg);
            }
            break;

         case 'R':
            if( optarg ){
               gRunningRMSCount = atol(optarg);
            }
            break;
                        
         case 'h':
            printf("HELP NEEDED ???:\n");
            usage();
            break;
            
         default:  
            fprintf(stderr,"Unknown option %c\n",opt);
            usage();
      }
   }
}                                                                                                                                                                                                                                                

double calc_running_rms( vector<double>& running_rms )
{
   my_sort_float( running_rms, running_rms.size() );

   double running_rms_value = running_rms[running_rms.size()/2];
   
   return running_rms_value;      
}

double add_running_rms( vector<double>& running_rms, double rms, int& current_index )
{
   int next_index = current_index + 1;
   if( running_rms.size() < gRunningRMSCount ){
      running_rms.push_back( rms );
      current_index = running_rms.size()-1;
   }else{
      if( next_index >= running_rms.size() ){
         next_index = 0;            
      }
      
      running_rms[next_index] = rms;
      current_index = next_index;
   }

   double running_rms_out = calc_running_rms( running_rms );
   return running_rms_out;   
}

int main(int argc,char* argv[])
{
  if( argc<3 || strncmp(argv[1],"-h",2)==0 ){
     usage();
  }  
  if( argc >= 2 ){
     text_file = argv[1];
  }
  if( argc >= 3 ){
     out_file_base = argv[2];
  }
  out_file      = out_file_base + "_median.txt";
  out_file_diff = out_file_base + "_median_subtr.txt";
  
  
  // parse command line :
  parse_cmdline(argc-2,argv+2);
  print_parameters();
  

  vector<CValueVector> txtfiles;
  vector<double> running_rms;
  int current_index=-1;
  
  CValueVector txtfile;
  int n = txtfile.read_file( text_file.c_str() , 0, 0, gYCol );
  
  for(int i=0;i<n;i++){
     vector<double> values;

     if( gMedianDirection == 0 ){     
        for(int k=(i-gHalfMedianPoints);k<(i+gHalfMedianPoints+1);k++){
           if( k>=0 && k<n ){
              if( k != i || gIncludeCentral ){
                 values.push_back( txtfile[k].y );
              }
           }
        }
     }else{
        if( gMedianDirection < 0 ){
           for(int k=(i-2*gHalfMedianPoints-1);k<=i;k++){
              if( k>=0 && k<n ){
                 if( k != i || gIncludeCentral || i==0 ){
                    values.push_back( txtfile[k].y );
                 }
              }
           }
        }
     }
     
     if( values.size() > 0 ){
        my_sort_float( values, values.size() );
        int half = values.size()/2;
     
        txtfile[i].z = values[half];
        
        int q75 = int(values.size()*0.75);
        int q25 = int(values.size()*0.25);
        double iqr = values[q75] - values[q25];
        double rms_iqr = iqr/1.35;
        txtfile[i].err = rms_iqr;
        
        // running rms here :
//        if( running_rms <= 0 ){
//           running_rms = txtfile[i].z; // set median value;
//        }
        if( i < 10 ){
           printf("DEBUG : rms_iqr = %.8f , current_index = %d\n",rms_iqr,current_index);
        }
        txtfile[i].v = add_running_rms( running_rms , rms_iqr, current_index );
        
//        printf("DEBUG : %d %.4f\n",i,txtfile[i].z);
     }else{
        printf("ERROR : bug in code ???? no values in sorted table ???\n");
        txtfile[i].z = -1000;
     }
  }

  MyOFile outf(out_file.c_str(),"w");
  outf.Printf("# TIMEIDX RunningMedian Value Difference RMS_IQR  STEP[?] DIFF RUNNING_RMS\n");
  double prev_median_value = txtfile[0].z;
  double prev_running_rms_value = txtfile[0].v;
  for(int i=0;i<n;i++){
     cValue& val = txtfile[i];
     bool ok = true;
     
     if( gMaxDiffInRMSIQR > 0 && val.err > 0 ){
        double diff_rms = fabs(val.y-val.z) / val.err;
        if( diff_rms > gMaxDiffInRMSIQR ){
           printf("Rejected uxtime = %.2f\n",val.x);
           ok = false;
        }
     }
     
     // finding steps :
     double diff = val.y - prev_median_value;
     bool step = false;
     if( fabs( diff )/prev_running_rms_value > gStepThresholdInRunningRMSs && (val.err/prev_running_rms_value) > gRMSChangeThresholdInRunningRMSs && i>=gHalfMedianPoints ){
        printf("DEBUG : Step detected at index %d : %.4f -> %.4f , RMS change from %.4f -> %.4f\n",i,prev_median_value,val.y,prev_running_rms_value,val.err);
        step = true;
     }
     if( ok ){  
        outf.Printf("%.8f %.8f %.8f %.8f %.8f %d %.4f %.4f\n",val.x,val.z,val.y,(val.y-val.z),val.err,step,diff/prev_running_rms_value,prev_running_rms_value);
     }
     
     prev_median_value = val.z;
     prev_running_rms_value = val.v;
//     if( i >= ( gHalfMedianPoints + 1 ) ){
//        prev_median_value = txtfile[i-gHalfMedianPoints-1].z;
//        prev_running_rms_value = txtfile[i-gHalfMedianPoints-1].v;
//     }
  }
  printf("Running median of text file %s written to output file %s\n",text_file.c_str(),out_file.c_str());

  MyOFile outf_diff(out_file_diff.c_str(),"w");
  for(int i=0;i<n;i++){
     cValue& val = txtfile[i];
     bool ok = true;

     if( gMaxDiffInRMSIQR > 0 && val.err > 0 ){
        double diff_rms = fabs(val.y-val.z) / val.err;
        if( diff_rms > gMaxDiffInRMSIQR ){
           ok = false;
        }
     }
     
     if( ok ){       
        outf_diff.Printf("%.8f %.8f %.8f\n",val.x,(val.y-val.z),val.err);
     }
  }
  printf("RunningMedian-subtracted values of text file %s written to output file %s\n",text_file.c_str(),out_file_diff.c_str());

  
/*     if( n > 0 ){
        printf("%d : Read %d values from file %s\n",i,n,fits_list[i].c_str());
     }
     if( n_channels < 0 ){
        n_channels = n;
     }
     if( n != n_channels ){
        printf("ERROR : files of different lenght, the situation not yet handled -> exiting program now\n");
     }
     txtfiles.push_back(txtfile);
  }                                    
  
  CValueVector& spectrum0 = txtfiles[0];
  CValueVector median_spectrum;
  double* tmp_buffer = new double[txtfiles.size()];
  for(int ch=0;ch<n_channels;ch++){
     double mean=0.00,sum2=0.00;;
     for(int f=0;f<txtfiles.size();f++){
        CValueVector& spectrum = txtfiles[f];
        
        tmp_buffer[f] = spectrum[ch].y;
        mean += tmp_buffer[f];
        sum2 += (tmp_buffer[f])*(tmp_buffer[f]);
     }

     double median = mean/txtfiles.size();
     double rms    = sqrt( sum2/txtfiles.size() - median*median );

     if( gAvgType == eMedian ){
        my_sort_float( tmp_buffer, txtfiles.size() );
        median = tmp_buffer[txtfiles.size()/2];
     }
     
     cValue median_value;
     median_value.x = spectrum0[ch].x;
     median_value.y = median;
     median_value.z = rms;
     median_spectrum.push_back(median_value);
  }

  MyOFile outf(out_file.c_str(),"w");
  for(int ch=0;ch<median_spectrum.size();ch++){
     outf.Printf("%.8f %.8f %.8f\n",median_spectrum[ch].x,median_spectrum[ch].y,median_spectrum[ch].z);
  }
  printf("Median of %d text files written to output file %s\n",txtfiles.size(),out_file.c_str());

  // subtract every file from median to verify if median is strongly BIASED towards one particular file (max number of channels from this file used)
  printf("Checking number of zeros:\n");
  int max_same_channels=-1e6;
  string median_similar_file;  
  for(int f=0;f<txtfiles.size();f++){
     CValueVector& spectrum = txtfiles[f];
     CValueVector diff_spectrum;
     spectrum.subtract( median_spectrum, diff_spectrum );
     int zeros_count = diff_spectrum.count_zeros(gMinFreq,gMaxFreq,0.0001);
     printf("\t%s - %d zeros\n",fits_list[f].c_str(),zeros_count);
     if( zeros_count > max_same_channels ){
        max_same_channels = zeros_count;
        median_similar_file = fits_list[f].c_str();
     }
  } 
  
  printf("Median calculated of of %d files\n",txtfiles.size());
  printf("Maximum number of channels close to median file is %d in file %s , median out of %d files\n",max_same_channels,median_similar_file.c_str(),txtfiles.size());
*/
}
