#include "stdafx.h"
#include "Ini2.h"
#include "StringConversion.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CIni::AddModulPath(CString &rstrFileName, bool bModulPath)
{
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];

	_tsplitpath(rstrFileName, drive, dir, fname, ext);
	if (!drive[0])
	{
		//PathCanonicalize(..) doesn't work with for all Plattforms !
		CString strModule;
		if (bModulPath)
		{
			DWORD dwModPathLen = GetModuleFileName(NULL, strModule.GetBuffer(MAX_PATH), MAX_PATH);
			strModule.ReleaseBuffer((dwModPathLen == 0 || dwModPathLen == MAX_PATH) ? 0 : -1);
		}
		else
		{
			DWORD dwCurDirLen = GetCurrentDirectory(MAX_PATH, strModule.GetBuffer(MAX_PATH));
			strModule.ReleaseBuffer((dwCurDirLen == 0 || dwCurDirLen >= MAX_PATH) ? 0 : -1);
			// fix by "cpp@world-online.no"
			strModule.TrimRight(_T('\\'));
			strModule.TrimRight(_T('/'));
			strModule += _T("\\");
		}
		_tsplitpath(strModule, drive, dir, fname, ext);
		strModule = drive;
		strModule += dir;
		strModule += rstrFileName;
		rstrFileName = strModule;
	}
}

CString CIni::GetDefaultSection()
{
	return AfxGetAppName();
}

CString CIni::GetDefaultIniFile(bool bModulPath)
{
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	CString strTemp;
	CString strApplName;
	DWORD dwModPathLen = GetModuleFileName(NULL, strTemp.GetBuffer(MAX_PATH), MAX_PATH);
	strTemp.ReleaseBuffer((dwModPathLen == 0 || dwModPathLen == MAX_PATH) ? 0 : -1);
	_tsplitpath( strTemp, drive, dir, fname, ext );
	strTemp = fname;
	strTemp += _T(".ini");
	if (bModulPath)
	{
		strApplName = drive;
		strApplName += dir;
		strApplName += strTemp;
	}
	else
	{
		DWORD dwCurDirLen = GetCurrentDirectory(MAX_PATH, strApplName.GetBuffer(MAX_PATH));
		strApplName.ReleaseBuffer((dwCurDirLen == 0 || dwCurDirLen >= MAX_PATH) ? 0 : -1);
		strApplName.TrimRight(_T('\\'));
		strApplName.TrimRight(_T('/'));
		strApplName += _T("\\");
		strApplName += strTemp;
	}
	return strApplName;
}

CIni::CIni():
	m_bModulPath(true)
{
	m_strFileName = GetDefaultIniFile(m_bModulPath);
	m_strSection  = GetDefaultSection();
}

CIni::CIni(CIni const &Ini):
	m_strFileName(Ini.m_strFileName),
	m_strSection(Ini.m_strSection),
	m_bModulPath(Ini.m_bModulPath)
{
	if (m_strFileName.IsEmpty())
		m_strFileName = GetDefaultIniFile(m_bModulPath);
	AddModulPath(m_strFileName, m_bModulPath);
	if (m_strSection.IsEmpty())
		m_strSection = GetDefaultSection();
}

CIni::CIni(CString const &rstrFileName):
	m_strFileName(rstrFileName),
	m_bModulPath(true)
{
	if (m_strFileName.IsEmpty())
		m_strFileName = GetDefaultIniFile(m_bModulPath);
	AddModulPath(m_strFileName, m_bModulPath);
	m_strSection = GetDefaultSection();
}

CIni::CIni(CString const &rstrFileName, CString const &rstrSection):
	m_strFileName(rstrFileName),
	m_strSection(rstrSection),
	m_bModulPath(true)
{
	if (m_strFileName.IsEmpty())
		m_strFileName = GetDefaultIniFile(m_bModulPath);
	AddModulPath(m_strFileName, m_bModulPath);
	if (m_strSection.IsEmpty())
		m_strSection = GetDefaultSection();
}

