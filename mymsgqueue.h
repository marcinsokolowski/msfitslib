#ifndef _MY_MSG_QUEUE_H__
#define _MY_MSG_QUEUE_H__

#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "mytypes.h"

#define BUFF_SIZE 2048

class mystring;

struct cMessageBuffer
{
	long int message_type;
	char buffer[BUFF_SIZE];
};

class CMyMsgQueue
{
public :
	key_t m_QueueKey;
	int m_QueueID;

	CMyMsgQueue( key_t queuekey, int mode=0666 );
	~CMyMsgQueue();
	
	BOOL_T SendMsg( char* msg, int size );
	BOOL_T GetMsg( mystring& szMessage );
	BOOL_T GetMsgNoWait( mystring& szMessage );
};



#endif
