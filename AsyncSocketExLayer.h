/*CAsyncSocketEx by Tim Kosse (Tim.Kosse@gmx.de)
            Version 1.1 (2002-11-01)
--------------------------------------------------------

Introduction:
-------------

CAsyncSocketEx is a replacement for the MFC class CAsyncSocket.
This class was written because CAsyncSocket is not the fastest WinSock
wrapper and it's very hard to add new functionality to CAsyncSocket
derived classes. This class offers the same functionality as CAsyncSocket.
Also, CAsyncSocketEx offers some enhancements which were not possible with
CAsyncSocket without some tricks.

How do I use it?
----------------
Basically exactly like CAsyncSocket.
To use CAsyncSocketEx, just replace all occurrences of CAsyncSocket in your
code with CAsyncSocketEx, if you did not enhance CAsyncSocket yourself in
any way, you won't have to change anything else in your code.

Why is CAsyncSocketEx faster?
-----------------------------

CAsyncSocketEx is slightly faster when dispatching notification event messages.
First have a look at the way CAsyncSocket works. For each thread that uses
CAsyncSocket, a window is created. CAsyncSocket calls WSAAsyncSelect with
the handle of that window. Until here, CAsyncSocketEx works the same way.
But CAsyncSocket uses only one window message (WM_SOCKET_NOTIFY) for all
sockets within one thread. When the window receive WM_SOCKET_NOTIFY, wParam
contains the socket handle and the window looks up an CAsyncSocket instance
using a map. CAsyncSocketEx works differently. It's helper window uses a
wide range of different window messages (WM_USER through 0xBFFF) and passes
a different message to WSAAsyncSelect for each socket. When a message in
the specified range is received, CAsyncSocketEx looks up the pointer to a
CAsyncSocketEx instance in an Array using the index of message - WM_USER.
As you can see, CAsyncSocketEx uses the helper window in a more efficient
way, as it don't have to use the slow maps to lookup it's own instance.
Still, speed increase is not very much, but it may be noticeable when using
a lot of sockets at the same time.
Please note that the changes do not affect the raw data throughput rate,
CAsyncSocketEx only dispatches the notification messages faster.

What else does CAsyncSocketEx offer?
------------------------------------

CAsyncSocketEx offers a flexible layer system. One example is the proxy layer.
Just create an instance of the proxy layer, configure it and add it to the layer
chain of your CAsyncSocketEx instance. After that, you can connect through
proxies.
Benefit: You don't have to change much to use the layer system.
Another layer that is currently in development is the SSL layer to establish
SSL encrypted connections.

License
-------

Feel free to use this class, as long as you don't claim that you wrote it
and this copyright notice stays intact in the source files.
If you use this class in commercial applications, please send a short message
to tim.kosse@gmx.de

*/
#pragma once
#include "AsyncSocketEx.h"	// Hinzugefügt von der Klassenansicht

class CAsyncSocketEx;
class CAsyncSocketExLayer
{
	friend CAsyncSocketEx;
	friend CAsyncSocketExHelperWindow;
protected:
	//Protected constructor so that CAsyncSocketExLayer can't be instantiated
	CAsyncSocketExLayer();
	virtual ~CAsyncSocketExLayer();

	//Notification event handlers
	virtual void OnAccept(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);

	//Operations
	virtual BOOL Accept(CAsyncSocketEx& rConnectedSocket, SOCKADDR* lpSockAddr = NULL, int* lpSockAddrLen = NULL);
	virtual void Close();
	virtual BOOL Connect(LPCSTR lpszHostAddress, UINT nHostPort);
	virtual BOOL Connect(const SOCKADDR* lpSockAddr, int nSockAddrLen);
	virtual BOOL Create(UINT nSocketPort = 0, int nSocketType = SOCK_STREAM,
						long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE,
						LPCSTR lpszSocketAddress = NULL );
	virtual BOOL GetPeerName(SOCKADDR* lpSockAddr, int* lpSockAddrLen);
#ifdef _AFX
	virtual BOOL GetPeerName(CString& rPeerAddress, UINT& rPeerPort);
#endif
	virtual BOOL Listen(int nConnectionBacklog);
	virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	virtual BOOL ShutDown(int nHow = sends);
	enum { receives = 0, sends = 1, both = 2 };

	//Functions that will call next layer
	BOOL ShutDownNext(int nHow = sends);
	BOOL AcceptNext(CAsyncSocketEx& rConnectedSocket, SOCKADDR* lpSockAddr = NULL, int* lpSockAddrLen = NULL);
	void CloseNext();
	BOOL ConnectNext(LPCSTR lpszHostAddress, UINT nHostPort);
	BOOL ConnectNext(const SOCKADDR* lpSockAddr, int nSockAddrLen);
	BOOL CreateNext(UINT nSocketPort, int nSocketType, long lEvent, LPCSTR lpszSocketAddress);
#ifdef _AFX
	BOOL GetPeerNameNext(CString& rPeerAddress, UINT& rPeerPort);
#endif
	BOOL GetPeerNameNext(SOCKADDR* lpSockAddr, int* lpSockAddrLen);
	BOOL ListenNext( int nConnectionBacklog);
	int ReceiveNext(void *lpBuf, int nBufLen, int nFlags = 0);
	int SendNext(const void *lpBuf, int nBufLen, int nFlags = 0);

	CAsyncSocketEx *m_pOwnerSocket;

	//Calls OnLayerCallback on owner socket
	int DoLayerCallback(int nType, int nCode, WPARAM wParam = 0, LPARAM lParam = 0);

	int GetLayerState();
	BOOL TriggerEvent(long lEvent, int nErrorCode, BOOL bPassThrough = FALSE);

	enum LayerState
	{
		notsock,
		unconnected,
		connecting,
		listening,
		connected,
		closed,
		aborted
	};

private:
	//Layer state can't be set directly from derived classes
	void SetLayerState(int nLayerState);
	int m_nLayerState;

	//Called by helper window, dispatches event notification and updated layer state
	void CallEvent(int nEvent, int nErrorCode);

	int m_nCriticalError;

	void Init(CAsyncSocketExLayer *pPrevLayer, CAsyncSocketEx *pOwnerSocket);
	CAsyncSocketExLayer *AddLayer(CAsyncSocketExLayer *pLayer, CAsyncSocketEx *pOwnerSocket);

	CAsyncSocketExLayer *m_pNextLayer;
	CAsyncSocketExLayer *m_pPrevLayer;

	struct t_LayerNotifyMsg
	{
		CAsyncSocketExLayer *pLayer;
		long lEvent;
	};
};
