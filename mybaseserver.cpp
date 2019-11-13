#include "mybaseserver.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// int (*m_pHandleRequestFunc)( const char* szMessage );
// int (*m_pHandleRequestWithAnswerFunc)( const char* szMessage , char* szAnswer  );

CMyBaseServer::CMyBaseServer()
: m_ServerSocket(0), m_ClientSocket(0)
{
	m_pHandleRequestFunc = NULL;
	m_pHandleRequestWithAnswerFunc = NULL;
}

int CMyBaseServer::Run()
{
	return 1;
}

void CMyBaseServer::HandleRequest( const char* szMessage )
{
	printf("REQUEST : \n%s\n",szMessage);
	if( m_pHandleRequestFunc ){
		printf("INFO : handling request %s with external handler \n",szMessage);
		int ret = (*m_pHandleRequestFunc)( szMessage );
		printf("INFO : external handler returned %d\n",ret);
	}
}


void CMyBaseServer::HandleRequest( const char* szMessage, char* szAnswer )
{
	printf("REQUEST WITH ANSWER: \n%s\n",szMessage);
	strcpy(szAnswer,"OK !!!! - ODO");

	if( m_pHandleRequestWithAnswerFunc ){
		printf("INFO : handling request %s with external handler with answer\n",szMessage);
		int ret = (*m_pHandleRequestWithAnswerFunc)( szMessage , szAnswer );
		printf("INFO : external handler returned %d / %s\n",ret,szAnswer);
	}else{
		if( m_pHandleRequestFunc ){
			printf("INFO : handling request %s with external handler ( no function with answer defined )\n",szMessage);
			int ret = (*m_pHandleRequestFunc)( szMessage );
			printf("INFO : external handler returned %d\n",ret);
		}	
	}
}
