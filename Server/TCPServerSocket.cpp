#define _CRT_SECURE_NO_WARNINGS
#include "TCPServerSocket.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

CTCPServerSocket::CTCPServerSocket(std::string& strIP, int nPort) : m_strIP(strIP), m_nPort(nPort), m_sServerSocket(INVALID_SOCKET)
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	m_nSystemInfo = systemInfo.dwNumberOfProcessors;
}


CTCPServerSocket::~CTCPServerSocket()
{
	m_clientMange.DeleteAllClient();
}

void CTCPServerSocket::ErrorMsg(const char* err, ...)
{
	char szErrorMsg[2048] = { 0x00 };
	va_list arg;
	va_start(arg, err);
	vsprintf_s(szErrorMsg, sizeof(szErrorMsg), err, arg);
	m_strErrorMsg = szErrorMsg;
}

void CTCPServerSocket::CleanSocket()
{
	if (m_sServerSocket != INVALID_SOCKET)
	{
		closesocket(m_sServerSocket);
		m_sServerSocket = INVALID_SOCKET;
	}
	WSACleanup();
}

void CTCPServerSocket::InitCompletionPort()
{
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, m_nSystemInfo);
	if (!m_hCompletionPort)
	{
		ErrorMsg("CreateIoCompletionPort error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
		throw 0;
	}

	m_thWorker = new std::thread[m_nSystemInfo];
	// 启动工作者线程
	for (int i = 0; i < m_nSystemInfo; ++i)
	{
		m_thWorker[i] = std::move(std::thread([this]() {
			while (1)
			{
				DWORD dwTrans = 0;
				ClientInfo* pClinet = nullptr; // 完成键
				OverlapptedData* pOverData = nullptr;
				GetQueuedCompletionStatus(this->m_hCompletionPort, &dwTrans, (PULONG_PTR)(&pClinet), (LPOVERLAPPED*)&pOverData, INFINITE);
				if (pClinet) // 读取该socket的数据
				{
					char recvBuff[1024] = { 0x00 };
					int nLen = recv(pClinet->sClientSocket, recvBuff, sizeof(recvBuff), 0);
					if (nLen == 0) // socket 已关闭
					{
						m_clientMange.DeleteClient(pClinet);
						continue;
					}
					else
					{
						sockaddr_in clientAddr;
						int         nLen = sizeof(sockaddr_in);
						getpeername(pClinet->sClientSocket, (sockaddr*)&clientAddr, &nLen);
						char szAddr[128] = { 0x00 };
						inet_ntop(AF_INET, &clientAddr.sin_addr, szAddr, sizeof(sockaddr_in));
						std::cout << "recv data from " << szAddr << ":" << ntohs(clientAddr.sin_port) << " " << recvBuff << std::endl;
						// 投递下一个接收请求

						OverlapptedData* lapptedData = new OverlapptedData;
						lapptedData->WsaBuff.buf = lapptedData->szRecvBuff;
						lapptedData->WsaBuff.len = 0;
						DWORD dwTrans = 0, dwFlag = 0;
						int nErr = WSARecv(pClinet->sClientSocket, &lapptedData->WsaBuff, 1, &dwTrans, &dwFlag, (LPWSAOVERLAPPED)lapptedData, NULL);
						if (nErr < 0)
						{
							nErr = WSAGetLastError();
							if (nErr != WSA_IO_PENDING) // pending是正常的
							{
								m_clientMange.DeleteClient(pClinet);
								delete lapptedData;
							}
						}

					}
				}
				if (pOverData)
					delete pOverData;
				else // 收到退出消息
					return;
			}
			
		}));
	}

}

bool CTCPServerSocket::Init()
{
	try 
	{
		InitCompletionPort();

		WSADATA wsaData;
		int     nErr = 0;
		nErr = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (nErr != 0)
		{
			ErrorMsg("WSAStartup error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
			throw 0;
		}

		m_sServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_sServerSocket == INVALID_SOCKET)
		{
			ErrorMsg("socket error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
			throw 0;
		}

		sockaddr_in sockAddr;
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = htons(m_nPort);
		nErr = inet_pton(AF_INET, m_strIP.c_str(), &sockAddr.sin_addr);
		if (nErr < 0)
		{
			ErrorMsg("inet_pton error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
			throw 0;
		}

		nErr = bind(m_sServerSocket, (sockaddr*)(&sockAddr), sizeof(sockaddr_in));
		if (nErr < 0)
		{
			ErrorMsg("inet_pton error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
			throw 0;
		}

		nErr = listen(m_sServerSocket, 5);
		if (nErr != 0)
		{
			ErrorMsg("inet_pton error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
			throw 0;
		}

		m_bRun = true;
		m_thAccept = std::move(std::thread([this]() { // 接收线程
			while (1)
			{
				ClientInfo* pClinet = m_clientMange.GetClient();
				if (!pClinet) // 分配失败
					continue;
				sockaddr_in client;
				int      nLen = sizeof(sockaddr_in);
				pClinet->sClientSocket = accept(m_sServerSocket, (sockaddr*)&client, &nLen);

				if (m_sServerSocket == INVALID_SOCKET) // 退出条件
					return;

				if (pClinet->sClientSocket == INVALID_SOCKET)
				{
					m_clientMange.DeleteClient(pClinet);
					continue;
				}
				// 添加到完成端口 完成键是pClinet->sClientSocket
				HANDLE hAcceptCompletionHandle = nullptr;
				hAcceptCompletionHandle = CreateIoCompletionPort((HANDLE)(pClinet->sClientSocket), m_hCompletionPort, ULONG_PTR(pClinet), m_nSystemInfo);
				if (hAcceptCompletionHandle)
				{
					// 投递接收数据请求
					OverlapptedData* lapptedData = new OverlapptedData;
					lapptedData->WsaBuff.buf = lapptedData->szRecvBuff;
					lapptedData->WsaBuff.len = 0;
					DWORD dwTrans = 0, dwFlag = 0;
					int nErr = WSARecv(pClinet->sClientSocket, &lapptedData->WsaBuff, 1, &dwTrans, &dwFlag, &(lapptedData->Overlapped), NULL);
					if (nErr < 0)
					{
						nErr = WSAGetLastError();
						if (nErr != WSA_IO_PENDING) // pending是正常的
						{
							m_clientMange.DeleteClient(pClinet);
							delete lapptedData;
						}
					}
				}
				else // 绑定完成端口失败
				{
					m_clientMange.DeleteClient(pClinet);
					continue;
				}

			}
		}));
		return true;
	}
	catch (...)
	{
		CleanSocket();
		return false;
	}
}

bool CTCPServerSocket::Stop()
{
	CleanSocket(); // 先关闭监听socket
	m_thAccept.join(); // 等待监听线程结束
	for (int i = 0; i < m_nSystemInfo; ++i)
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL); // 唤醒工作者线程
	for (int i = 0; i < m_nSystemInfo; ++i)
		m_thWorker[i].join(); // 等待线程结束
	delete[] m_thWorker;
	return true;
}
