#ifndef _MYWGET_H__
#define _MYWGET_H__

#include <stdlib.h>
#include "mytypes.h"

struct CMemoryStruct
{
	CMemoryStruct();
	~CMemoryStruct();
	
	char* memory;
	size_t size;	
};



class CMyWget
{
public:
	void *curl_handle;
	BOOL_T m_bInitDone;
	
	// to store wget results in memory :
	CMemoryStruct chunk;
	
	CMyWget();
	~CMyWget();
	const char* GetData(){ return chunk.memory; }
	
	void Init();
	int SendRequest( const char* url_req );
};



#endif

