#ifndef _ServerSocket_H_
#define _ServerSocket_H_

#include "IOCP.h"
#include "..\log\log.h"
#include "..\MsgDef\MsgDef.h"

#include <list>
using namespace std;

template <class T>
class CQueue : public list<T>
{
public:
	CQueue(void) 
	{
		::InitializeCriticalSection(&m_ListLock);
	}

	virtual ~CQueue(void) 
	{
		::DeleteCriticalSection(&m_ListLock);
	}

	unsigned long GetSize()
	{
		unsigned long m_size = 0;
		
		::EnterCriticalSection(&m_ListLock);
		m_size = size(); 
		::LeaveCriticalSection(&m_ListLock);
		
		return m_size;
	}

	void Push(T lpData)
	{
		::EnterCriticalSection(&m_ListLock);

		push_back(lpData);
		
		::LeaveCriticalSection(&m_ListLock);
	}

	T Pop()
	{
		T lpData = NULL;
		::EnterCriticalSection(&m_ListLock);

		if(size())
		{
			lpData = front();
			pop_front(); 
		}

		::LeaveCriticalSection(&m_ListLock);

		return lpData;
	}

	bool Empty()
	{
		bool bRet = false;
	
		::EnterCriticalSection(&m_ListLock);

		bRet = empty();

		::LeaveCriticalSection(&m_ListLock);

		return bRet;
	}

private::
	CRITICAL_SECTION m_ListLock;
};

typedef unsigned (WINAPI *PTHREADFUN)(LPVOID lpParameter); 
class CUserServer : public CIOCPServer
{
public:
	CUserServer();
	~CUserServer();

public:
	void IniServer();     // 初始化，先分配好内存

    bool SendPacket(PACKET* lpPacket);  //用户层将游戏数据包发送到发送队列

	bool StartupAllMsgThread();   //启动接收和发送队列

	PACKET* AllocatePacket();
	void ReleasePacket(PACKET *pPacket);
//	void FreePacket();

private:
	CQueue<PACKET*> m_listRecvMsg;
	CQueue<PACKET*> m_listSendMsg;
	//CQueue<PACKET*> m_listDelayMsg;
	//消息处理线程
private:
	void AddPacketToRecvlist(PACKET* lpPacket)
	{
		m_listRecvMsg.Push(lpPacket);
	}
	PACKET* PopPacketFromRecvList()
	{
		return m_listRecvMsg.Pop();
	}
	void AddPacketToSendlist(PACKET* lpPacket)
	{
		m_listSendMsg.Push(lpPacket);
	}
	PACKET* PopPacketFromSendList()
	{
		return m_listSendMsg.Pop();
	}
	//void AddPacketToDelaylist(PACKET* lpPacket)
	//{
	//	m_listDelayMsg.Push(lpPacket);
	//}
	//PACKET* PopPacketFromDelayList()
	//{
	//	return m_listDelayMsg.Pop();
	//}

	HANDLE m_hRecvWait,m_hSendWait/*,m_hDelayWait*/;
	bool m_bRecvRun,m_bSendRun/*,m_bDelayRun*/;
	HANDLE m_hRecvThread,m_hSendThread/*,m_hDelayThread*/;


	void CloseAllMsgThread();

	virtual void HandleRecvMessage(PACKET* lpPacket) = 0;
	//将游戏包以WSASend方式发送到IOCP上
	void SendPacketToIOCP(PACKET* lpPacket);

	static DWORD WINAPI RecvThread(LPVOID lpParameter);
	static DWORD WINAPI SendThread(LPVOID lpParameter);
	//static DWORD WINAPI DelayThread(LPVOID lpParameter);


protected:
/*
	PACKET* AllocatePacket();
	void ReleasePacket(PACKET *pPacket);
*/	void FreePacket();

private:
	PACKET *m_pFreePacketList;
	int m_nFreePacketCount;	
	int m_nMaxPacketBuffers;
	CRITICAL_SECTION m_FreePacketListLock;
	//网络事件通知
private:
	void OnConnectionEstablished(CIOCPContext *pContext, CIOCPBuffer *pBuffer);
	void OnConnectionClosing(CIOCPContext *pContext, CIOCPBuffer *pBuffer);
	void OnConnectionError(CIOCPContext *pContext, CIOCPBuffer *pBuffer, int nError);
	void OnReadCompleted(CIOCPContext *pContext, CIOCPBuffer *pBuffer);
	void OnWriteCompleted(CIOCPContext *pContext, CIOCPBuffer *pBuffer);
	bool SplitPacket(CIOCPContext *pContext, CIOCPBuffer *pBuffer);
	void OnPacketError(){ WriteToLog("拆包错误!\r\n");}
};
#endif
