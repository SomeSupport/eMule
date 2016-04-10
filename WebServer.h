#pragma once

#include <zlib/zlib.h>
#include "WebSocket.h"
#include "PartFile.h"
//#include "Loggable.h"

#define WEB_GRAPH_HEIGHT		120
#define WEB_GRAPH_WIDTH			500

#define SESSION_TIMEOUT_SECS	300	// 5 minutes session expiration
#define SHORT_LENGTH_MAX		60	// Max size for strings maximum
#define SHORT_LENGTH			40	// Max size for strings
#define SHORT_LENGTH_MIN		30	// Max size for strings minimum

typedef struct
{
	double download;
	double upload;
	long connections;
} UpDown;

typedef struct
{
	CTime	startTime;
	long	lSession;
	bool	admin;
	int		lastcat;

} Session;

struct BadLogin {
	uint32	datalen;
	DWORD	timestamp;
};

typedef struct
{
	CString	sFileName;
	CString sFileNameJS;
	CString	sFileType;
	CString	sFileState;
	int		iFileState;
	CString sCategory;
	uint64	m_qwFileSize;
	uint64	m_qwFileTransferred;
	long	lFileSpeed;
	long	lSourceCount;
	long	lNotCurrentSourceCount;
	long	lTransferringSourceCount;
	double	m_dblCompleted;
	int		nFileStatus;
	int		nFilePrio;
	CString	sFileHash;
	CString	sED2kLink;
	CString	sFileInfo;
	bool	bFileAutoPrio;
	bool	bIsComplete;
	bool	bIsPreview;
	bool	bIsGetFLC;
	int		iComment;
} DownloadFiles;

typedef struct
{
	bool	bIsPartFile;
	CString	sFileState;
	CString	sFileName;
	CString sFileType;
	uint64	m_qwFileSize;
	uint64	nFileTransferred;
	uint64	nFileAllTimeTransferred;
	UINT	nFileRequests;
	uint32	nFileAllTimeRequests;
	UINT	nFileAccepts;
	uint32	nFileAllTimeAccepts;
	CString sFileCompletes;
	double	dblFileCompletes;
	byte	nFilePriority;
	CString sFilePriority;
	bool	bFileAutoPriority;
	CString sFileHash;
	CString	sED2kLink;
} SharedFiles;

typedef struct
{
	CString	sClientState;
	CString	sUserHash;
	CString	sActive;
	CString sFileInfo;
	CString sClientSoft;
	CString	sClientExtra;
	CString	sUserName;
	CString	sFileName;
	uint32	nTransferredDown;
	uint32	nTransferredUp;
	int		nDataRate;
	CString	sClientNameVersion;
} UploadUsers;

typedef struct
{
	CString	sClientState;
	CString	sClientStateSpecial;
	CString	sUserHash;
	CString sClientSoft;
	CString sClientSoftSpecial;
	CString	sClientExtra;
	CString	sUserName;
	CString	sFileName;
	CString	sClientNameVersion;
	uint32	nScore;
	CString sIndex;	//SyruS CQArray-Sorting element
} QueueUsers;

typedef enum
{
	DOWN_SORT_STATE,
	DOWN_SORT_TYPE,
	DOWN_SORT_NAME,
	DOWN_SORT_SIZE,
	DOWN_SORT_TRANSFERRED,
	DOWN_SORT_SPEED,
	DOWN_SORT_PROGRESS,
	DOWN_SORT_SOURCES,
	DOWN_SORT_PRIORITY,
	DOWN_SORT_CATEGORY,
	DOWN_SORT_FAKECHECK
} DownloadSort;

typedef enum
{
	UP_SORT_CLIENT,
	UP_SORT_USER,
	UP_SORT_VERSION,
	UP_SORT_FILENAME,
	UP_SORT_TRANSFERRED,
	UP_SORT_SPEED
} UploadSort;

typedef enum
{
	QU_SORT_CLIENT,
	QU_SORT_USER,
	QU_SORT_VERSION,
	QU_SORT_FILENAME,
	QU_SORT_SCORE
} QueueSort;

typedef enum
{
	SHARED_SORT_STATE,
	SHARED_SORT_TYPE,
	SHARED_SORT_NAME,
	SHARED_SORT_SIZE,
	SHARED_SORT_TRANSFERRED,
	SHARED_SORT_ALL_TIME_TRANSFERRED,
	SHARED_SORT_REQUESTS,
	SHARED_SORT_ALL_TIME_REQUESTS,
	SHARED_SORT_ACCEPTS,
	SHARED_SORT_ALL_TIME_ACCEPTS,
	SHARED_SORT_COMPLETES,
	SHARED_SORT_PRIORITY
} SharedSort;

