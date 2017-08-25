#ifndef __CTCPSERVERSOCKET__H
#define __CTCPSERVERSOCKET__H
#include <iostream>
#include <atomic>
#include <mutex>
#include "TCPClientSocketManage.h"


// 重叠数据结构定义
struct OverlapptedData
{
	OVERLAPPED Overlapped; // 重叠数据
	WSABUF WsaBuff; // 接收缓冲区
	char   szRecvBuff[1024]; // 接收缓存
	OverlapptedData()
	{
		memset(this, 0, sizeof(OverlapptedData));
	}
};

// 服务端模块

class CTCPServerSocket
{
public:
	CTCPServerSocket(std::string& strIP, int nPort);
	~CTCPServerSocket();

public:
	bool Start()
	{
		return Init();
	}

	const char* GetErrorMsg()
	{
		return m_strErrorMsg.c_str();
	}

	bool Stop(); // 退出

private:
	bool Init(); // 初始化socket
	void InitCompletionPort(); // 初始化完成端口

private:
	void ErrorMsg(const char* err, ...);
	void CleanSocket();

private:
	std::string m_strIP;
	int         m_nPort;
	std::string m_strErrorMsg; // 错误代码
	SOCKET      m_sServerSocket;
	std::atomic<bool> m_bRun;
	HANDLE      m_hCompletionPort; // 完成端口
	int         m_nSystemInfo; // 处理器核心个数
	std::thread* m_thWorker; // 工作者线程
	std::thread m_thAccept; // accept线程

	CTCPClientSocketManage m_clientMange;
};

#endif