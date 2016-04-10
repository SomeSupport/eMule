#include <stdafx.h>
#include "emule.h"
#include "OtherFunctions.h"
#include "WebSocket.h"
#include "WebServer.h"
#include "Preferences.h"
#include "StringConversion.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static HANDLE s_hTerminate = NULL;
static CWinThread* s_pSocketThread = NULL;

typedef struct
{
	void	*pThis;
	SOCKET	hSocket;
	in_addr incomingaddr;
} SocketData;

void CWebSocket::SetParent(CWebServer *pParent)
{
	m_pParent = pParent;
}

void CWebSocket::OnRequestReceived(char* pHeader, DWORD dwHeaderLen, char* pData, DWORD dwDataLen, in_addr inad)
{
	CStringA sHeader(pHeader, dwHeaderLen);
	CStringA sData(pData, dwDataLen);
	CStringA sURL;
	bool filereq=false;

	if(sHeader.Left(3) == "GET")
		sURL = sHeader.Trim();

	else if(sHeader.Left(4) == "POST")
		sURL = "?" + sData.Trim();	// '?' to imitate GET syntax for ParseURL

	if(sURL.Find(" ") > -1)
		sURL = sURL.Mid(sURL.Find(" ")+1, sURL.GetLength());
	if(sURL.Find(" ") > -1)
		sURL = sURL.Left(sURL.Find(" "));

	if (sURL.GetLength()>4 &&	// min length (for valid extentions)
		(sURL.Right(4).MakeLower()==".gif" || sURL.Right(4).MakeLower()==".jpg" || sURL.Right(4).MakeLower()==".png" ||
		sURL.Right(4).MakeLower()==".ico" ||sURL.Right(4).MakeLower()==".css" ||sURL.Right(3).MakeLower()==".js" ||
		sURL.Right(4).MakeLower()==".bmp" || sURL.Right(5).MakeLower()==".jpeg"
		)
		&& sURL.Find("..")==-1	// dont allow leaving the emule-webserver-folder for accessing files
		)
			filereq=true;

	ThreadData Data;
	Data.sURL = sURL;
	Data.pThis = m_pParent;
	Data.inadr = inad;
	Data.pSocket = this;

	if (!filereq)
		m_pParent->ProcessURL(Data);
	else
		m_pParent->ProcessFileReq(Data);

	Disconnect();
}

void CWebSocket::OnReceived(void* pData, DWORD dwSize, in_addr inad)
{
	const UINT SIZE_PRESERVE = 0x1000;

	if (m_dwBufSize < dwSize + m_dwRecv)
	{
		// reallocate
		char* pNewBuf = new char[m_dwBufSize = dwSize + m_dwRecv + SIZE_PRESERVE];
		if (!pNewBuf)
		{
			m_bValid = false; // internal problem
			return;
		}

		if (m_pBuf)
		{
			CopyMemory(pNewBuf, m_pBuf, m_dwRecv);
			delete[] m_pBuf;
		}

		m_pBuf = pNewBuf;
	}
	CopyMemory(m_pBuf + m_dwRecv, pData, dwSize);
	m_dwRecv += dwSize;

	// check if we have all that we want
	if (!m_dwHttpHeaderLen)
	{
		// try to find it
		bool bPrevEndl = false;
		for (DWORD dwPos = 0; dwPos < m_dwRecv; dwPos++)
			if ('\n' == m_pBuf[dwPos])
				if (bPrevEndl)
				{
					// We just found the end of the http header
					// Now write the message's position into two first DWORDs of the buffer
					m_dwHttpHeaderLen = dwPos + 1;

					// try to find now the 'Content-Length' header
					for (dwPos = 0; dwPos < m_dwHttpHeaderLen; )
					{
						// Elandal: pPtr is actually a char*, not a void*
						char* pPtr = (char*)memchr(m_pBuf + dwPos, '\n', m_dwHttpHeaderLen - dwPos);
						if (!pPtr)
							break;
						// Elandal: And thus now the pointer substraction works as it should
						DWORD dwNextPos = pPtr - m_pBuf;

						// check this header
						char szMatch[] = "content-length";
						if (!_strnicmp(m_pBuf + dwPos, szMatch, sizeof(szMatch) - 1))
						{
							dwPos += sizeof(szMatch) - 1;
							pPtr = (char*)memchr(m_pBuf + dwPos, ':', m_dwHttpHeaderLen - dwPos);
							if (pPtr)
								m_dwHttpContentLen = atol((pPtr) + 1);

							break;
						}
						dwPos = dwNextPos + 1;
					}

					break;
				}
				else
				{
					bPrevEndl = true;
				}
			else
				if ('\r' != m_pBuf[dwPos])
					bPrevEndl = false;

	}
	if (m_dwHttpHeaderLen && !m_bCanRecv && !m_dwHttpContentLen)
		m_dwHttpContentLen = m_dwRecv - m_dwHttpHeaderLen; // of course

	if (m_dwHttpHeaderLen && m_dwHttpContentLen < m_dwRecv && (!m_dwHttpContentLen || (m_dwHttpHeaderLen + m_dwHttpContentLen <= m_dwRecv)))
	{
		OnRequestReceived(m_pBuf, m_dwHttpHeaderLen, m_pBuf + m_dwHttpHeaderLen, m_dwHttpContentLen, inad);

		if (m_bCanRecv && (m_dwRecv > m_dwHttpHeaderLen + m_dwHttpContentLen))
		{
			// move our data
			m_dwRecv -= m_dwHttpHeaderLen + m_dwHttpContentLen;
			MoveMemory(m_pBuf, m_pBuf + m_dwHttpHeaderLen + m_dwHttpContentLen, m_dwRecv);
		} else
			m_dwRecv = 0;

		m_dwHttpHeaderLen = 0;
		m_dwHttpContentLen = 0;
	}

}