typedef struct
{
	CString	sServerName;
	CString	sServerDescription;
	int		nServerPort;
	CString	sServerIP;
	CString sServerFullIP;
	CString sServerState;
	bool    bServerStatic;
	uint32		nServerUsers;
	uint32		nServerMaxUsers;
	uint32		nServerFiles;
	CString sServerPriority;
	byte   nServerPriority;
	int		nServerPing;
	int		nServerFailed;
	uint32		nServerSoftLimit;
	uint32		nServerHardLimit;
	CString	sServerVersion;
} ServerEntry;

typedef enum
{
	SERVER_SORT_STATE,
	SERVER_SORT_NAME,
	SERVER_SORT_IP,
	SERVER_SORT_DESCRIPTION,
	SERVER_SORT_PING,
	SERVER_SORT_USERS,
	SERVER_SORT_FILES,
	SERVER_SORT_PRIORITY,
	SERVER_SORT_FAILED,
	SERVER_SORT_LIMIT,
	SERVER_SORT_VERSION
} ServerSort;

typedef struct
{
	uint32			nUsers;
	DownloadSort	DownloadSort;
	bool			bDownloadSortReverse;
	UploadSort		UploadSort;
	bool			bUploadSortReverse;
	QueueSort		QueueSort;
	bool			bQueueSortReverse;
	ServerSort		ServerSort;
	bool			bServerSortReverse;
	SharedSort		SharedSort;
	bool			bSharedSortReverse;	
	bool			bShowUploadQueue;
	bool			bShowUploadQueueBanned;
	bool			bShowUploadQueueFriend;
	bool			bShowTransferLine;//Purity: Action Buttons
	CString			sShowTransferFile;//Purity: Action Buttons
	bool			bShowServerLine;//Purity: Action Buttons
	CString			sShowServerIP;//Purity: Action Buttons
	bool			bShowSharedLine;//Purity: Action Buttons
	CString			sShowSharedFile;//Purity: Action Buttons

	CArray<UpDown>	PointsForWeb;
	CArray<Session>	Sessions;

	CString			sLastModified;
	CString			sETag;

	CArray<BadLogin>	badlogins;	//TransferredData= IP : time

} GlobalParams;

typedef struct
{
	CString			sURL;
	void			*pThis;
	CWebSocket		*pSocket;
	in_addr			inadr;
} ThreadData;

typedef struct
{
	CString	sHeader;
	CString	sHeaderStylesheet;
	CString	sFooter;
	CString	sServerList;
	CString	sServerLine;
	CString	sTransferImages;
	CString	sTransferList;
	CString	sTransferDownHeader;
	CString	sTransferDownFooter;
	CString	sTransferDownLine;
	CString	sTransferUpHeader;
	CString	sTransferUpFooter;
	CString	sTransferUpLine;
	CString	sTransferUpQueueShow;
	CString	sTransferUpQueueHide;
	CString	sTransferUpQueueLine;
	CString	sTransferUpQueueBannedShow;
	CString	sTransferUpQueueBannedHide;
	CString	sTransferUpQueueBannedLine;
	CString	sTransferUpQueueFriendShow;
	CString	sTransferUpQueueFriendHide;
	CString	sTransferUpQueueFriendLine;
	CString	sSharedList;
	CString	sSharedLine;
	CString	sGraphs;
	CString	sLog;
	CString	sServerInfo;
	CString sDebugLog;
	CString sStats;
	CString sPreferences;
	CString	sLogin;
	CString	sAddServerBox;
	CString	sSearch;
	CString	sProgressbarImgs;
	CString sProgressbarImgsPercent;
	uint16	iProgressbarWidth;
	CString sSearchResultLine;
	CString sSearchHeader;
	CString sCatArrow;
	CString sDownArrow;
	CString sUpArrow;
	CString strDownDoubleArrow;
	CString strUpDoubleArrow;
	CString	sKad;
	CString sBootstrapLine;
	CString sMyInfoLog;
	CString sCommentList;
	CString sCommentListLine;
} WebTemplates;