CIni::~CIni()
{
}

void CIni::SetFileName(const CString &rstrFileName)
{
	m_strFileName = rstrFileName;
	AddModulPath(m_strFileName);
}

void CIni::SetSection(const CString &rstrSection)
{
	m_strSection = rstrSection;
}

const CString& CIni::GetFileName() const
{
	return m_strFileName;
}

const CString& CIni::GetSection() const
{
	return m_strSection;
}

void CIni::Init(LPCTSTR lpszFileName, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;
	if (lpszFileName != NULL)
		m_strFileName = lpszFileName;
}

CString CIni::GetString(LPCTSTR lpszEntry, LPCTSTR lpszDefault, LPCTSTR lpszSection)
{
	if (lpszDefault == NULL)
		return GetLPCSTR(lpszEntry, lpszSection, _T(""));
	else
		return GetLPCSTR(lpszEntry, lpszSection, lpszDefault);
}

CString CIni::GetStringLong(LPCTSTR lpszEntry, LPCTSTR lpszDefault, LPCTSTR lpszSection)
{
	CString ret;
	unsigned int maxstrlen = MAX_INI_BUFFER;

	if (lpszSection != NULL)
		m_strSection = lpszSection;

	do {
		GetPrivateProfileString(m_strSection, lpszEntry, (lpszDefault == NULL) ? _T("") : lpszDefault, 
			ret.GetBufferSetLength(maxstrlen), maxstrlen, m_strFileName);
		ret.ReleaseBuffer();
		if ((unsigned int)ret.GetLength() < maxstrlen - 2)
			break;
		maxstrlen += MAX_INI_BUFFER;
	}
	while (maxstrlen < 32767);

	return ret;
}

CString CIni::GetStringUTF8(LPCTSTR lpszEntry, LPCTSTR lpszDefault, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;

	CStringA strUTF8;
	GetPrivateProfileStringA(CT2CA(m_strSection), CT2CA(lpszEntry), CT2CA(lpszDefault),
							 strUTF8.GetBufferSetLength(MAX_INI_BUFFER), MAX_INI_BUFFER, CT2CA(m_strFileName));
	strUTF8.ReleaseBuffer();
	return OptUtf8ToStr(strUTF8);
}

double CIni::GetDouble(LPCTSTR lpszEntry, double fDefault, LPCTSTR lpszSection)
{
	TCHAR szDefault[MAX_PATH];
	_sntprintf(szDefault, _countof(szDefault), _T("%g"), fDefault);
	szDefault[_countof(szDefault) - 1] = _T('\0');
	GetLPCSTR(lpszEntry, lpszSection, szDefault);
	return _tstof(m_chBuffer);
}

float CIni::GetFloat(LPCTSTR lpszEntry, float fDefault, LPCTSTR lpszSection)
{
	TCHAR szDefault[MAX_PATH];
	_sntprintf(szDefault, _countof(szDefault), _T("%g"), fDefault);
	szDefault[_countof(szDefault) - 1] = _T('\0');
	GetLPCSTR(lpszEntry, lpszSection, szDefault);
	return (float)_tstof(m_chBuffer);
}

int CIni::GetInt(LPCTSTR lpszEntry, int nDefault, LPCTSTR lpszSection)
{
	TCHAR szDefault[MAX_PATH];
	_sntprintf(szDefault, _countof(szDefault), _T("%d"), nDefault);
	szDefault[_countof(szDefault) - 1] = _T('\0');
	GetLPCSTR(lpszEntry, lpszSection, szDefault);
	return _tstoi(m_chBuffer);
}

