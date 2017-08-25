#ifndef __CTCPCLIENTSOCKETMANAGE__H
#define __CTCPCLIENTSOCKETMANAGE__H
#include <iostream>
#include <list>
#include "MemoryPool.h"
#include <WinSock2.h>
#include <Windows.h>

// 客户端socket管理

struct ClientInfo
{
	SOCKET sClientSocket;
};

class CTCPClientSocketManage
{
public:
	CTCPClientSocketManage();
	~CTCPClientSocketManage();

	ClientInfo* GetClient()
	{
		ClientInfo* pClient = m_clientSocket.GetElement();
		if (pClient)
			m_lstClient.push_back(pClient);
		return pClient;
	}

	void DeleteClient(ClientInfo* pClient)
	{
		if (!pClient)
			return;
		closesocket(pClient->sClientSocket);
		auto iter = std::find_if(m_lstClient.begin(), m_lstClient.end(), [pClient](ClientInfo* pInfo) { return pClient == pInfo; });
		m_lstClient.erase(iter);
		m_clientSocket.ReleaseElement(pClient);
	}

	void DeleteAllClient()
	{
		for (auto& pClient : m_lstClient)
		{
			closesocket(pClient->sClientSocket);
			m_clientSocket.ReleaseElement(pClient);
		}

		m_lstClient.clear();
	}

private:
	CMemoryPool<ClientInfo> m_clientSocket;
	std::list<ClientInfo*> m_lstClient;
};

#endif