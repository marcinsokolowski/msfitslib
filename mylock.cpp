#include "mylock.h"
#include <stdio.h>
#include "mymacros.h"

CMyMutex::CMyMutex()
: is_locked(0)
{
	id = pthread_mutex_init( &mutex, NULL );
}

CMyMutex::~CMyMutex()
{
	UnLock();
	pthread_mutex_destroy( &mutex );
}

void CMyMutex::Lock()
{
	pthread_mutex_lock( &mutex );
	is_locked = 1;
	_TRACE_PRINTF_6("Mutex %x : LOCKED\n",this);
}


void CMyMutex::UnLock()
{
	pthread_mutex_unlock( &mutex );
	is_locked = 0;
	_TRACE_PRINTF_6("Mutex %x : UN-LOCKED\n",this);
}