ULONGLONG CIni::GetUInt64(LPCTSTR lpszEntry, ULONGLONG nDefault, LPCTSTR lpszSection)
{
	TCHAR szDefault[MAX_PATH];
	_sntprintf(szDefault, _countof(szDefault), _T("%I64u"), nDefault);
	szDefault[_countof(szDefault) - 1] = _T('\0');
	GetLPCSTR(lpszEntry, lpszSection, szDefault);
	ULONGLONG nResult;
	if (_stscanf(m_chBuffer, _T("%I64u"), &nResult) != 1)
		return nDefault;
	return nResult;
}

WORD CIni::GetWORD(LPCTSTR lpszEntry, WORD nDefault, LPCTSTR lpszSection)
{
	TCHAR szDefault[MAX_PATH];
	_sntprintf(szDefault, _countof(szDefault), _T("%u"), nDefault);
	szDefault[_countof(szDefault) - 1] = _T('\0');
	GetLPCSTR(lpszEntry, lpszSection, szDefault);
	return (WORD)_tstoi(m_chBuffer);
}

bool CIni::GetBool(LPCTSTR lpszEntry, bool bDefault, LPCTSTR lpszSection)
{
	TCHAR szDefault[MAX_PATH];
	_sntprintf(szDefault, _countof(szDefault), _T("%d"), bDefault);
	szDefault[_countof(szDefault) - 1] = _T('\0');
	GetLPCSTR(lpszEntry, lpszSection, szDefault);
	return _tstoi(m_chBuffer) != 0;
}

CPoint CIni::GetPoint(LPCTSTR lpszEntry, CPoint ptDefault, LPCTSTR lpszSection)
{
	CPoint ptReturn = ptDefault;

	CString strDefault;
	strDefault.Format(_T("(%d,%d)"), ptDefault.x, ptDefault.y);

	CString strPoint = GetString(lpszEntry, strDefault, lpszSection);
	if (_stscanf(strPoint,_T("(%d,%d)"), &ptReturn.x, &ptReturn.y) != 2)
		return ptDefault;

	return ptReturn;
}

CRect CIni::GetRect(LPCTSTR lpszEntry, CRect rectDefault, LPCTSTR lpszSection)
{
	CRect rectReturn = rectDefault;

	CString strDefault;
	strDefault.Format(_T("%d,%d,%d,%d"), rectDefault.left, rectDefault.top, rectDefault.right, rectDefault.bottom);

	CString strRect = GetString(lpszEntry, strDefault, lpszSection);

	//new Version found
	if (_stscanf(strRect, _T("%d,%d,%d,%d"), &rectDefault.left, &rectDefault.top, &rectDefault.right, &rectDefault.bottom) == 4)
		return rectReturn;
	//old Version found
	if (_stscanf(strRect, _T("(%d,%d,%d,%d)"), &rectReturn.top, &rectReturn.left, &rectReturn.bottom, &rectReturn.right) != 4)
		return rectDefault;
	return rectReturn;
}

COLORREF CIni::GetColRef(LPCTSTR lpszEntry, COLORREF crDefault, LPCTSTR lpszSection)
{
	int temp[3] = {	GetRValue(crDefault),
					GetGValue(crDefault),
					GetBValue(crDefault) };

	CString strDefault;
	strDefault.Format(_T("RGB(%hd,%hd,%hd)"), temp[0], temp[1], temp[2]);

	CString strColRef = GetString(lpszEntry, strDefault, lpszSection);
	if (_stscanf(strColRef, _T("RGB(%d,%d,%d)"), temp, temp+1, temp+2) != 3)
		return crDefault;

	return RGB(temp[0], temp[1], temp[2]);
}
	
void CIni::WriteString(LPCTSTR lpszEntry, LPCTSTR lpsz, LPCTSTR lpszSection)
{
	if (lpszSection != NULL) 
		m_strSection = lpszSection;
	WritePrivateProfileString(m_strSection, lpszEntry, lpsz, m_strFileName);
}

