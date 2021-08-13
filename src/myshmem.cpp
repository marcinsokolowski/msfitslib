#include "myshmem.h"
#include "cexcp.h"
#include "mymsgqueue.h"
#include <errno.h>
#include "mymacros.h"

sSharedMemoryInfo::sSharedMemoryInfo()
: FrameIndex(0), PipelineSize(0), FrameCounter(0), FrameHeaderCount(0),
  CurrentlyAnalysingFrame(0),LastAnalysiedFrame(0)
{
	shMemName[0] = '\0';
}

void CMyShMem::Initialize()
{
	sSharedMemoryInfo* pInfo = GetMemInfo();
	pInfo->FrameIndex = 0;
	pInfo->PipelineSize = 0;
	pInfo->FrameCounter = 0;
	pInfo->FrameHeaderCount = 0;
	pInfo->CurrentlyAnalysingFrame = 0;
	pInfo->LastAnalysiedFrame = 0;
}

CMyShMem::CMyShMem( key_t memkey, int size, BOOL_T bReadOnly/*=FALSE*/, 
						  int mode, BOOL_T bLocks  )
: m_MemKey(memkey), m_ShmID(0), m_bReadOnly(bReadOnly), ptr(NULL),m_Size(size),
  m_pQueue(NULL),m_pMemInfo(NULL), m_pSharedData(NULL),m_Mode(mode)
{
	m_TotalSize = m_Size + sizeof(sSharedMemoryInfo);
	//m_TotalSize = m_Size;
	// Init( m_MemKey, TRUE, FALSE );
}

BOOL_T CMyShMem::Init( key_t& key, BOOL_T bAssert/*=TRUE*/, 
							  BOOL_T bLocks/*=TRUE*/, BOOL_T bAutoRetry/*=FALSE*/,
							  const char* shName/*="NONAME"*/, BOOL_T bUseIfExist/*=FALSE*/ )
{
	int mode = m_Mode | IPC_CREAT;
	if(!m_bReadOnly)
		mode = mode | IPC_EXCL;
	m_ShmID = shmget( m_MemKey, m_TotalSize, mode);

	BOOL_T bCheckName=FALSE;

	if(m_ShmID == -1){
		printf("error = %s\n",strerror(errno));
		if(bUseIfExist && errno==EEXIST){
			mode = m_Mode | IPC_CREAT;
			m_ShmID = shmget( m_MemKey, m_TotalSize, mode);			
			if( m_ShmID<0 ){
				printf("error : %s\n",strerror(errno));
				Assert(FALSE,"Could not re-use shared memory");
			}
			bCheckName=TRUE;
		}else{
			if(!bAutoRetry){
				if(bAssert)
					Assert(FALSE,"Could not allocate shared memory buffer of size %d",m_Size);
				return FALSE;
			}else{
				while(m_ShmID==-1 && errno==EEXIST){
					m_MemKey++;
					m_ShmID = shmget( m_MemKey, m_TotalSize, m_Mode | IPC_CREAT | IPC_EXCL);
				}
			}
		}
	}
	ptr = shmat( m_ShmID, (void*)0, 0 );
	if(ptr == (void*)-1){
		if(bAssert)
			Assert(FALSE,"Unsuccessfull call to shmat");
		return FALSE;
	}
	// m_pSharedData = ptr;
	m_pSharedData = (void*)((char*)ptr + sizeof(sSharedMemoryInfo));
	m_pMemInfo = (sSharedMemoryInfo*)ptr;

	if(bCheckName){
		if(strcmp(m_pMemInfo->shMemName,shName)){
			_TRACE_PRINTF_2("Module %s error : Segment with key %d already used by another module %s ",m_MemKey,shName,m_pMemInfo->shMemName);
			return FALSE;
		}
	}else{
		if(!m_bReadOnly)
			strcpy(m_pMemInfo->shMemName,shName);
	}
	
	if(bLocks){
		key_t queuekey = 2*m_MemKey;
		m_pQueue = new CMyMsgQueue( queuekey );
	}

	Initialize();	
	//for(register int i=0;i<m_TotalSize;i++){
		// printf("i=%d\n",i);
	//	((ELEM_TYPE*)ptr)[i] = 0;
	//}
	return TRUE;
}

CMyShMem::~CMyShMem()
{
	if(ptr){
		if(shmdt(ptr) == -1){
			Assert(FALSE,"Unsuccessfull call to shmdt : %s",strerror(errno));
		}	
	}
	if(m_ShmID!=-1 && !m_bReadOnly){
		if(shmctl(m_ShmID,IPC_RMID,0) == -1){
			printf("Unsuccessfull call to shmctl(IPC_RMID) : %s\n",strerror(errno));
			printf("m_ShmID = %d\n",m_ShmID);
			Assert(FALSE,"Unsuccessfull call to shmctl(IPC_RMID) : %s",strerror(errno));
		}
	}
	if(m_pQueue){
		delete m_pQueue;
	}
}

BOOL_T CMyShMem::Clean( key_t memkey, BOOL_T bAssert )
{
	int id = shmget( memkey, 0, 0666 | IPC_CREAT );
	if(id == -1){
		printf("Non existing shared memory segment for key=%d\n",memkey);
		if(bAssert){
			Assert(FALSE,"Non existing shared memory segment for key=%d",memkey);
		}
		return FALSE;
	}
	void* ptr = shmat( id, (void*)0, 0 );
	if(ptr == (void*)-1){
		printf("Clean :: Unsuccessfull call to shmat\n");
		if( bAssert ){
			Assert(FALSE,"Clean :: Unsuccessfull call to shmat");
		}
		return FALSE;
	}
	if(shmdt(ptr) == -1){
		printf("Unsuccessfull call to shmdt\n");
		if( bAssert ){
			Assert(FALSE,"Unsuccessfull call to shmdt");
		}
		return FALSE;
	}	
	if(shmctl(id,IPC_RMID,0) == -1){
		printf("Unsuccessfull call to shmctl(IPC_RMID)\n");
		if( bAssert ){
			Assert(FALSE,"Unsuccessfull call to shmctl(IPC_RMID)");
		}
		return FALSE;
	}
	return TRUE;
}

void* CMyShMem::GetMem()
{
	return m_pSharedData;
}

sSharedMemoryInfo* CMyShMem::GetMemInfo()
{
	return m_pMemInfo;
}

void CMyShMem::UnLock()
{
	if(m_pQueue){
		m_pQueue->SendMsg("x",1);
	}
}

BOOL_T CMyShMem::Wait()
{
	if(m_pQueue){
		mystring szMsg;
		return m_pQueue->GetMsg( szMsg );
	}
	return FALSE;
}

void CMyShMem::ClearQueue()
{
	if(m_pQueue){
		mystring szMsg;
		while( m_pQueue->GetMsgNoWait( szMsg)){}	
	}
}


void CMyShMem::SelfTest()
{
	for(register int i=0;i<m_TotalSize;i++){
		((char*)ptr)[i] = 0;	
	}	
}


BOOL_T CMyShMem::ValidateKey( key_t memkey )
{
	return TRUE;
}