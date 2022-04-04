#ifndef __ARRAY_CONFIG_COMMON_H__
#define __ARRAY_CONFIG_COMMON_H__

#include <string>

using namespace std;

class InputMapping
{
public :
   int input;
   int antenna;
   string szAntName;
   char pol;
   int delta;
   int flag;  
   
   double x;
   double y;
   double z;

   // constructor and definition in .cpp file
   InputMapping();

//   static int read_mapping_file( std::vector<InputMapping>& inputs , const char* filename="instr_config.txt" );
};


#endif