void CIni::WriteStringUTF8(LPCTSTR lpszEntry, LPCTSTR lpsz, LPCTSTR lpszSection)
{
	if (lpszSection != NULL) 
		m_strSection = lpszSection;
	CString str(lpsz);
	WritePrivateProfileStringA(CT2CA(m_strSection), CT2CA(lpszEntry), StrToUtf8(str), CT2CA(m_strFileName));
}

void CIni::WriteDouble(LPCTSTR lpszEntry, double f, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;
	TCHAR szBuffer[MAX_PATH];
	_sntprintf(szBuffer, _countof(szBuffer), _T("%g"), f);
	szBuffer[_countof(szBuffer) - 1] = _T('\0');
	WritePrivateProfileString(m_strSection, lpszEntry, szBuffer, m_strFileName);
}

void CIni::WriteFloat(LPCTSTR lpszEntry, float f, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;
	TCHAR szBuffer[MAX_PATH];
	_sntprintf(szBuffer, _countof(szBuffer), _T("%g"), f);
	szBuffer[_countof(szBuffer) - 1] = _T('\0');
	WritePrivateProfileString(m_strSection, lpszEntry, szBuffer, m_strFileName);
}

void CIni::WriteInt(LPCTSTR lpszEntry, int n, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;
	TCHAR szBuffer[MAX_PATH];
	_itot(n, szBuffer, 10);
	WritePrivateProfileString(m_strSection, lpszEntry, szBuffer, m_strFileName);
}

void CIni::WriteUInt64(LPCTSTR lpszEntry, ULONGLONG n, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;
	TCHAR szBuffer[MAX_PATH];
	_ui64tot(n, szBuffer, 10);
	WritePrivateProfileString(m_strSection, lpszEntry, szBuffer, m_strFileName);
}

void CIni::WriteWORD(LPCTSTR lpszEntry, WORD n, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;
	TCHAR szBuffer[MAX_PATH];
	_ultot(n, szBuffer, 10);
	WritePrivateProfileString(m_strSection, lpszEntry, szBuffer, m_strFileName);
}

void CIni::WriteBool(LPCTSTR lpszEntry, bool b, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;
	TCHAR szBuffer[MAX_PATH];
	_sntprintf(szBuffer, _countof(szBuffer), _T("%d"), (int)b);
	szBuffer[_countof(szBuffer) - 1] = _T('\0');
	WritePrivateProfileString(m_strSection, lpszEntry, szBuffer, m_strFileName);
}

void CIni::WritePoint(LPCTSTR lpszEntry, CPoint pt, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;
	CString strBuffer;
	strBuffer.Format(_T("(%d,%d)"), pt.x, pt.y);
	Write(m_strFileName, m_strSection, lpszEntry, strBuffer);
}

void CIni::WriteRect(LPCTSTR lpszEntry, CRect rect, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;
	CString strBuffer;
	strBuffer.Format(_T("(%d,%d,%d,%d)"), rect.top, rect.left, rect.bottom, rect.right);
	Write(m_strFileName, m_strSection, lpszEntry, strBuffer);
}

void CIni::WriteColRef(LPCTSTR lpszEntry, COLORREF cr, LPCTSTR lpszSection)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;
	CString strBuffer;
	strBuffer.Format(_T("RGB(%d,%d,%d)"), GetRValue(cr), GetGValue(cr), GetBValue(cr));
	Write(m_strFileName, m_strSection, lpszEntry, strBuffer);
}

TCHAR* CIni::GetLPCSTR(LPCTSTR lpszEntry, LPCTSTR lpszSection, LPCTSTR lpszDefault)
{
	if (lpszSection != NULL)
		m_strSection = lpszSection;

	CString strTemp;
	if (lpszDefault == NULL)
		strTemp = Read(m_strFileName, m_strSection, lpszEntry, CString());
	else
		strTemp = Read(m_strFileName, m_strSection, lpszEntry, lpszDefault);

	return (TCHAR *)memcpy(m_chBuffer, (LPCTSTR)strTemp, (strTemp.GetLength() + 1)*sizeof(TCHAR));
}

