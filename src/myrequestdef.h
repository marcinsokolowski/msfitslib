#ifndef _MY_REQUEST_DEF_H__
#define _MY_REQUEST_DEF_H__

#include "mystring.h"
#include "mystrtable.h"
#include "basestructs.h"

// this maybe changed in future :
#include <ccdanal_interf.h>

#include <queue>
#include <vector>

using namespace std;

class CMyRequestDef
{
public :
	CMyRequestDef();
	CMyRequestDef( const char* szReqName, CMyStrTable& req_def_list );

	CMyRequestDef& operator=( const CMyRequestDef& right );

	mystring m_szRequestName;
	eCCDRequestType m_ReqType;

	eCCDRequestTo m_ReqTo;
	int m_ReqPipeline;
	int m_ReqCamera;
	
	
	CMyStrTable m_ReqDefTab;
	vector<CEnvVar> m_ReqParamsTab;	
};


class CMyRequestQueue : deque<CMyRequestDef>
{
public :
	CMyRequestQueue(){};


	int getcount() { return size(); };	
	CMyRequestDef& GetFront() { return front(); }
	void PopFront() { pop_front(); }		
	
	virtual void AddNewRequest( const CMyRequestDef& new_req );
	virtual CMyRequestDef GetNextRequestToHandle();
	virtual void RemoveFront();
};


#endif