void CWebSocket::SendData(const void* pData, DWORD dwDataSize)
{
	ASSERT(pData);
	if (m_bValid && m_bCanSend)
	{
		if (!m_pHead)
		{
			// try to send it directly
			//-- remember: in "nRes" could be "-1" after "send" call
			int nRes = send(m_hSocket, (const char*) pData, dwDataSize, 0);

			if (((nRes < 0) || (nRes > (signed) dwDataSize)) && (WSAEWOULDBLOCK != WSAGetLastError()))
			{
				m_bValid = false;
			}
			else
			{                
				//-- in nRes still could be "-1" (if WSAEWOULDBLOCK occured)
				//-- next to line should be like this:

				((const char*&) pData) += (nRes == -1 ? 0 : nRes);
				dwDataSize -= (nRes == -1 ? 0 : nRes);

				//-- ... and not like this:
				//-- ((const char*&) pData) += nRes;
				//-- dwDataSize -= nRes;

			}
		}

		if (dwDataSize && m_bValid)
		{
			// push it to our tails
			CChunk* pChunk = new CChunk;
			if (pChunk)
			{
				pChunk->m_pNext = NULL;
				pChunk->m_dwSize = dwDataSize;
				if ((pChunk->m_pData = new char[dwDataSize]) != NULL)
				{
					//-- data should be copied into "pChunk->m_pData" anyhow
					//-- possible solution is simple:

					CopyMemory(pChunk->m_pData, pData, dwDataSize);

					// push it to the end of our queue
					pChunk->m_pToSend = pChunk->m_pData;
					if (m_pTail)
						m_pTail->m_pNext = pChunk;
					else
						m_pHead = pChunk;
					m_pTail = pChunk;

				} else
					delete pChunk; // oops, no memory (???)
			}
		}
	}
}

void CWebSocket::SendReply(LPCSTR szReply)
{
	char szBuf[256];
	int nLen = _snprintf(szBuf, _countof(szBuf), "%s\r\n", szReply);
	if (nLen > 0)
		SendData(szBuf, nLen);
}

void CWebSocket::SendContent(LPCSTR szStdResponse, const void* pContent, DWORD dwContentSize)
{
	char szBuf[0x1000];
	int nLen = _snprintf(szBuf, _countof(szBuf), "HTTP/1.1 200 OK\r\n%sContent-Length: %ld\r\n\r\n", szStdResponse, dwContentSize);
	if (nLen > 0) {
		SendData(szBuf, nLen);
		SendData(pContent, dwContentSize);
	}
}

void CWebSocket::SendContent(LPCSTR szStdResponse, const CString& rstr)
{
	CStringA strA(wc2utf8(rstr));
	SendContent(szStdResponse, strA, strA.GetLength());
}

void CWebSocket::Disconnect()
{
	if (m_bValid && m_bCanSend)
	{
		m_bCanSend = false;
		if (m_pTail)
		{
			// push it as a tail
			CChunk* pChunk = new CChunk;
			if (pChunk)
			{
				pChunk->m_dwSize = 0;
				pChunk->m_pData = NULL;
				pChunk->m_pToSend = NULL;
				pChunk->m_pNext = NULL;

				m_pTail->m_pNext = pChunk;
			}

		} else
			if (shutdown(m_hSocket, SD_SEND))
				m_bValid = false;
	}
}

