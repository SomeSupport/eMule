#pragma once

class CWebServer;

void StartSockets(CWebServer *pThis);
void StopSockets();

class CWebSocket 
{
public:
	void SetParent(CWebServer *);
	CWebServer* m_pParent;

	class CChunk 
	{
	public:
		char* m_pData;
		char* m_pToSend;
		DWORD m_dwSize;
		CChunk* m_pNext;

		~CChunk() { delete[] m_pData; }
	};

	CChunk* m_pHead; // tails of what has to be sent
	CChunk* m_pTail;

	char* m_pBuf;
	DWORD m_dwRecv;
	DWORD m_dwBufSize;
	DWORD m_dwHttpHeaderLen;
	DWORD m_dwHttpContentLen;

	bool m_bCanRecv;
	bool m_bCanSend;
	bool m_bValid;
	SOCKET m_hSocket;

	void OnReceived(void* pData, DWORD dwDataSize, in_addr inad); // must be implemented
	void SendData(const void* pData, DWORD dwDataSize);
	void SendData(LPCSTR szText) { SendData(szText, lstrlenA(szText)); }
	void SendContent(LPCSTR szStdResponse, const void* pContent, DWORD dwContentSize);
	void SendContent(LPCSTR szStdResponse, const CString& rstr);
	void SendReply(LPCSTR szReply);
	void Disconnect();

	void OnRequestReceived(char* pHeader, DWORD dwHeaderLen, char* pData, DWORD dwDataLen , in_addr inad);
};
