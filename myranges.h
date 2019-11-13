#ifndef _MY_RANGES_FILE_H__
#define _MY_RANGES_FILE_H__


#include <stdio.h>
#include <stdarg.h>
#include "mystring.h"
#include "mytypes.h"
#include "basedefines.h"
#include "basestructs.h"
#include <vector>

using namespace std;

#define MAX_OUT_BUFFER_SIZE 20000
#define DUMP_WHEN           10000


class CMyStrTable;
class CSafeKeyTab;


class CRangeDef
{
public :
	CRangeDef(const char* szVarName,const char* szLow,const char* szUp)
	: m_szVarName(szVarName),m_szLow(szLow),m_szUp(szUp)
	{};
	
	mystring m_szVarName;
	mystring m_szLow;
	mystring m_szUp;
};

class BASELIB_EI CRangesFile
{	
protected:
	vector<CRangeDef> m_RangesDefTab;	
	vector<CEnvVar>   m_CfgTab;		
public :
	CRangesFile(const char* filename="ranges.def");	
	void Init(const char* filename);
	~CRangesFile();
	LONG_T GetCount();
	
	vector<CRangeDef>& GetDescTab(){ return (m_RangesDefTab); }
	
	BOOL_T IsOpened(){ return (m_RangesDefTab.size()>0); }
	
	BOOL_T GetRange( const char* varname,double& low, double& up );


	// parameters handling :
	inline int GetParamCount(){ return m_CfgTab.size(); }
	inline vector<CEnvVar>& GetParamTab(){ return m_CfgTab; }
};



#endif
