#include "mytxtserver.h"


CMyTxtServer::CMyTxtServer( const char* socket_name )
: m_szSocketName( socket_name )
{
	m_szSocketName.env2str();
}


int CMyTxtServer::Run()
{
	char buffer[1024];
	char buf2[2048];
	unlink( m_szSocketName.c_str() );
	m_ServerSocket = socket(AF_UNIX,SOCK_STREAM,0); 
	m_ServerAddres.sun_family = AF_UNIX;
	strcpy(m_ServerAddres.sun_path,m_szSocketName.c_str());
	
	int server_size = sizeof(m_ServerAddres);
	socklen_t client_size;
	bind( m_ServerSocket, (struct sockaddr*)&m_ServerAddres, server_size );	

	listen( m_ServerSocket, 5 );
	while( 1 ){		
		m_ClientSocket = accept( m_ServerSocket, (struct sockaddr*)&m_ClientAddres, &client_size );
		
		int bytes_read;
		mystring szMessage;
		while( bytes_read = read( m_ClientSocket, buffer, 1024 )){
			if(buffer[bytes_read-1]=='\0')
				szMessage << buffer;			
			else{
				memcpy( buf2, buffer, bytes_read );
				buf2[bytes_read]='\0';
				szMessage << buf2;
			}
		} 
		HandleRequest( szMessage.c_str() );
		close( m_ClientSocket );
	}
}


