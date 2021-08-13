#include "mytxtinetserver.h"
#include <errno.h>
#include "cexcp.h"

CMyTxtInetServer::CMyTxtInetServer( const char* inet_adres, int port_no )
: m_szInetAdres(inet_adres),	m_PortNo(port_no)
{

}

int CMyTxtInetServer::Run()
{
	char buffer[1024];
	char buf2[2048];
	char answer_buffer[CCD_MAX_ANSWER_BUFF_SIZE];

	m_ServerSocket = socket(AF_INET,SOCK_STREAM,0); 
	m_ServerAddres.sin_family = AF_INET;
	m_ServerAddres.sin_addr.s_addr = inet_addr( m_szInetAdres.c_str() );
	m_ServerAddres.sin_port = m_PortNo;
	
	int server_size = sizeof(m_ServerAddres);
	socklen_t client_size=sizeof(m_ClientAddres);
	bind( m_ServerSocket, (struct sockaddr*)&m_ServerAddres, server_size );	

	listen( m_ServerSocket, 5 );
	while( 1 ){		
		// m_ClientSocket = accept( m_ServerSocket, (struct sockaddr*)&m_ClientAddres, &client_size );
		m_ClientSocket = accept( m_ServerSocket, (struct sockaddr*)&m_ClientAddres, &client_size );
		if(m_ClientSocket<=0){
			printf("ERROR : accept returned ret = %d\n",m_ClientSocket);
			printf("Could not obtain client's socket due to error errno=%d, %s\n",errno,strerror(errno));
			continue;
		}
		
		int bytes_read;
		mystring szMessage;
		while( (bytes_read = read( m_ClientSocket, buffer, 1024 ))>0 ){
			if(buffer[bytes_read-1]=='\0'){
				szMessage << buffer;			
				break;
			}else{
				memcpy( buf2, buffer, bytes_read );
				buf2[bytes_read]='\0';
				szMessage << buf2;
			}
		} 
		HandleRequest( szMessage.c_str(), answer_buffer );
		Assert(strlen(answer_buffer)<CCD_MAX_ANSWER_BUFF_SIZE,"Answer exceeds buffer size");
		write( m_ClientSocket, answer_buffer, CCD_MAX_ANSWER_BUFF_SIZE );

		close( m_ClientSocket );
	}
}

