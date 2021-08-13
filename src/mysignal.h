#ifndef _SIGNAL_HANDLER_H__
#define _SIGNAL_HANDLER_H__

#include<signal.h>
#include<unistd.h>



class CSigHandler {
	void (*m_pHandlerFunc)(int);
	int m_Signal;
public :	
	CSigHandler( int sig, void(*HandlerFunc)(int) );
	~CSigHandler();
};



#endif

