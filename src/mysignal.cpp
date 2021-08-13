#include "mysignal.h"


CSigHandler::CSigHandler( int sig, void(*HandlerFunc)(int) )
{
	m_pHandlerFunc = HandlerFunc;
	m_Signal = sig;
	signal( m_Signal, m_pHandlerFunc );
}

CSigHandler::~CSigHandler(){
	signal(m_Signal, SIG_DFL);
}

