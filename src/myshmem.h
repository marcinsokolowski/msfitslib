#ifndef _MY_SHMEM_H__
#define _MY_SHMEM_H__

#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "mytypes.h"

class CMyMsgQueue;

#define SHMEM_NAME 256

struct sSharedMemoryInfo
{
	sSharedMemoryInfo();

	int FrameIndex;
	int PipelineSize;
	int FrameCounter;
	int FrameHeaderCount;
	int CurrentlyAnalysingFrame;
	int LastAnalysiedFrame;
	char shMemName[SHMEM_NAME];
};

class CMyShMem 
{
public:
	key_t m_MemKey;
	int   m_ShmID;
	int m_Mode;
	int m_Size;
	int m_TotalSize;
	BOOL_T m_bReadOnly;
	void* ptr;	
	sSharedMemoryInfo* m_pMemInfo;
	void* m_pSharedData;
	CMyMsgQueue* m_pQueue;

	static BOOL_T ValidateKey( key_t memkey );

	CMyShMem( key_t memkey, int size, BOOL_T bReadOnly=FALSE, 
				 int mode=0666, BOOL_T bLocks=TRUE );
	~CMyShMem();
	BOOL_T Init( key_t& key, BOOL_T bAssert=TRUE,
					 BOOL_T bLocks=TRUE, BOOL_T bAutoRetry=FALSE,
					 const char* shName="NONAME", BOOL_T bUseIfExist=FALSE );
	void Initialize();					 
	void* GetMem();
	sSharedMemoryInfo* GetMemInfo();
	
	static BOOL_T Clean( key_t memkey, BOOL_T bAssert=TRUE );
	
	void SelfTest();
	
	// synchronization :
	void ClearQueue();
	BOOL_T Wait();
	void UnLock();
};


#endif
