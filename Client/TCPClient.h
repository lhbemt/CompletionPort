#ifndef __CTCPCLIENT__H
#define __CTCPCLIENT__H
#include <WS2tcpip.h>
#include <Windows.h>
#include <WinSock2.h>
#include <iostream>

class CTCPClient
{
public:
	CTCPClient();
	~CTCPClient();

public:
	bool Connect(std::string& strIP, int nPort);
	void DisConnect();
	int  SendData(void* pBuff, int nLen);
	const char* GetErrorMsg()
	{
		return m_strErrorMsg.c_str();
	}
private:
	bool Init();
	void CleanUp();
	void ErrorMsg(const char* err, ...);
private:
	std::string m_strErrorMsg;
	SOCKET      m_sSocket;
};

#endif