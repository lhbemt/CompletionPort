#include "TCPClient.h"

int main()
{
	CTCPClient tcpClient;
	std::string ip = "192.168.1.106";
	bool bRet = tcpClient.Connect(ip, 8080);
	if (!bRet)
	{
		std::cout << "connect server error: " << tcpClient.GetErrorMsg() << std::endl;
		return 0;
	}
	for (int i = 0; i < 10; ++i)
	{
		int nLen = tcpClient.SendData("hello world", strlen("hello world"));
		if (nLen <= 0)
		{
			tcpClient.DisConnect();
			std::cout << "connect server error: " << tcpClient.GetErrorMsg() << std::endl;
			return 0;
		}
	}

	tcpClient.DisConnect();

	return 0;
}