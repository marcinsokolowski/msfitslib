#include <dlfcn.h>
#include "mylibloader.h"

vector<CLibInfo> CMyLibLoader::m_LibTab;

CMyLibLoader::CMyLibLoader()
:m_pCurrLib(NULL),curr_ret_int_symbol(NULL)
{
}


CLibInfo* CMyLibLoader::LoadLibrary(const char* _szLibName)
{
	if(strlen(_szLibName)){
		mystring szLibName = _szLibName;
		szLibName.env2str();
		CLibInfo* pLibInfo = FindLibrary(szLibName.c_str());
		if(!pLibInfo){
			CLibInfo newLib;
			newLib.m_szLibName = szLibName;
			newLib.pLibHandle = dlopen(szLibName.c_str(),RTLD_LAZY);
			// newLib.pLibHandle = dlopen(szLibName.c_str(),RTLD_NOW );
			// newLib.pLibHandle = dlopen(szLibName.c_str(),RTLD_GLOBAL );
			if(newLib.pLibHandle){
				m_LibTab.push_back(newLib);	
				pLibInfo = (&(m_LibTab[m_LibTab.size()-1]));
				m_pCurrLib = pLibInfo;
				printf("library %s loaded ok\n",szLibName.c_str());
			}else{
				printf("Error : %s\n",dlerror());
			}
		}
		return pLibInfo;
	}
	return NULL;
}

CLibInfo* CMyLibLoader::FindLibrary(const char* szLibName)
{
	vector<CLibInfo>::iterator i;
	for(i=m_LibTab.begin();i!=m_LibTab.end();i++){
		if(strcmp(i->m_szLibName.c_str(),szLibName)==0){
			return &(*i);
		}
	}
	return NULL;
}

BOOL_T CMyLibLoader::SetCurSymbol(const char* _szLibName, const char* szSymbol)
{
	char* error;
	mystring szLibName = _szLibName;
	szLibName.env2str();
	if(!SetCurLibrary(szLibName.c_str()))
		return FALSE;
	curr_ret_int_symbol = (int (*)(void*))dlsym( m_pCurrLib->pLibHandle, szSymbol );

	if ((error = dlerror()) != NULL)  {
		printf("%s\n",error);
		return FALSE;
   }


	if(curr_ret_int_symbol){
		m_szCurrSymbol = szSymbol;
		return TRUE;
	}
	return FALSE;	
}

BOOL_T CMyLibLoader::SetCurLibrary(const char* _szLibName)
{
	mystring szLibName = _szLibName;
	szLibName.env2str();
	if(m_pCurrLib){
		if(strcmp(m_pCurrLib->m_szLibName.c_str(),szLibName.c_str())==0)
			return TRUE;
	}
	CLibInfo* pLibInfo = FindLibrary( szLibName.c_str());
	if(!pLibInfo)
	pLibInfo = LoadLibrary( szLibName.c_str() );
	if(!pLibInfo) 
		return FALSE;
	m_pCurrLib = pLibInfo;
	return TRUE;
}

BOOL_T CMyLibLoader::CallCurSymbol( void* params, int& ret )
{
	ret = 0;
	if(curr_ret_int_symbol){
		ret = (*curr_ret_int_symbol)(params);
		return TRUE;				
	}
	return FALSE;		
}
