#ifndef _MYLIBLOADER_H__
#define _MYLIBLOADER_H__

#include "mystring.h"
#include <vector>

using namespace std;

class CLibInfo
{
public:
	CLibInfo() : pLibHandle(NULL),curr_ret_int_symbol(NULL) {};

	// members :
	void* pLibHandle;			
	mystring m_szLibName;

	mystring m_szCurrSymbol;
	int (*curr_ret_int_symbol)(void*);
};

class CMyLibLoader
{
protected:
	CLibInfo* m_pCurrLib;
	mystring m_szCurrSymbol;
	int (*curr_ret_int_symbol)(void*);		

	static vector<CLibInfo> m_LibTab;	
public :
	CMyLibLoader();
	CLibInfo* LoadLibrary(const char* _szLibName);

	CLibInfo* FindLibrary(const char* szLibName);

	BOOL_T SetCurLibrary(const char* _szLibName);
		
	BOOL_T SetCurSymbol(const char* _szLibName, const char* szSymbol);

	BOOL_T CallCurSymbol( void* params, int& ret );
};


#endif
