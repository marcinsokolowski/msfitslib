#include "mytxtclient.h"
#include <netinet/ip.h>
#include <netdb.h>


CMyTxtClient::CMyTxtClient( const char* socket_name )
: m_szSocketName(socket_name),m_Socket(-1)
{
	m_szSocketName.env2str();
}

BOOL_T CMyTxtClient::Connect()
{
	m_Socket = socket( AF_UNIX, SOCK_STREAM, 0 );
	m_Adres.sun_family = AF_UNIX;
	strcpy(m_Adres.sun_path,m_szSocketName.c_str());	
	int size = sizeof(m_Adres);
	int res = connect( m_Socket, (struct sockaddr*)&m_Adres,size);
	return (res!=-1);
}

CMyTxtClient::~CMyTxtClient()
{
	Disconnect();
}

void CMyTxtClient::Disconnect()
{
	if( m_Socket > 0 ){
		close( m_Socket );
		m_Socket = -1;
	}
}

int CMyTxtClient::SendRequest( const char* szRequest )
{
	int ret = write(m_Socket,szRequest,strlen(szRequest)+1);
	return ret;
}


CMyTxtInetClient::CMyTxtInetClient( const char* inet_adres, int port_no )
: m_szInetAdres( inet_adres ), m_PortNo(port_no), m_Socket(-1)
{
}

BOOL_T CMyTxtInetClient::Connect()
{
	m_Socket = socket( AF_INET, SOCK_STREAM, 0 );
	m_Adres.sin_family = AF_INET;
	m_Adres.sin_addr.s_addr = inet_addr(m_szInetAdres.c_str() );	
	m_Adres.sin_port = m_PortNo; // sometimes not work use = htons( m_PortNo );
	int size = sizeof(m_Adres);
	int res = connect( m_Socket, (struct sockaddr*)&m_Adres,size);
	return (res!=-1);
}

void CMyTxtInetClient::Disconnect()
{
	if( m_Socket > 0 ){
		close( m_Socket );
		m_Socket = -1;
	}
}

CMyTxtInetClient::~CMyTxtInetClient()
{
	Disconnect();	
}

int CMyTxtInetClient::SendRequest( const char* szRequest )
{
	int ret = write(m_Socket,szRequest,strlen(szRequest)+1);
	return ret;
}


int CMyTxtInetClient::SendRequest( const char* szRequest, char* szAnswer )
{
	int ret1 = write(m_Socket,szRequest,strlen(szRequest)+1);
	int ret2 = read( m_Socket,szAnswer,CCD_MAX_ANSWER_BUFF_SIZE);

	int ret = MIN( ret1 , ret2 );
	return ret;
}

int CMyTxtInetClient::ReadAnswer( char* szAnswer , int size /*=CCD_MAX_ANSWER_BUFF_SIZE */ )
{
	int ret = read( m_Socket,szAnswer,size);
	return ret;
}


/*
struct sockaddr_in CMyTxtInetClient::getipa(const char* hostname, int port){
	struct sockaddr_in ipa;
	ipa.sin_family = AF_INET;
	ipa.sin_port = htons(port);

	struct hostent* localhost = gethostbyname(hostname);
	if(!localhost){
//		printerror("resolveing localhost");

		return ipa;
	}

	char* addr = localhost->h_addr_list[0];
	memcpy(&ipa.sin_addr.s_addr, addr, sizeof(addr));

	return ipa;
}
*/
