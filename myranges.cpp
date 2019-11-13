#include "myranges.h"
#include "myfile.h"
#include "myparser.h"
#include "mystrtable.h"

CRangesFile::CRangesFile(const char* filename)
{
	Init(filename);
}

void CRangesFile::Init(const char* filename)
{
	if(MyFile::DoesFileExist(filename)){
		MyIFile infile(filename);
	   const char* pLine;
		m_RangesDefTab.clear();
	   while(pLine = infile.GetLine(TRUE)){
			if (strlen(pLine)==0 || pLine[0]=='\n')
		      continue;
			// skip comments :
			if (pLine[0]=='#' || strncmp(pLine,"//",2)==0)
				continue;

			MyParser parser = pLine;
			CMyStrTable items;

			parser.GetItems( items, ", \t");
			
			if( items.size()>=3){ 
				CRangeDef tmp( items[0].c_str(), items[1].c_str(), items[2].c_str() );
				m_RangesDefTab.push_back( tmp );
			}else{
				// in this case this is parameter definition :	
				parser = pLine;
   	      CEnvVar tmp;
      	   if(!parser.GetVarAndValue(tmp.szName,tmp.szValue))
         	   continue;
	         tmp.szValue.env2str();

    	     if(strlen(tmp.szName.c_str())==0 && strlen(tmp.szValue.c_str())==0)
      	      continue;

         	m_CfgTab.push_back(tmp);
			}
	   }
	   infile.Close();			
	}
}

CRangesFile::~CRangesFile()
{}


LONG_T CRangesFile::GetCount()
{
	return m_RangesDefTab.size();
}


BOOL_T CRangesFile::GetRange( const char* varname,double& low, double& up )
{
	vector<CRangeDef>::iterator i;
	for(i = m_RangesDefTab.begin();i!=m_RangesDefTab.end();i++){
		if(strcasecmp( i->m_szVarName.c_str(), varname )==0){
			low = atof(i->m_szLow.c_str());
			up = atof(i->m_szUp.c_str());
			return TRUE;
		}
	}	
	return FALSE;
}
