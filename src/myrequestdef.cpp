#include "myrequestdef.h"

CMyRequestDef::CMyRequestDef( const char* szReqName, CMyStrTable& req_def_list )
: m_szRequestName(szReqName), m_ReqDefTab(req_def_list), m_ReqTo(eReqGlobal)
  , m_ReqPipeline(-1), m_ReqCamera(-1), m_ReqType(eCCDUndefined)
{
	
}
CMyRequestDef::CMyRequestDef()
: m_ReqTo(eReqGlobal),m_ReqPipeline(-1), m_ReqCamera(-1), m_ReqType(eCCDUndefined)
{
	
}

CMyRequestDef& CMyRequestDef::operator=( const CMyRequestDef& right )
{
	m_szRequestName = right.m_szRequestName;
	m_ReqDefTab = right.m_ReqDefTab;

	return (*this);
}

void CMyRequestQueue::AddNewRequest( const CMyRequestDef& new_req )
{
	push_back( new_req );
}

CMyRequestDef CMyRequestQueue::GetNextRequestToHandle()
{
	return front();
}


void CMyRequestQueue::RemoveFront()
{
	pop_front();
}
