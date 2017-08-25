#define _CRT_SECURE_NO_WARNINGS
#include "TCPClient.h"


CTCPClient::CTCPClient()
{
}


CTCPClient::~CTCPClient()
{
}

void CTCPClient::ErrorMsg(const char* err, ...)
{
	char szErrorMsg[2048] = { 0x00 };
	va_list arg;
	va_start(arg, err);
	vsprintf_s(szErrorMsg, sizeof(szErrorMsg), err, arg);
	m_strErrorMsg = szErrorMsg;
}

bool CTCPClient::Init()
{
	WSADATA wsaData;
	int     nErr = 0;
	nErr = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nErr != 0)
	{
		ErrorMsg("WSAStartup error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
		return false;
	}

	m_sSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sSocket == INVALID_SOCKET)
	{
		ErrorMsg("socket error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
		return false;
	}
	return true;
}

bool CTCPClient::Connect(std::string& strIP, int nPort)
{
	bool bRet = Init();
	if (!bRet)
		return false;

	int nErr = 0;
	sockaddr_in sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(nPort);
	nErr = inet_pton(AF_INET, strIP.c_str(), &sockAddr.sin_addr);
	if (nErr < 0)
	{
		ErrorMsg("inet_pton error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
		return false;
	}

	nErr = connect(m_sSocket, (sockaddr*)(&sockAddr), sizeof(sockaddr_in));
	if (nErr == -1)
	{
		ErrorMsg("connect error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
		return false;
	}
	return true;
}

void CTCPClient::CleanUp()
{
	closesocket(m_sSocket);
	WSACleanup();
}

void CTCPClient::DisConnect()
{
	CleanUp();
	m_sSocket = INVALID_SOCKET;
}

int CTCPClient::SendData(void* pBuff, int nLen)
{
	int nErr = send(m_sSocket, (char*)pBuff, nLen, 0);
	if (nErr == SOCKET_ERROR)
	{
		ErrorMsg("send error, errorcode: %d, errormsg: %s", GetLastError(), strerror(GetLastError()));
		return -1;
	}
	return nErr;
}