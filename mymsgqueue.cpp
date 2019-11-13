#include "mymsgqueue.h"
#include "cexcp.h"


CMyMsgQueue::CMyMsgQueue( key_t queuekey, int mode )
: m_QueueKey( queuekey ), m_QueueID(0)
{
	m_QueueID = msgget( m_QueueKey, mode | IPC_CREAT );
	if(m_QueueID == -1){
		Assert(FALSE,"Could not create queue : msgget failed");
	}	
}


CMyMsgQueue::~CMyMsgQueue()
{
	if(!msgctl(m_QueueID,IPC_RMID,0)==-1){
		Assert(FALSE,"Could not remove queue : msgctl failed");
	}
}


BOOL_T CMyMsgQueue::SendMsg( char* msg, int size )
{		
	if(size>=BUFF_SIZE)
		return FALSE;
	cMessageBuffer message;
	message.message_type = 1;
	strcpy(message.buffer,msg);
	if(msgsnd(m_QueueID,(void*)&message,BUFF_SIZE,0)==-1){
		return FALSE;
	}
	return TRUE;	
}

BOOL_T CMyMsgQueue::GetMsgNoWait( mystring& szMessage )
{
	cMessageBuffer message;
   long int message_to_get = 0;
	int nbytes = msgrcv(m_QueueID,(void*)&message,BUFF_SIZE,message_to_get, IPC_NOWAIT );
	if(nbytes<=0){
		return FALSE;
	}
	szMessage = message.buffer;
	return TRUE;
}

BOOL_T CMyMsgQueue::GetMsg( mystring& szMessage )
{	
	cMessageBuffer message;
	long int message_to_get = 0;
	int nbytes = msgrcv(m_QueueID,(void*)&message,BUFF_SIZE,message_to_get, 0 );
	if(nbytes<=0){
		return FALSE;
	}	
	szMessage = message.buffer;
	return TRUE;
}