UINT AFX_CDECL WebSocketAcceptedFunc(LPVOID pD)
{
	DbgSetThreadName("WebSocketAccepted");

	srand(time(NULL));
	InitThreadLocale();

	SocketData *pData = (SocketData *)pD;
	SOCKET hSocket = pData->hSocket;
	CWebServer *pThis = (CWebServer *)pData->pThis;
	in_addr ad=pData->incomingaddr;
	
	delete pData;

	ASSERT(INVALID_SOCKET != hSocket);

	HANDLE hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hEvent)
	{
		if (!WSAEventSelect(hSocket, hEvent, FD_READ | FD_CLOSE | FD_WRITE))
		{
			CWebSocket stWebSocket;
			stWebSocket.SetParent(pThis);
			stWebSocket.m_pHead = NULL;
			stWebSocket.m_pTail = NULL;
			stWebSocket.m_bValid = true;
			stWebSocket.m_bCanRecv = true;
			stWebSocket.m_bCanSend = true;
			stWebSocket.m_hSocket = hSocket;
			stWebSocket.m_pBuf = NULL;
			stWebSocket.m_dwRecv = 0;
			stWebSocket.m_dwBufSize = 0;
			stWebSocket.m_dwHttpHeaderLen = 0;
			stWebSocket.m_dwHttpContentLen = 0;

			HANDLE pWait[] = { hEvent, s_hTerminate };

			while (WAIT_OBJECT_0 == WaitForMultipleObjects(2, pWait, FALSE, INFINITE))
			{
				while (stWebSocket.m_bValid)
				{
					WSANETWORKEVENTS stEvents;
					if (WSAEnumNetworkEvents(hSocket, NULL, &stEvents))
						stWebSocket.m_bValid = false;
					else
					{
						if (!stEvents.lNetworkEvents)
							break; //no more events till now

						if (FD_READ & stEvents.lNetworkEvents)
							for (;;)
							{
								char pBuf[0x1000];
								int nRes = recv(hSocket, pBuf, sizeof(pBuf), 0);
								if (nRes <= 0)
								{
									if (!nRes)
									{
										stWebSocket.m_bCanRecv = false;
										stWebSocket.OnReceived(NULL, 0, ad);
									}
									else
										if (WSAEWOULDBLOCK != WSAGetLastError())
											stWebSocket.m_bValid = false;
									break;
								}
								stWebSocket.OnReceived(pBuf, nRes,ad);
							}

						if (FD_CLOSE & stEvents.lNetworkEvents)
							stWebSocket.m_bCanRecv = false;

						if (FD_WRITE & stEvents.lNetworkEvents)
							// send what is left in our tails
							while (stWebSocket.m_pHead)
							{
								if (stWebSocket.m_pHead->m_pToSend)
								{
									int nRes = send(hSocket, stWebSocket.m_pHead->m_pToSend, stWebSocket.m_pHead->m_dwSize, 0);
									if (nRes != (signed) stWebSocket.m_pHead->m_dwSize)
									{
										if (nRes)
											if ((nRes > 0) && (nRes < (signed) stWebSocket.m_pHead->m_dwSize))
											{
												stWebSocket.m_pHead->m_pToSend += nRes;
												stWebSocket.m_pHead->m_dwSize -= nRes;

											} else
												if (WSAEWOULDBLOCK != WSAGetLastError())
													stWebSocket.m_bValid = false;
										break;
									}
								} else
									if (shutdown(hSocket, SD_SEND))
									{
										stWebSocket.m_bValid = false;
										break;
									}

								// erase this chunk
								CWebSocket::CChunk* pNext = stWebSocket.m_pHead->m_pNext;
								delete stWebSocket.m_pHead;
								stWebSocket.m_pHead = pNext;
								if (stWebSocket.m_pHead == NULL)
									stWebSocket.m_pTail = NULL;
							}
					}
				}

				if (!stWebSocket.m_bValid || (!stWebSocket.m_bCanRecv && !stWebSocket.m_pHead))
					break;
			}

			while (stWebSocket.m_pHead)
			{
				CWebSocket::CChunk* pNext = stWebSocket.m_pHead->m_pNext;
				delete stWebSocket.m_pHead;
				stWebSocket.m_pHead = pNext;
			}
			delete[] stWebSocket.m_pBuf;
		}
		VERIFY( CloseHandle(hEvent) );
	}
	VERIFY( !closesocket(hSocket) );

	return 0;
}

