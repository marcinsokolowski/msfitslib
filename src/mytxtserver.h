#ifndef _MY_TXT_SERVER_H__
#define _MY_TXT_SERVER_H__

#include "mystring.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "mybaseserver.h"

class CMyTxtServer : public CMyBaseServer
{
public:
	CMyTxtServer( const char* socket_name );
			
	int Run();	

	
protected:
	mystring m_szSocketName;
	
	
	struct sockaddr_un m_ServerAddres;
	struct sockaddr_un m_ClientAddres;	
};



#endif
