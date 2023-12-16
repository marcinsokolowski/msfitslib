#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <bg_globals.h>
#include <bg_fits.h>
#include <bg_array.h>
#include <bg_bedlam.h>

#include <myfile.h>
#include <mystring.h>

#include <vector>
using namespace std;

// to check maximum stack limits:
#include <sys/time.h>
#include <sys/resource.h>

// enum eCalcFitsAction_T  {eNone=0,eAdd,eSubtract,eMultiply,eDivide,eSubtractLines,eCompare,eGetStat,eDivideConst, eLog10File, eSqrtFile, eAstroRootImage };

string fits_left="left.fits";
string fits_right="right.fits";
eCalcFitsAction_T action=eGetStat;
eFindValueType_T  gFindValueType=eFindValueExact;
string fits_out="out.fits";
int do_save_out=0,dual_file=1;
int param1=0,param2=0;
int gStartInt=0;
int gEndInt=-1;
string gInttypeKeyword;
string gOutputDir;
double gParamValue=-10000;
string gRFI_FlagFile;
int gUseMedian=0;
int gOutChannels=0;
double constValue=1.00;
int gRadius=-1000;

double gSubtractConstAfter=0;

void usage()
{
   printf("calcfits_bg FITS_LEFT ACTION FITS_RIGHT OUTPUT_FILE[default out.fits] -s START_INT -e END_INT -k INTTYPE -o OUTDIR -d -p PARAM -r RFI_FLAGS_FITS_FILE(in ao-flagger format) -a SUBTRACT_CONST_AFTER -m\n");
   printf("-d : increases devug level\n");
   printf("-m : uses median for statistics option (s)\n");
   printf("-p PARAM : parameter value\n");
   printf("-R RMS_RADIUS around center. For other positions put X and Y coordinates into FITS_RIGHT and OUTPUT_FILE (after action s, for example calcfits_bg test.fits s 1400 1500)\n");
   printf("ACTION :\n");
   printf("Two fits files actions  : +,-,/,*,=,sefd_xx_yy,stokes_i,sefd2aot\n");   
   printf("Single fits file actions : \n");
   printf("\t\t\t l - subtract 2 integrations, d - divide 2 integrations, log, s - statistics, i invert, a abs, sqrt , x - times constant (provide in -v option), n - normalize by median integration, f - find value (default exact, but f< finds smaller, f> finds larger), h - dump values to output file for histograming etc, v - normalise by external spectrum in TXTFILE, t - subtract spectrum from text file, -p print pixel ( X Y ) , m - magnitude left=REAL, right=IMAG, output=MAG.fits \n");
   printf("\t\t\t z - subtract median, dB (lin -> dB), log10, 0 - find zero integrations, lin (dB->lin) \n");
   printf("EXAMPLES :\n");
   printf("\tcalcfits_bg test.fits l 23 373\n");
   exit(-1);
}

void print_parameters()
{
   printf("#####################################\n");
   printf("PARAMETERS :\n");
   printf("#####################################\n");
   printf("FITS_LEFT  = %s\n",fits_left.c_str());
   printf("action     = %d (find = %d)\n",action,gFindValueType);
   if( dual_file ){
      printf("FITS_RIGHT = %s\n",fits_right.c_str());
   }else{
      printf("param1     = %d\n",param1);
      printf("param2     = %d\n",param2);
      
   }
   printf("OUT_FITS   = %s\n",fits_out.c_str());
   printf("OUTDIR     = %s\n",gOutputDir.c_str());
   printf("State      = %s\n",gInttypeKeyword.c_str());
   printf("Integration range : %d - %d\n",gStartInt,gEndInt);
   printf("Param value = %.4f\n",gParamValue);
   printf("AO-flagger file = %s\n",gRFI_FlagFile.c_str());
   printf("Subtract constant after = %.2f\n",gSubtractConstAfter);
   printf("Use median   = %d\n",gUseMedian);
   printf("Out channels = %d\n",gOutChannels);
   printf("Constant value = %.4f\n",constValue);
   printf("Radius       = %d\n",gRadius);
   printf("#####################################\n");   
}