UINT AFX_CDECL WebSocketListeningFunc(LPVOID pThis)
{
	DbgSetThreadName("WebSocketListening");

	srand(time(NULL));
	InitThreadLocale();

	SOCKET hSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	if (INVALID_SOCKET != hSocket)
	{
		SOCKADDR_IN stAddr;
		stAddr.sin_family = AF_INET;
		stAddr.sin_port = htons(thePrefs.GetWSPort());
		if (thePrefs.GetBindAddrA())
			stAddr.sin_addr.S_un.S_addr = inet_addr(thePrefs.GetBindAddrA());
		else
			stAddr.sin_addr.S_un.S_addr = INADDR_ANY;

		if (!bind(hSocket, (sockaddr*)&stAddr, sizeof(stAddr)) && !listen(hSocket, SOMAXCONN))
		{
			HANDLE hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
			if (hEvent)
			{
				if (!WSAEventSelect(hSocket, hEvent, FD_ACCEPT))
				{
					HANDLE pWait[] = { hEvent, s_hTerminate };
					while (WAIT_OBJECT_0 == WaitForMultipleObjects(2, pWait, FALSE, INFINITE))
					{
						for (;;)
						{
							struct sockaddr_in their_addr;
                            int sin_size = sizeof(struct sockaddr_in);

							SOCKET hAccepted = accept(hSocket,(struct sockaddr *)&their_addr, &sin_size);
							if (INVALID_SOCKET == hAccepted)
								break;

							if (thePrefs.GetAllowedRemoteAccessIPs().GetCount() > 0)
							{
								bool bAllowedIP = false;
								for (int i = 0; i < thePrefs.GetAllowedRemoteAccessIPs().GetCount(); i++)
								{
									if (their_addr.sin_addr.S_un.S_addr == thePrefs.GetAllowedRemoteAccessIPs()[i])
									{
										bAllowedIP = true;
										break;
									}
								}
								if (!bAllowedIP) {
									LogWarning(_T("Web Interface: Rejected connection attempt from %s"), ipstr(their_addr.sin_addr.S_un.S_addr));
									VERIFY( !closesocket(hAccepted) );
									break;
								}
							}

							if(thePrefs.GetWSIsEnabled())
							{
								SocketData *pData = new SocketData;
								pData->hSocket = hAccepted;
								pData->pThis = pThis;
								pData->incomingaddr=their_addr.sin_addr;
								
								// - do NOT use Windows API 'CreateThread' to create a thread which uses MFC/CRT -> lot of mem leaks!
								// - 'AfxBeginThread' could be used here, but creates a little too much overhead for our needs.
								CWinThread* pAcceptThread = new CWinThread(WebSocketAcceptedFunc, (LPVOID)pData);
								if (!pAcceptThread->CreateThread())
								{
									delete pAcceptThread;
									pAcceptThread = NULL;
									VERIFY( !closesocket(hAccepted) );
									hAccepted = NULL;
								}
							}
							else
							{
								VERIFY( !closesocket(hAccepted) );
								hAccepted = NULL;
							}
						}
					}
				}
				VERIFY( CloseHandle(hEvent) );
				hEvent = NULL;
			}
		}
		VERIFY( !closesocket(hSocket) );
		hSocket = NULL;
	}

	return 0;
}

void StartSockets(CWebServer *pThis)
{
	ASSERT( s_hTerminate == NULL );
	ASSERT( s_pSocketThread == NULL );
	if ((s_hTerminate = CreateEvent(NULL, TRUE, FALSE, NULL)) != NULL)
	{
		// - do NOT use Windows API 'CreateThread' to create a thread which uses MFC/CRT -> lot of mem leaks!
		// - because we want to wait on the thread handle we have to disable 'CWinThread::m_AutoDelete' -> can't 
		//   use 'AfxBeginThread'
		s_pSocketThread = new CWinThread(WebSocketListeningFunc, (LPVOID)pThis);
		s_pSocketThread->m_bAutoDelete = FALSE;
		if (!s_pSocketThread->CreateThread())
		{
			CloseHandle(s_hTerminate);
			s_hTerminate = NULL;
			delete s_pSocketThread;
			s_pSocketThread = NULL;
		}
	}
}

void StopSockets()
{
	if (s_pSocketThread)
	{
		VERIFY( SetEvent(s_hTerminate) );

		if (s_pSocketThread->m_hThread)
		{
			// because we want to wait on the thread handle we must not use 'CWinThread::m_AutoDelete'.
			// otherwise we may run into the situation that the CWinThread was already auto-deleted and
			// the CWinThread::m_hThread is invalid.
			ASSERT( !s_pSocketThread->m_bAutoDelete );

			DWORD dwWaitRes = WaitForSingleObject(s_pSocketThread->m_hThread, 1300);
			if (dwWaitRes == WAIT_TIMEOUT)
			{
				TRACE("*** Failed to wait for websocket thread termination - Timeout\n");
				VERIFY( TerminateThread(s_pSocketThread->m_hThread, (DWORD)-1) );
				VERIFY( CloseHandle(s_pSocketThread->m_hThread) );
			}
			else if (dwWaitRes == -1)
			{
				TRACE("*** Failed to wait for websocket thread termination - Error %u\n", GetLastError());
				ASSERT(0); // probable invalid thread handle
			}
		}
		delete s_pSocketThread;
		s_pSocketThread = NULL;
	}
	if (s_hTerminate){
		VERIFY( CloseHandle(s_hTerminate) );
		s_hTerminate = NULL;
	}
}
