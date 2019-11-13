#ifndef _MY_TXT_INET_SERVER_H__
#define _MY_TXT_INET_SERVER_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include "mybaseserver.h"
#include "mystring.h"

#include <ccdanal_interf.h>

class CMyTxtInetServer : public CMyBaseServer
{
public:
	CMyTxtInetServer( const char* inet_adres, int port_no );
			
	virtual int Run();	

protected:
	mystring m_szInetAdres;
	int m_PortNo;
	
	
	struct sockaddr_in m_ServerAddres;
	struct sockaddr_in m_ClientAddres;
};


#endif