void CIni::SerGetString(bool bGet, CString &rstr, LPCTSTR lpszEntry, LPCTSTR lpszSection, LPCTSTR lpszDefault)
{
	if (bGet)
		rstr = GetString(lpszEntry, lpszDefault, lpszSection);
	else
		WriteString(lpszEntry, rstr, lpszSection);
}

void CIni::SerGetDouble(bool bGet, double &f, LPCTSTR lpszEntry, LPCTSTR lpszSection, double fDefault)
{
	if (bGet)
		f = GetDouble(lpszEntry, fDefault, lpszSection);
	else
		WriteDouble(lpszEntry, f, lpszSection);
}

void CIni::SerGetFloat(bool bGet, float &f, LPCTSTR lpszEntry, LPCTSTR lpszSection, float fDefault)
{
	if (bGet)
		f = GetFloat(lpszEntry, fDefault, lpszSection);
	else
		WriteFloat(lpszEntry, f, lpszSection);
}

void CIni::SerGetInt(bool bGet, int &n, LPCTSTR lpszEntry, LPCTSTR lpszSection, int nDefault)
{
	if (bGet)
		n = GetInt(lpszEntry, nDefault, lpszSection);
	else
		WriteInt(lpszEntry, n, lpszSection);
}

void CIni::SerGetDWORD(bool bGet, DWORD &n,	LPCTSTR lpszEntry, LPCTSTR lpszSection, DWORD nDefault)
{
	if (bGet)
		n = (DWORD)GetInt(lpszEntry, nDefault, lpszSection);
	else
		WriteInt(lpszEntry, n, lpszSection);
}

void CIni::SerGetBool(bool bGet, bool &b, LPCTSTR lpszEntry, LPCTSTR lpszSection, bool bDefault)
{
	if (bGet)
		b = GetBool(lpszEntry, bDefault, lpszSection);
	else
		WriteBool(lpszEntry, b, lpszSection);
}

void CIni::SerGetPoint(bool bGet, CPoint &pt, LPCTSTR lpszEntry, LPCTSTR lpszSection, CPoint ptDefault)
{
	if (bGet)
		pt = GetPoint(lpszEntry, ptDefault, lpszSection);
	else
		WritePoint(lpszEntry, pt, lpszSection);
}

void CIni::SerGetRect(bool bGet, CRect & rect, LPCTSTR lpszEntry, LPCTSTR lpszSection, CRect rectDefault)
{
	if (bGet)
		rect = GetRect(lpszEntry, rectDefault, lpszSection);
	else
		WriteRect(lpszEntry, rect, lpszSection);
}

void CIni::SerGetColRef(bool bGet, COLORREF &cr, LPCTSTR lpszEntry, LPCTSTR lpszSection, COLORREF crDefault)
{
	if (bGet)
		cr = GetColRef(lpszEntry, crDefault, lpszSection);
	else
		WriteColRef(lpszEntry, cr, lpszSection);
}

void CIni::SerGet(bool bGet, CString &rstr, LPCTSTR lpszEntry, LPCTSTR lpszSection, LPCTSTR lpszDefault)
{
	SerGetString(bGet, rstr, lpszEntry, lpszSection, lpszDefault);
}

void CIni::SerGet(bool bGet, double &f, LPCTSTR lpszEntry, LPCTSTR lpszSection, double fDefault)
{
	SerGetDouble(bGet, f, lpszEntry, lpszSection, fDefault);
}

void CIni::SerGet(bool bGet, float &f, LPCTSTR lpszEntry, LPCTSTR lpszSection, float fDefault)
{
	SerGetFloat(bGet, f, lpszEntry, lpszSection, fDefault);
}

void CIni::SerGet(bool bGet, int &n, LPCTSTR lpszEntry, LPCTSTR lpszSection, int nDefault)
{
	SerGetInt(bGet, n, lpszEntry, lpszSection, nDefault);
}