void parse_cmdline(int argc, char * argv[]) {
   char optstring[] = "mcdhs:e:k:o:p:r:a:v:R:";
   int opt,opt_param,i;
        
   while ((opt = getopt(argc, argv, optstring)) != -1) {
      switch (opt) {
         case 'd':
            gBGPrintfLevel++;
            break;
            
         case 's':
            if( optarg ){
               gStartInt = atol(optarg);
            }
            break;
            
         case 'c':
            gOutChannels = 1;
            break;
            
         case 'e':
            if( optarg ){
               gEndInt = atol(optarg);
            }
            break;
            
         case 'k':
            if( optarg ){
               gInttypeKeyword = optarg;
            }
            break;
            
         case 'o':
            if( optarg ){
               gOutputDir = optarg;
            }
            break;
            
         case 'p':
            if( optarg ){
               gParamValue = atof(optarg);               
            }
            break;
            
         case 'r':
            if( optarg ){
               gRFI_FlagFile = optarg;
            }
            break;

         case 'R':
            if( optarg ){
               gRadius = atol( optarg );
            }
            break;
            
         case 'a':
            if( optarg ){
               gSubtractConstAfter = atof(optarg);
            }
            break;
         case 'm':
            gUseMedian = 1;
            break;

         case 'v':
            constValue = atof(optarg);
            break;
            
         case 'h':
            usage();
            break;
         default:  
            fprintf(stderr,"Unknown option %c\n",opt);
            usage();
      }
   }
   
   if( strlen(gOutputDir.c_str()) > 0 ){
      string szTmp = fits_out.c_str();
      fits_out = gOutputDir.c_str();
      fits_out += "/";
      fits_out += szTmp.c_str();
   }
}
                                                        
eCalcFitsAction_T parse_action( const char* szAction, const char* argv3 )
{
   printf("Parsing action = %s\n",szAction);
   eCalcFitsAction_T ret = eNone;
   if( szAction && strlen(szAction) ){
      if( szAction[0] == '+' ){
         if( atol(argv3) == 0 ){
            ret = eAdd;
         }else{
            ret = eAddConst;
         }                              
      }   
      if( szAction[0] == 'B' ){
         ret = eAvgImages;
      }   
      if( szAction[0] == 'p' ){
         ret = ePrintPixelValue;
      }   
      if( szAction[0] == '-' ){
         ret = eSubtract;
      }   
      if( szAction[0] == '/' ){
         ret = eDivide;
      }   
      if( szAction[0] == '*' ){
         ret = eMultiply;
      }   
      if( szAction[0] == '=' ){
         ret = eCompare;
      }   
      if( strcmp( szAction, "sefd_xx_yy" ) == 0  ){
         ret = eSEFD_XX_YY;
      }   
      if( strcmp( szAction, "sefd2aot" ) == 0  ){
         ret = eSEFD_TO_AOT;
      }   
      if( strlen(szAction)==1 && (szAction[0] == 'l' || szAction[0] == 'L') ){
         ret = eSubtractLines;
      }   
      if( strlen(szAction)==1 && (szAction[0] == 'c' || szAction[0] == 'C') ){
         ret = eAvgChannels;
      }   
      if( strlen(szAction)==1 && (szAction[0] == 'd' || szAction[0] == 'D') ){
         ret = eDivideLines;
      }   
      if( szAction[0] == 'm' || szAction[0] == 'M' ){
         ret = eComplexMag;
      }   
      if( strlen(szAction)==1 && (szAction[0] == 'x' || szAction[0] == 'X') ){
         ret = eTimesConst;
      }   
      if( strlen(szAction)==1 && (szAction[0] == 't' || szAction[0] == 'T') ){
         ret = eSubtractSpectrum;
      }   
      if( strlen(szAction)==1 && (szAction[0] == 'v' || szAction[0] == 'V') ){
         ret = eDivbySpectrum;
      }   
      if( strlen(szAction)==1 && (szAction[0] == 'n' || szAction[0] == 'N') ){
         ret = eNormalizeByMedian;
      }   
      if( strlen(szAction)==1 && (szAction[0] == 'z' || szAction[0] == 'Z') ){
         ret = eSubtractMedian;
      }   
      if( strlen(szAction)<=2 && (szAction[0] == 'f' || szAction[0] == 'F') ){
         ret = eFindValue;
         gFindValueType = eFindValueExact;
         if( strlen(szAction) == 2 ){
            if( szAction[1] == '<' ){
               gFindValueType = eFindValueSmaller;
            }
            if( szAction[1] == '>' ){
               gFindValueType = eFindValueLarger;
            }
         }
      }   
      if( strlen(szAction)==1 && (szAction[0] == 'h' || szAction[0] == 'H') ){
         printf("DUMP FOR HISTOGRAM REQUIRED !\n");
         ret = eDumpForHisto;
      }   
      if( strcmp(szAction,"log10")==0 || strcmp(szAction,"log")==0 ){
         ret = eLog10File;
      }
      if( strcmp(szAction,"dB")==0 || strcmp(szAction,"db")==0 || strcmp(szAction,"dB")==0 ){
         ret = eDBFile;
      }
      if( strcmp(szAction,"LIN")==0 || strcmp(szAction,"Lin")==0 || strcmp(szAction,"lin")==0 ){
         ret = eLin2DB;
      }
      if( strcmp(szAction,"image")==0 ){
         ret = eAstroRootImage;
      }
      if( strlen(szAction)==1 && szAction[0] == 's' || szAction[0] == 'S' ){
         ret = eGetStat;         
      }else{
         if( strcmp(szAction,"sqrt")==0 ){
            ret = eSqrtFile;
         }else{
            if( szAction[0] == 'i' || szAction[0] == 'I' ){
               ret = eInvert;
            }else{
               if( szAction[0] == 'a' || szAction[0] == 'A' ){
                  ret = eABS;
               }
            }
         }
      }
      if( strlen(szAction)==1 && szAction[0] == '0' || szAction[0] == '0' ){
         ret = eFindZeroInt;
      }
      
   }
   
   return ret;
}                                                                 
       
                                                                                                                  

