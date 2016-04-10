#pragma once

class CPerfLog
{
public:
	CPerfLog();

	void Startup();
	void Shutdown();
	void LogSamples();

protected:
	// those values have to be specified in 'preferences.ini' -> hardcode them
	enum ELogMode {
		None		= 0,
		OneSample	= 1,
		AllSamples	= 2
	} m_eMode;
	// those values have to be specified in 'preferences.ini' -> hardcode them
	enum ELogFileFormat {
		CSV			= 0,
		MRTG		= 1
	} m_eFileFormat;
	DWORD m_dwInterval;
	bool m_bInitialized;
	CString m_strFilePath;
	CString m_strMRTGDataFilePath;
	CString m_strMRTGOverheadFilePath;
	DWORD m_dwLastSampled;
	uint64 m_nLastSessionSentBytes;
	uint64 m_nLastSessionRecvBytes;
	uint64 m_nLastDnOH;
	uint64 m_nLastUpOH;

	void WriteSamples(UINT nCurDn, UINT nCurUp, UINT nCurDnOH, UINT uCurUpOH);
};

extern CPerfLog thePerfLog;