void CIni::SerGet(bool bGet, short &n, LPCTSTR lpszEntry, LPCTSTR lpszSection, int nDefault)
{
	int nTemp = n;
	SerGetInt(bGet, nTemp, lpszEntry, lpszSection, nDefault);
	n = (short)nTemp;
}

void CIni::SerGet(bool bGet, DWORD &n, LPCTSTR lpszEntry, LPCTSTR lpszSection, DWORD nDefault)
{
	SerGetDWORD(bGet, n, lpszEntry, lpszSection, nDefault);
}

void CIni::SerGet(bool bGet, WORD &n, LPCTSTR lpszEntry, LPCTSTR lpszSection, DWORD nDefault)
{
	DWORD dwTemp = n;
	SerGetDWORD(bGet, dwTemp, lpszEntry, lpszSection, nDefault);
	n = (WORD)dwTemp;
}

void CIni::SerGet(bool bGet, CPoint &pt, LPCTSTR lpszEntry, LPCTSTR lpszSection, CPoint ptDefault)
{
	SerGetPoint(bGet, pt, lpszEntry, lpszSection, ptDefault);
}

void CIni::SerGet(bool bGet, CRect &rect, LPCTSTR lpszEntry, LPCTSTR lpszSection, CRect rectDefault)
{
	SerGetRect(bGet, rect, lpszEntry, lpszSection, rectDefault);
}

void CIni::SerGet(bool bGet, CString *ar, int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection, LPCTSTR lpszDefault)
{
	if (nCount > 0) {
		CString strBuffer;
		if (bGet) {
			strBuffer = GetString(lpszEntry, _T(""), lpszSection);
			int nOffset = 0;
			for (int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, ar[i]);
				if (ar[i].GetLength() == 0)
					ar[i] = lpszDefault;
			}
		} else {
			strBuffer = ar[0];
			for (int i = 1; i < nCount; i++) {
				strBuffer.AppendChar(_T(','));
				strBuffer.Append(ar[i]);
			}
			WriteString(lpszEntry, strBuffer, lpszSection);
		}
	}
}

void CIni::SerGet(bool bGet, double *ar, int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection, double fDefault)
{
	if (nCount > 0) {
		CString strBuffer;
		if (bGet) {
			strBuffer = GetString(lpszEntry, _T(""), lpszSection);
			CString strTemp;
			int nOffset = 0;
			for (int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if (strTemp.GetLength() == 0)
					ar[i] = fDefault;
				else
					ar[i] = _tstof(strTemp);
			}
		} else {
			CString strTemp;
			strBuffer.Format(_T("%g"), ar[0]);
			for (int i = 1; i < nCount; i++) {
				strTemp.Format(_T("%g"), ar[i]);
				strBuffer.AppendChar(_T(','));
				strBuffer.Append(strTemp);
			}
			WriteString(lpszEntry, strBuffer, lpszSection);
		}
	}
}

void CIni::SerGet(bool bGet, float *ar, int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection, float fDefault)
{
	if (nCount > 0) {
		CString strBuffer;
		if (bGet) {
			strBuffer = GetString(lpszEntry, _T(""), lpszSection);
			CString strTemp;
			int nOffset = 0;
			for (int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if (strTemp.GetLength() == 0)
					ar[i] = fDefault;
				else
					ar[i] = (float)_tstof(strTemp);
			}
		} else {
			CString strTemp;
			strBuffer.Format(_T("%g"), ar[0]);
			for (int i = 1; i < nCount; i++) {
				strTemp.Format(_T("%g"), ar[i]);
				strBuffer.AppendChar(_T(','));
				strBuffer.Append(strTemp);
			}
			WriteString(lpszEntry, strBuffer, lpszSection);
		}
	}
}

