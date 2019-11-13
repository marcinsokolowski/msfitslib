#ifndef _MY_TXT_CLIENT_H__
#define _MY_TXT_CLIENT_H__

#include "mystring.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// inet client :
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ccdanal_interf.h>

class CMyTxtClient
{
public:
	CMyTxtClient( const char* socket_name );
	~CMyTxtClient();

	virtual BOOL_T Connect();
	void Disconnect();
	int SendRequest( const char* szRequest );
	
protected:
	mystring m_szSocketName;
	int m_Socket;
	struct sockaddr_un m_Adres;	
};

class CMyTxtInetClient
{
public:
	CMyTxtInetClient( const char* inet_adres, int port_no );
	~CMyTxtInetClient();

	virtual BOOL_T Connect();
	void Disconnect();
	int SendRequest( const char* szRequest );
	int SendRequest( const char* szRequest, char* szAnswer );
	int ReadAnswer( char* szAnswer, int size=CCD_MAX_ANSWER_BUFF_SIZE );

protected:
	mystring m_szInetAdres;
	int m_PortNo;
	
	int m_Socket;
	struct sockaddr_in m_Adres;
};


#endif
