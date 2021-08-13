#ifndef _MY_BASE_SERVER_H__
#define _MY_BASE_SERVER_H__

class CMyBaseServer
{
public :
	CMyBaseServer();
	
	virtual int Run();
	virtual void HandleRequest( const char* szMessage );
	
	virtual void HandleRequest( const char* szMessage, char* szAnswer );

	virtual void SetHandlerPointer( int (*pHandleRequestFunc)( const char* szMessage ) )
			{ m_pHandleRequestFunc = pHandleRequestFunc; }
	virtual void SetHandlerWithAnswerPointer( int (*pHandleRequestWithAnswerFunc)( const char* szMessage , char* szAnswer ) )
			{ m_pHandleRequestWithAnswerFunc = pHandleRequestWithAnswerFunc; }
protected:
	int m_ServerSocket;
   int m_ClientSocket;				
   
   // handler function pointers :
	int (*m_pHandleRequestFunc)( const char* szMessage );
	int (*m_pHandleRequestWithAnswerFunc)( const char* szMessage , char* szAnswer  );
};



#endif