int main(int argc,char* argv[])
{
  if( argc<2 || (argc>=2 && (strcmp(argv[1],"-h")==0 || strcmp(argv[1],"--h")==0)) ){
     usage();
  }

  fits_left = argv[1];
  if( argc>=3 ){
     action = parse_action( argv[2], argv[3] );
  }
  if( action == eSubtractLines || action == eDivideLines || action == eGetStat || action == eLog10File || action == eSqrtFile || action == eAstroRootImage || action==eTimesConst || action==eNormalizeByMedian || action==eAddConst ||
      action==eSubtractMedian || action==eFindValue || action==eDumpForHisto ||
      action == eAvgChannels || action == eSubtractSpectrum || action == eDivbySpectrum  || action == eDBFile || action==eLin2DB || action == eFindZeroInt || action == ePrintPixelValue || action == eInvert || action == eABS 
    ){
     dual_file=0;
  }
  if( (action == eDivide || action==eAddConst) && 
      argc>=4 && !strstr(argv[3],"fit") && atof(argv[3])!=0 ){
     dual_file=0;
     if( action == eDivide ){
        action = eDivideConst;
     }
     constValue=atof(argv[3]);
     printf("constValue = %e\n",constValue);
  }
  if( action != eSubtractLines && action != eCompare && action != eGetStat ){
     do_save_out = 1;
  }

  string szFitsRight;
  string szFitsOut;
  
  if( argc>=4 ){
     szFitsRight = argv[3];
  }
  if( argc>=5 ){
     szFitsOut = argv[4];
  }

  if( dual_file ){
     if( argc >= 4 ){
        fits_right = argv[3];
     }
     if( argc >= 5 ){
        fits_out = argv[4];
     }
  }else{
     if( argc >= 4 ){
        param1 = atol(argv[3]);
     }
     if( argc >= 5 ){ // && strstr(argv[4],".fit") ){
        fits_out = argv[4];
     }else{
        if( argc>=5 ){
           param2 = atol(argv[4]);
        }
     }
  }

  // parse command line :
  parse_cmdline(argc-4,argv+4);
  print_parameters();
  
  CBgFits left(fits_left.c_str());
  CBgFits right(fits_right.c_str());      
  
  printf("Reading file %s ...\n",fits_left.c_str());
  if( left.ReadFits( NULL, 0, 1, 1) ){
     printf("ERROR : error reading fits file %s\n",fits_left.c_str());
     exit(-1);
  }
  
  struct rlimit rlim;
  getrlimit( RLIMIT_STACK, &rlim);
  printf("SYSTEM INFO : maximum stack size = %ld bytes (soft limit) and %ld bytes of hard limit\n",rlim.rlim_cur,rlim.rlim_max);
  if( left.GetXSize() >= 4000 ){
     rlim.rlim_cur = 33554432;
     setrlimit( RLIMIT_STACK, &rlim);
     printf("SYSTEM INFO : maximum stack size set to %ld bytes\n",rlim.rlim_cur);
  }

  CBgFits* pRFI_FlagsFits = NULL;
  if( strlen(gRFI_FlagFile.c_str()) > 0 ){
     pRFI_FlagsFits = new CBgFits( gRFI_FlagFile.c_str());
     if( pRFI_FlagsFits->ReadFits( gRFI_FlagFile.c_str() ) ){
        printf("ERROR : error reading RFI-flags fits file %s\n",gRFI_FlagFile.c_str());
        exit(-1);
     }     
  }

  if( dual_file ){
     printf("2-files action ...\n");fflush(stdout);
     if( right.ReadFits( NULL, 0, 1, 1 ) ){
        printf("ERROR : error reading fits file %s\n",fits_right.c_str());
        exit(-1);
     }

     if( action == eDivide ){
        left.Divide( right );     
     }

     if( action == eMultiply ){
        left.Multiply( right );
     }
     
     if( action == eSubtract ){
        left.Subtract( right );
     }
     if( action == eSEFD_XX_YY ){
        left.SEFD_XX_YY( right );
     }
     if( action == eSEFD_TO_AOT ){
        left.SEFD2AOT();
     }
     if( action == eAvgImages ){
        printf("Averaging images -> calling AddImages( right , 0.5 )\n");
        left.AddImages( right , 0.5 );
     }
     
     if( action == eComplexMag ){
        left.ComplexMag( right );
     }
     
     if( action == eCompare ){
        double min_diff=0.00001;
        if( gParamValue >= 0 ){
           min_diff = gParamValue;
        }
        int ret = left.Compare(right, min_diff, gBGPrintfLevel);
        printf("\n\nCOMPARISON RESULT : ");
        if( ret ){
           printf("Images differ by %d pixels\n",ret);
        }else{
           printf("Images are EQUAL !!!\n");
        }
     }
  
     if( do_save_out ){
        if( left.WriteFits( fits_out.c_str() ) ){
           printf("ERROR : could not write output file %s\n",fits_out.c_str());
        }else{
           printf("OK : written the output FITS file %s\n",fits_out.c_str());
        }        
     }
  }else{
    printf("1-file action ...\n");fflush(stdout);
    
    if( action == ePrintPixelValue )    
    {
       int x = int(param1); // atol( fits_right.c_str() );
       int y = atol( fits_out.c_str() );
       
       double val = left.getXY( x , y );
       printf("Value at (%d,%d) = %.8f\n",x,y,val);       
    }
    
    
    if( action == eSubtractLines ){
       char outfile[1024];

       string fits_base;
       getbasename_new(fits_left,fits_base);
       sprintf(outfile,"%s_diff_%05d_minus_%05d.txt",fits_base.c_str(),param1,param2);
       fits_out =  outfile;

       printf("Saving difference of lines %d - %d to file %s\n",param1,param2,fits_out.c_str());
       left.SubtractLines( param1, param2, fits_out.c_str() );
    }          

    if( action == eDivideLines ){
       char outfile[1024];
       sprintf(outfile,"ratio_%05d_div_%05d.txt",param1,param2);
       printf("Saving ratio of lines %d - %d to file %s\n",param1,param2,outfile);
       left.DivideLines( param1, param2, outfile );
    }          

    if( action == eAvgChannels ){
       right.Realloc(left.GetXSize(),left.GetYSize());
       left.AvgChannels( param1, right ); // store resulting image in right 
       printf("Writing image with %d channels averaged to file %s\n",param1,fits_out.c_str());
       
       right.SetKeys( left.GetKeys() );
       if( right.WriteFits( fits_out.c_str() ) ){
          printf("ERROR : could not write output file %s\n",fits_out.c_str());
       }                          
    }

    if( action == eDumpForHisto ){
       MyOFile out_txt(fits_out.c_str(),"w");
       printf("Dumping pixel values to output file %s ...",fits_out.c_str());fflush(stdout);      
       char buff[1024];
       for(int y=0;y<left.GetYSize();y++){
          mystring szLine;
          for(int x=0;x<left.GetXSize();x++){
             sprintf(buff,"%.4f %.20f\n",left.ch2freq(x),left.getXY(x,y));
             szLine << buff;
//             out_txt.Printf("%.20f\n",left.getXY(x,y));
        
          }
          out_txt.Printf("%s",szLine.c_str());
       }
       printf("OK\n");
    }
    
    if( action == eFindZeroInt ){
       printf("--------------------------\n");
       printf("List of ZERO integrations:\n");
       printf("--------------------------\n");
       for(int y=0;y<left.GetYSize();y++){
          int is_zero=1;
          
          for(int x=0;x<left.GetXSize();x++){
             if( left.getXY(x,y) != 0 ){
                is_zero = 0;
                break;
             }
          }
          if( is_zero > 0 ){
             printf("%d\n",y);
          }
       }
       printf("--------------------------\n");
    }
    if( action == eGetStat ){
       CBgArray avg,rms,min_spec,max_spec;
       int start_int=0;
       int end_int=left.GetYSize();

       printf("GetStat StartInt=%d, EndInt=%d , state=|%s|\n",gStartInt, gEndInt,gInttypeKeyword.c_str() );       
       
       double inttime = 0;
       if( gRadius > 0 ){
          // radius gParamValue pixels around :
          double avg_val, rms_val, min_val, max_val, median, iqr, rms_iqr;
          int stat_x = -1, stat_y = -1;
          int pixel_count=0;  
          if( strlen( szFitsRight.c_str() ) > 0 && atof( szFitsRight.c_str() ) > 0 ){
             stat_x = atol( szFitsRight.c_str() );
          }
          if( strlen( szFitsOut.c_str() ) > 0 && atof( szFitsOut.c_str() ) > 0 ){
             stat_y = atol( szFitsOut.c_str() );
          }
          inttime = left.GetStatRadiusAll( avg_val, rms_val, min_val, max_val, median, iqr, rms_iqr, pixel_count, gRadius, true, stat_x, stat_y );
          printf("%s : MEDIAN = %.8f, RMS_IQR = %.8f , MEAN = %.8f , RMS = %.8f , MIN_VAL = %.8f , MAX_VAL = %.8f in N_PIXELS = %d around pixel (%d,%d)\n",left.GetFileName(), avg_val, rms_val, median, rms_iqr, min_val, max_val, pixel_count, stat_x, stat_y );
       }else{
          // full image
          inttime = left.GetStat( avg, rms, gStartInt, gEndInt, gInttypeKeyword.c_str(), &min_spec, &max_spec, -1e20, NULL, pRFI_FlagsFits );
       }
       
       if( gUseMedian > 0 ){
          vector<cIntRange> int_ranges;
          printf("Use MEDIAN/IQR instead of MEAN/RMS\n");
          if( strcmp(gInttypeKeyword.c_str(),"ANT")==0 ){
             printf("Using only ANT integrations for median !\n");
             left.GetAntRanges( int_ranges );
             printf("%d ANT ranges detected\n",int(int_ranges.size()));
          }else{
             int_ranges.push_back( cIntRange(0,(left.GetYSize()-1)) );
          }                     
          left.GetMedianInt( int_ranges, avg, rms );
       }
       
       string out_file_name,out_file_name_rms,out_file_name_all, out_mean_column_file;
       change_ext(fits_left.c_str(),"avg_and_rms",out_file_name);
       change_ext(fits_left.c_str(),"rms",out_file_name_rms);       
       change_ext(fits_left.c_str(),"stat",out_file_name_all);
       change_ext(fits_left.c_str(),"mean_spectrum", out_mean_column_file );
       
       if( argc >= 4 && strcmp(argv[3],"-") ){
          out_file_name = argv[3];
          change_ext(out_file_name.c_str(),"rms",out_file_name_rms);
          change_ext(out_file_name.c_str(),"stat",out_file_name_all);
       }

       string out_file_name_full=gOutputDir.c_str(),out_file_name_rms_full=gOutputDir.c_str();
       if( strlen(gOutputDir.c_str()) ){
          out_file_name_full += "/";
          out_file_name_rms_full += "/";
       }
       out_file_name_full += out_file_name.c_str();
       out_file_name_rms_full += out_file_name_rms.c_str();
       
       MyOFile out_file( out_file_name_full.c_str() , "w" );
       MyOFile out_file_rms( out_file_name_rms_full.c_str() , "w" );
       out_file.Printf("# FREQ[MHz] AVG[BEDLAM] RMS[BEDLAM] MIN[BEDLAM] MAX[BEDLAM] AVG[dBm] MIN[dBm] MAX[dBm]\n");
       out_file_rms.Printf("# FREQ[MHz] RMS[BEDLAM]\n");
       for(int ch=0;ch<avg.size();ch++){
          double freq = left.ch2freq(ch);
          if ( gOutChannels > 0 ){
             freq = ch;
          }
          double avg_dbm = CBedlamSpectrometer::power2dbm( freq, avg[ch] );
          double min_dbm = CBedlamSpectrometer::power2dbm( freq, min_spec[ch] );
          double max_dbm = CBedlamSpectrometer::power2dbm( freq, max_spec[ch] );
       
          out_file.Printf("%e %.20f %.20f %.20f %.20f %.8f %.8f %.8f\n",freq,avg[ch],rms[ch],min_spec[ch],max_spec[ch],avg_dbm,min_dbm,max_dbm);
          out_file_rms.Printf("%e %.20f\n",freq,rms[ch]);          
       }
       printf("Integration time = %.4f [sec]\n",inttime);
       printf("Saved %d points of AVG and RMS spectrum to files %s and %s\n",(int)avg.size(),out_file_name_full.c_str(),out_file_name_rms_full.c_str());
       
       CBgArray mean_lines, rms_lines;
       left.MeanLines( mean_lines, rms_lines );
       mean_lines.SaveToFile( out_mean_column_file.c_str(), NULL, &rms_lines );
    }
    if( action == eDivideConst || action == eLog10File || action == eSqrtFile || action == eAstroRootImage || action==eTimesConst || action==eNormalizeByMedian || action==eSubtractMedian || action==eFindValue ||
        action==eSubtractSpectrum || action==eDivbySpectrum || action==eDBFile || action==eLin2DB || action==eAddConst || action==eInvert || action==eABS ){    
       if( action == eTimesConst ){
          printf("Multiplying by const = %e!\n",constValue);
          left.Recalc( action, constValue );
       }

       if( action == eInvert ){
          printf("Calculating 1/image\n");
          left.Recalc( action, constValue );
       }

       if( action == eABS ){
          printf("Calculating ABS(image) = |image|\n");
          left.Recalc( action, constValue );
       }

       if( action == eAddConst ){
          printf("Adding const = %e!\n",constValue);
          left.Recalc( eAddConst, constValue );
       }

       if( action == eFindValue ){
          double val = atof(argv[3]);
          double delta=0.00001; // 1000.00;
          if( argc >= 5 ){
             delta = atof(argv[4]);
          }
          printf("DEBUG : looking for value = %.8f +/- %.8f\n",val,delta);
          int found = left.FindValue(val,delta,gFindValueType);
          printf("Number of occurences = %d\n",found);
          exit(0);
       }

       if( action == eNormalizeByMedian || action == eSubtractMedian ){ // or just by integration
//          left.Recalc( action, constValue );
          vector<cIntRange> int_ranges;
          CBgArray median_int, rms_iqr_int;
          
          if( is_number(argv[3]) ){
             int y = atol(argv[3]);
             printf("Normalization by integration %d\n",y);
             left.get_line(y,median_int);
          }else{
             printf("Normalization by median integration\n");
             if( strcmp(gInttypeKeyword.c_str(),"ANT")==0 ){
                printf("Using only ANT integrations for median !\n");
                left.GetAntRanges( int_ranges );
                printf("%d ANT ranges detected\n",int(int_ranges.size()));
             }else{
                int_ranges.push_back( cIntRange(0,(left.GetYSize()-1)) );
             }           
             left.GetMedianInt( int_ranges, median_int, rms_iqr_int );
          }                   
          if( action == eNormalizeByMedian ){
             left.Normalize(median_int);          
          }else{
             left.SubtractSpectrum(median_int);
          }
          
          if( gSubtractConstAfter != 0 ){
             left.Recalc( eAddConst, gSubtractConstAfter );
          }
          
          string out_norm_int_file;
          change_ext(fits_left.c_str(),"normint",out_norm_int_file);
          MyOFile out_file(out_norm_int_file.c_str());
          for(int i=0;i<median_int.size();i++){
             out_file.Printf("%.8f %.8f\n",left.ch2freq(i),median_int[i]);
          } 
          printf("INFO : normalization integration saved to file %s\n",out_norm_int_file.c_str());
       }else{
          if( action == eDivideConst || action == eLog10File || action == eSqrtFile || action == eAstroRootImage || action == eDBFile || action==eLin2DB ){  
             if( action == eDivideConst ){
                printf("Dividing by constant = %e !\n",constValue);       
                left.Recalc( action, constValue );
             }else{
                if( action == eLog10File ){
                   printf("Logarithm of file\n");
                }
                if( action == eDBFile ){
                   printf("Lin -> DB scale of file\n");
                }
                if( action == eLin2DB ){
                   printf("DB -> linear scale of file\n");
                }
                if( action == eSqrtFile ){
                   printf("Sqrt of file\n");
                }
                if( action == eAstroRootImage ){
                   printf("AstroRootImage of file\n");
                   if( param1 <= 0.00 ){
//                      param1 = 3000.00;
//                     param1 = 7000000.00;
                        param1 = 4e7;
                   }
                }
                fflush(stdout);
           
                left.Recalc( action, param1 );          
             }
          }
       }
       
       if( action == eSubtractSpectrum ){
          printf("Subtracting spectrum from file %s\n",szFitsRight.c_str());fflush(stdout);
          vector<cValue> spectrum;
          if( read_file( szFitsRight.c_str(), spectrum ) > 0 ){
             CBgArray spectrum_bg;
             for(int i=0;i<spectrum.size();i++){
                spectrum_bg.push_back(spectrum[i].y);
             }
          
             if( left.GetXSize() == spectrum.size() ){
                left.SubtractSpectrum( spectrum_bg );
             }else{
                printf("ERROR : cannot subtract spectrum of different size %d != %d\n",left.GetXSize(),(int)spectrum.size());
             }                 
          }else{
             printf("ERROR : could not read spectrum from file %s\n",szFitsRight.c_str());
          }
       }

       if( action == eDivbySpectrum ){
          printf("Dividing by spectrum from file %s\n",szFitsRight.c_str());
          vector<cValue> spectrum;
          if( read_file( szFitsRight.c_str(), spectrum ) > 0 ){
             left.DivideBySpectrum( spectrum );
          }else{
             printf("ERROR : could not read spectrum from file %s\n",szFitsRight.c_str());
          }
       }


       printf("Writing output fits file %s\n",fits_out.c_str());       
       if( left.WriteFits( fits_out.c_str() ) ){
          printf("ERROR : could not write output file %s\n",fits_out.c_str());
          exit(-1);
       }                                  
       printf("SUCCESS : written output file to %s\n",fits_out.c_str());
     }
  }
}