void CIni::SerGet(bool bGet, int *ar, int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection, int iDefault)
{
	if (nCount > 0) {
		CString strBuffer;
		if (bGet) {
			strBuffer = GetString(lpszEntry, _T(""), lpszSection);
			CString strTemp;
			int nOffset = 0;
			for (int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if (strTemp.GetLength() == 0)
					ar[i] = iDefault;
				else
					ar[i] = _tstoi(strTemp);
			}
		} else {
			CString strTemp;
			strBuffer.Format(_T("%d"), ar[0]);
			for (int i = 1; i < nCount; i++) {
				strTemp.Format(_T("%d"), ar[i]);
				strBuffer.AppendChar(_T(','));
				strBuffer.Append(strTemp);
			}
			WriteString(lpszEntry, strBuffer, lpszSection);
		}
	}
}

void CIni::SerGet(bool bGet, unsigned char *ar, int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection, unsigned char ucDefault)
{
	if (nCount > 0) {
		CString strBuffer;
		if (bGet) {
			strBuffer = GetString(lpszEntry, _T(""), lpszSection);
			CString strTemp;
			int nOffset = 0;
			for (int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if (strTemp.GetLength() == 0)
					ar[i] = ucDefault;
				else
					ar[i] = (unsigned char)_tstoi(strTemp);
			}
		} else {
			CString strTemp;
			strBuffer.Format(_T("%d"), ar[0]);
			for (int i = 1; i < nCount; i++) {
				strTemp.Format(_T("%d"), ar[i]);
				strBuffer.AppendChar(_T(','));
				strBuffer.Append(strTemp);
			}
			WriteString(lpszEntry, strBuffer, lpszSection);
		}
	}
}

void CIni::SerGet(bool bGet, short *ar, int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection, int iDefault)
{
	if (nCount > 0) {
		CString strBuffer;
		if (bGet) {
			strBuffer = GetString(lpszEntry, _T(""), lpszSection);
			CString strTemp;
			int nOffset = 0;
			for (int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if (strTemp.GetLength() == 0)
					ar[i] = (short)iDefault;
				else
					ar[i] = (short)_tstoi(strTemp);
			}
		} else {
			CString strTemp;
			strBuffer.Format(_T("%d"), ar[0]);
			for (int i = 1; i < nCount; i++) {
				strTemp.Format(_T("%d"), ar[i]);
				strBuffer.AppendChar(_T(','));
				strBuffer.Append(strTemp);
			}
			WriteString(lpszEntry, strBuffer, lpszSection);
		}
	}
}

void CIni::SerGet(bool bGet, DWORD *ar, int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection, DWORD dwDefault)
{
	if (nCount > 0) {
		CString strBuffer;
		if (bGet) {
			strBuffer = GetString(lpszEntry, _T(""), lpszSection);
			CString strTemp;
			int nOffset = 0;
			for (int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.GetLength() == 0)
					ar[i] = dwDefault;
				else
					ar[i] = (DWORD)_tstoi(strTemp);
			}
		} else {
			CString strTemp;
			strBuffer.Format(_T("%d"), ar[0]);
			for (int i = 1; i < nCount; i++) {
				strTemp.Format(_T("%d"), ar[i]);
				strBuffer.AppendChar(_T(','));
				strBuffer.Append(strTemp);
			}
			WriteString(lpszEntry, strBuffer, lpszSection);
		}
	}
}

void CIni::SerGet(bool bGet, WORD *ar, int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection, DWORD dwDefault)
{
	if (nCount > 0) {
		CString strBuffer;
		if (bGet) {
			strBuffer = GetString(lpszEntry, _T(""), lpszSection);
			CString strTemp;
			int nOffset = 0;
			for (int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if (strTemp.GetLength() == 0)
					ar[i] = (WORD)dwDefault;
				else
					ar[i] = (WORD)_tstoi(strTemp);
			}
		} else {
			CString strTemp;
			strBuffer.Format(_T("%d"), ar[0]);
			for (int i = 1; i < nCount; i++) {
				strTemp.Format(_T("%d"), ar[i]);
				strBuffer.AppendChar(_T(','));
				strBuffer.Append(strTemp);
			}
			WriteString(lpszEntry, strBuffer, lpszSection);
		}
	}
}