class CWebServer 
{
	friend class CWebSocket;

public:
	CWebServer(void);
	~CWebServer(void);

	inline void SetIP(ULONG ip) { m_ulCurIP = ip; }
	int	 UpdateSessionCount();
	void StartServer(void);
	void RestartServer();
	void AddStatsLine(UpDown line);
	void ReloadTemplates();
	UINT GetSessionCount()	{ return m_Params.Sessions.GetCount();}
	bool IsRunning()	{ return m_bServerWorking;}
	CArray<UpDown>* GetPointsForWeb()	{return &m_Params.PointsForWeb;} // MobileMule
protected:
	static void		ProcessURL(ThreadData);
	static void		ProcessFileReq(ThreadData Data);

private:
	static CString	_GetHeader(ThreadData, long lSession);
	static CString	_GetFooter(ThreadData);
	static CString	_GetServerList(ThreadData);
	static CString	_GetTransferList(ThreadData);
	static CString	_GetSharedFilesList(ThreadData);
	static CString	_GetGraphs(ThreadData);
	static CString	_GetLog(ThreadData);
	static CString	_GetServerInfo(ThreadData);
	static CString	_GetDebugLog(ThreadData);
	static CString	_GetStats(ThreadData);
	static CString  _GetKadDlg(ThreadData);
	static CString	_GetPreferences(ThreadData);
	static CString	_GetLoginScreen(ThreadData);
	static CString	_GetConnectedServer(ThreadData);
	static CString 	_GetAddServerBox(ThreadData Data);
	static CString	_GetCommentlist(ThreadData Data);
	static void		_RemoveServer(CString sIP, int nPort);
	static void		_AddToStatic(CString sIP, int nPort);
	static void		_RemoveFromStatic(CString sIP, int nPort);
	static uchar*	_GetFileHash(CString sHash, uchar *FileHash);

	static CString	_GetWebSearch(ThreadData Data);
	static CString	_GetSearch(ThreadData Data);

	static CString	_ParseURL(CString URL, CString fieldname); 
	static CString	_ParseURLArray(CString URL, CString fieldname);
	static void		_ConnectToServer(CString sIP, int nPort);
	static bool		_IsLoggedIn(ThreadData Data, long lSession);
	static void		_RemoveTimeOuts(ThreadData Data);
	static bool		_RemoveSession(ThreadData Data, long lSession);
	static CString	_SpecialChars(CString str, bool noquote = true);
	static CString	_GetPlainResString(UINT nID, bool noquote = true);
	static void		_GetPlainResString(CString *pstrOut, UINT nID, bool noquote = true);
	static int		_GzipCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
	CString			_LoadTemplate(CString sAll, CString sTemplateName);
	static Session	GetSessionByID(ThreadData Data,long sessionID);
	static bool		IsSessionAdmin(ThreadData Data, const CString &strSsessionID);
	static CString	GetPermissionDenied();
	static CString	_GetDownloadGraph(ThreadData Data,CString filehash);
	static void		InsertCatBox(CString &Out,int preselect,CString boxlabel, bool jump,bool extraCats,CString sSession,CString sFileHash,bool ed2kbox=false);
	static CString	GetSubCatLabel(int iCat);
	static CString  _GetRemoteLinkAddedOk(ThreadData Data);
	static CString  _GetRemoteLinkAddedFailed(ThreadData Data);
	static void		_SetLastUserCat(ThreadData Data, long lSession,int cat);
	static int		_GetLastUserCat(ThreadData Data, long lSession);
	static CString	_CreateTransferList(CString Out, CWebServer *pThis, ThreadData* Data, void* FilesArray, void* UploadArray, bool bAdmin);

private:
	static void		SaveWIConfigArray(BOOL array[], int size, LPCTSTR key);
	static CString	GetWebImageNameForFileType(CString filename);
	static CString  GetClientSummary(CUpDownClient* client);
	static CString	_GetMyInfo(ThreadData Data);
	static CString	GetClientversionImage(CUpDownClient* client);


// Common data
	GlobalParams	m_Params;
	WebTemplates	m_Templates;
	bool			m_bServerWorking;
	int				m_iSearchSortby;
	bool			m_bSearchAsc;
	uint16			m_nIntruderDetect;
	bool			m_bIsTempDisabled;
	uint32			m_nStartTempDisabledTime;
	bool			GetIsTempDisabled() { return m_bIsTempDisabled; }
	ULONG			m_ulCurIP;
};
