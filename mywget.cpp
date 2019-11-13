#include "mywget.h"

#include <stdio.h>
#include <string.h>

size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register int realsize = size * nmemb;
	struct CMemoryStruct *mem = (struct CMemoryStruct *)data;
 
 	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}


#ifdef _NO_CURL_

void CMyWget::Init()
{
	m_bInitDone = TRUE;
}

int CMyWget::SendRequest( const char* url_req )
{
	return 0;
}

#else

#include <curl/curl.h>
// #include <curl/types.h>
#include <curl/easy.h>
#include <string.h>

void CMyWget::Init()
{
	if(!m_bInitDone){
		curl_global_init(CURL_GLOBAL_ALL);
		m_bInitDone = TRUE;
	}
}


int CMyWget::SendRequest( const char* url_req )
{
	Init();
	CURLcode res;

	curl_handle = curl_easy_init(); 
	curl_easy_setopt(curl_handle, CURLOPT_URL, url_req);
//	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER );
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);	
	res = curl_easy_perform(curl_handle);
/*	if(res==CURLE_OK){
		CURLINFO info;
		double ret;

		CURLcode res2 = curl_easy_getinfo( (CURL*)curl_handle, info, &ret );
		printf("res2 = %d\n",(int)res2);
	}*/	

	curl_easy_cleanup(curl_handle);

	return (res == CURLE_OK);
}

#endif


CMemoryStruct::CMemoryStruct()
: memory(NULL), size(0)
{
	
}

CMemoryStruct::~CMemoryStruct()
{
	if(memory){
		delete [] memory;
	}
}


CMyWget::CMyWget()
: curl_handle(NULL), m_bInitDone(FALSE) 
{
}

CMyWget::~CMyWget()
{
}