void CIni::SerGet(bool bGet, CPoint * ar, int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection, CPoint ptDefault)
{
	CString strBuffer;
	for (int i = 0; i < nCount; i++)
	{
		strBuffer.Format(_T("_%i"), i);
		strBuffer = lpszEntry + strBuffer;
		SerGet(bGet, ar[i], strBuffer, lpszSection, ptDefault);
	}
}

void CIni::SerGet(bool bGet, CRect *ar, int nCount, LPCTSTR lpszEntry, LPCTSTR lpszSection, CRect rcDefault)
{
	CString strBuffer;
	for (int i = 0; i < nCount; i++)
	{
		strBuffer.Format(_T("_%i"), i);
		strBuffer = lpszEntry + strBuffer;
		SerGet(bGet, ar[i], strBuffer, lpszSection, rcDefault);
	}
}

int CIni::Parse(const CString &strIn, int nOffset, CString &strOut)
{
	strOut.Empty();
	int nLength = strIn.GetLength();

	if (nOffset < nLength) {
		if (nOffset != 0 && strIn[nOffset] == _T(','))
			nOffset++;

		while (nOffset < nLength) {
			if (!_istspace((_TUCHAR)strIn[nOffset]))
				break;
			nOffset++;
		}

		while (nOffset < nLength) {
			strOut += strIn[nOffset];
			if (strIn[++nOffset] == _T(','))
				break;
		}
		strOut.Trim();
	}
	return nOffset;
}

CString CIni::Read(LPCTSTR lpszFileName, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault)
{
	CString strReturn;
	GetPrivateProfileString(lpszSection,
							lpszEntry,
							lpszDefault,
							strReturn.GetBufferSetLength(MAX_INI_BUFFER),
							MAX_INI_BUFFER,
							lpszFileName);
	strReturn.ReleaseBuffer();
	return strReturn;
}

void CIni::Write(LPCTSTR lpszFileName, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue)
{
	WritePrivateProfileString(lpszSection,
							  lpszEntry,
							  lpszValue,
							  lpszFileName);
}

bool CIni::GetBinary(LPCTSTR lpszEntry, BYTE** ppData, UINT* pBytes, LPCTSTR pszSection)
{
	*ppData = NULL;
	*pBytes = 0;

	CString str = GetString(lpszEntry, NULL, pszSection);
	if (str.IsEmpty())
		return false;
	ASSERT(str.GetLength()%2 == 0);
	INT_PTR nLen = str.GetLength();
	*pBytes = UINT(nLen)/2;
	*ppData = new BYTE[*pBytes];
	for (int i=0;i<nLen;i+=2)
	{
		(*ppData)[i/2] = (BYTE)(((str[i+1] - 'A') << 4) + (str[i] - 'A'));
	}
	return true;
}

bool CIni::WriteBinary(LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes, LPCTSTR pszSection)
{
	// convert to string and write out
	LPTSTR lpsz = new TCHAR[nBytes*2+1];
	UINT i;
	for (i = 0; i < nBytes; i++)
	{
		lpsz[i*2] = (TCHAR)((pData[i] & 0x0F) + 'A'); //low nibble
		lpsz[i*2+1] = (TCHAR)(((pData[i] >> 4) & 0x0F) + 'A'); //high nibble
	}
	lpsz[i*2] = 0;


	WriteString(lpszEntry, lpsz, pszSection);
	delete[] lpsz;
	return true;
}

void CIni::DeleteKey(LPCTSTR pszKey)
{
	WritePrivateProfileString(m_strSection, pszKey, NULL, m_strFileName);
}
