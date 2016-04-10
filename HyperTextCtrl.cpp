/********************************************************************
		HyperTextCtrl.h - Controls that shows hyperlinks 
		in text

		Copyright (C) 2001-2002 Magomed G. Abdurakhmanov			
********************************************************************/

//edited by (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//-> converted it to MFC
//-> included colored keywords
//-> fixed GPF bugs
//-> made it flickerfree
//-> some other small changes
// (the whole code still needs some work though)
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "emule.h"
#include "emuledlg.h"
#include "hypertextctrl.h"
#include <deque>

#pragma warning(disable:4244) // conversion from <type1> to <type2>, possible loss of data
#pragma warning(disable:4018) // signed/unsigned mismatch
#pragma warning(disable:4100) // unreferenced formal parameter

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CHyperLink
CHyperLink::CHyperLink(int iBegin, uint16 iEnd, const CString& sTitle, const CString& sCommand, const CString& sDirectory){
	m_Type = lt_Shell;
	m_iBegin = iBegin;
	m_iEnd = iEnd;
	m_sTitle = sTitle;
	m_sCommand = sCommand;
	m_sDirectory = sDirectory;
   // [i_a] used for lt_Message  
    m_hWnd = 0; 
    m_uMsg = 0; 
    m_wParam = 0; 
    m_lParam = 0; 
} // [/i_a] 

CHyperLink::CHyperLink(int iBegin, uint16 iEnd, const CString& sTitle, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	m_Type = lt_Message;
	m_iBegin = iBegin;
	m_iEnd = iEnd;
	m_sTitle = sTitle;
	m_hWnd = hWnd;
	m_uMsg = uMsg;
	m_wParam = wParam;
	m_lParam = lParam;
}
 CHyperLink::CHyperLink(){  // [i_a] 
   m_Type = lt_Unknown; 
   m_iBegin = 0; 
   m_iEnd = 0; 
   m_sTitle.Empty(); 
   m_sCommand.Empty(); 
   m_sDirectory.Empty(); 
   m_hWnd = 0; 
   m_uMsg = 0; 
   m_wParam = 0; 
   m_lParam = 0; 
 } // [/i_a] 
CHyperLink::CHyperLink(const CHyperLink& Src){
	m_Type = Src.m_Type;
	m_iBegin = Src.m_iBegin;
	m_iEnd = Src.m_iEnd;
	m_sTitle = Src.m_sTitle;
	m_sCommand = Src.m_sCommand;
	m_sDirectory = Src.m_sDirectory;
	m_hWnd = Src.m_hWnd;
	m_uMsg = Src.m_uMsg;
	m_wParam = Src.m_wParam;
	m_lParam = Src.m_lParam;
}

void CHyperLink::Execute(){
	switch(m_Type)
	{
	case lt_Shell:
		ShellExecute(NULL, NULL, m_sCommand, NULL, m_sDirectory, SW_SHOWDEFAULT);
		break;

	case lt_Message:
		PostMessage(m_hWnd, m_uMsg, m_wParam, m_lParam);
		break;
	}
}

// CKeyWord
CKeyWord::CKeyWord(int iBegin, uint16 iEnd, COLORREF icolor){
	color = icolor;
	m_iBegin = iBegin;
	m_iEnd = iEnd;
}

// CPreparedHyperText
void CPreparedHyperText::PrepareText(const CString& sText)
{
	m_sText = sText;
	m_Links.clear();

	enum {
		unknown,
		space,
		http0,		/* http://		*/
		http1, http2, http3, http4, http5, http6,
		ftp0,		/* ftp://		*/
		ftp1, ftp2, ftp3, ftp4, ftp5,
		ftp,		/* ftp.			*/
		www0,		/* www.			*/
		www1, www2, www3,
		mailto0, 	/* mailto:		*/
		mailto1, mailto2, mailto3, mailto4, mailto5, mailto6,
		mail,		/* xxx@yyy		*/
		ed2k0,		/* ed2k://		*/
		ed2k1, ed2k2, ed2k3, ed2k4, ed2k5, ed2k6
	} state = space;

	int WordPos = 0;
	TCHAR sz[2];
	TCHAR& c = sz[0];
	sz[1] = 0;
	int last = m_sText.GetLength() -1;
	for(int i = 0; i <= last; i++)
	{
		c = m_sText[i];
		_tcslwr(sz);

		switch(state)
		{
		case unknown:
			if(tspace(c))
				state = space;
			else
				if(c == _T('@') && WordPos != i)
					state = mail;		
			break;

		case space:
			WordPos = i;
			switch(c)
			{
			case _T('h'): state = http0; break;
			case _T('f'): state = ftp0; break;
			case _T('w'): state = www0; break;
			case _T('m'): state = mailto0; break;
			case _T('e'): state = ed2k0; break;
			default:
				if(!tspace(c))
					state = unknown;
			}
			break;

			/*----------------- http -----------------*/
		case http0:
			if(c == _T('t'))
				state = http1;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http1:
			if(c == _T('t'))
				state = http2;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http2:
			if(c == _T('p'))
				state = http3;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http3:
			if(c == _T(':'))
				state = http4;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http4:
			if(c == _T('/'))
				state = http5;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http5:
			if(c == _T('/'))
				state = http6;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http6:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, (LPCTSTR)NULL));
				state = space;
			}
			break;

			/*----------------- ed2k -----------------*/
		case ed2k0:
			if(c == _T('d'))
				state = ed2k1;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k1:
			if(c == _T('2'))
				state = ed2k2;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k2:
			if(c == _T('k'))
				state = ed2k3;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k3:
			if(c == _T(':'))
				state = ed2k4;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k4:
			if(c == _T('/'))
				state = ed2k5;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k5:
			if(c == _T('/'))
				state = ed2k6;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k6:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, (LPCTSTR)NULL));
				state = space;
			}
			break;
			/*----------------- ftp -----------------*/
		case ftp0:
			if(c == _T('t'))
				state = ftp1;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ftp1:
			if(c == _T('p'))
				state = ftp2;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ftp2:
			if(c == _T(':'))
				state = ftp3;
			else
				if(c == _T('.'))
					state = ftp;
				else
					if(tspace(c))
						state = space;
					else
						state = unknown;
			break;

		case ftp3:
			if(c == _T('/'))
				state = ftp4;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ftp4:
			if(c == _T('/'))
				state = ftp5;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ftp:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = CString(_T("ftp://")) + m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, (LPCTSTR)NULL));
				state = space;
			}
			break;

		case ftp5:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, (LPCTSTR)NULL));
				state = space;
			}
			break;

			/*----------------- www -----------------*/
		case www0:
			if(c == _T('w'))
				state = www1;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case www1:
			if(c == _T('w'))
				state = www2;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case www2:
			if(c == _T('.'))
				state = www3;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case www3:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = CString(_T("http://")) + m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, (LPCTSTR)NULL));
				state = space;
			}
			break;

			/*----------------- mailto -----------------*/
		case mailto0:
			if(c == _T('a'))
				state = mailto1;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto1:
			if(c == _T('i'))
				state = mailto2;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto2:
			if(c == _T('l'))
				state = mailto3;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto3:
			if(c == _T('t'))
				state = mailto4;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto4:
			if(c == _T('o'))
				state = mailto5;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto5:
			if(c == _T(':'))
				state = mailto6;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto6:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, (LPCTSTR)NULL));
				state = space;
			}
			break;

			/*----------------- mailto -----------------*/
		case mail:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = CString(_T("mailto:")) + m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, (LPCTSTR)NULL));
				state = space;
			}
			break;
		}
	}

	m_Links.sort();
}

 void CPreparedHyperText::RemoveLastSign(CString& sLink)
{
	int len = sLink.GetLength();
	if(len > 0)
	{
		TCHAR c = sLink[len-1];
		switch(c)
		{
		case _T('.'):
		case _T(','):
		case _T(';'):
		case _T('\"'):
		case _T('\''):
		case _T('('):
		case _T(')'):
		case _T('['):
		case _T(']'):
		case _T('{'):
		case _T('}'):
			sLink.Delete(len -1, 1);
			break;
		}
	}
}

CPreparedHyperText::CPreparedHyperText(const CString& sText){
	PrepareText(sText);
}

CPreparedHyperText::CPreparedHyperText(const CPreparedHyperText& src){
	m_sText = src.m_sText;
	m_Links.assign(src.m_Links.begin(), src.m_Links.end());
}

void CPreparedHyperText::Clear(){
	m_sText.Empty();
	m_Links.erase(m_Links.begin(), m_Links.end());
}

void CPreparedHyperText::SetText(const CString& sText){
	Clear();
	PrepareText(sText);
}

void CPreparedHyperText::AppendText(const CString& sText){
	int len = m_sText.GetLength();
	////////////////////////////////////////////////
	//Top:The Original code didn't check to see if the buffer was full..
	////////////////////////////////////////////////
	bool flag = true;
	if( len > 60000 ){
		m_sText = m_sText.Right(50000);
		int shift = len - m_sText.GetLength();
		while( flag == true ){
			CHyperLink &test = m_Links.front();
			if( !m_Links.empty() ){
				if( test.Begin() < shift )
					m_Links.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		flag = true;
		while( flag == true ){
			CKeyWord &test = m_KeyWords.front();
			if( !m_KeyWords.empty() ){
				if( test.Begin() < shift )
					m_KeyWords.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		len = m_sText.GetLength();
		CHyperLink &ltest = m_Links.front();
		int litest = ltest.Begin() - shift;
		CKeyWord &wtest = m_KeyWords.front();
		int witest = wtest.Begin() - shift;
		flag = true;
		while( flag == true && !m_Links.empty() ){
			CHyperLink &temp = m_Links.front();
			CHyperLink backup( temp);
			backup.SetBegin( backup.Begin() - shift );
			backup.SetEnd( backup.End() - shift );
			m_Links.pop_front();
			m_Links.push_back( backup );
			if( ((CHyperLink)m_Links.front()).Begin() == (UINT)litest )
				flag = false;
		}
		flag = true;
		while( flag == true && !m_KeyWords.empty() ){
			CKeyWord &temp = m_KeyWords.front();
			CKeyWord backup( temp.Begin()-shift, temp.End()-shift, temp.Color());
			m_KeyWords.pop_front();
			m_KeyWords.push_back( backup );
			if( ((CKeyWord)m_KeyWords.front()).Begin() == (UINT)witest )
				flag = false;
		}
	}
	////////////////////////////////////////////////
	//Bottom: May not be the nicest code but it works.
	////////////////////////////////////////////////
	CPreparedHyperText ht(sText);
	m_sText+=sText;
	for(std::list<CHyperLink>::iterator it = ht.m_Links.begin(); it != ht.m_Links.end(); it++)
	{
		CHyperLink hl = *it;
		hl.m_iBegin += len;
		hl.m_iEnd += len;
		m_Links.push_back(hl);
	}
}

void CPreparedHyperText::AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory){
	if (!(sText.GetLength() && sCommand.GetLength()))
		return;
	int len = m_sText.GetLength();
	////////////////////////////////////////////////
	//Top:The Original code didn't check to see if the buffer was full..
	////////////////////////////////////////////////
	bool flag = true;
	if( len > 60000 ){
		m_sText = m_sText.Right(50000);
		int shift = len - m_sText.GetLength();
		while( flag == true ){
			CHyperLink &test = m_Links.front();
			if( !m_Links.empty() ){
				if( test.Begin() < shift )
					m_Links.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		flag = true;
		while( flag == true ){
			CKeyWord &test = m_KeyWords.front();
			if( !m_KeyWords.empty() ){
				if( test.Begin() < shift )
					m_KeyWords.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		len = m_sText.GetLength();
		CHyperLink &ltest = m_Links.front();
		int litest = ltest.Begin() - shift;
		CKeyWord &wtest = m_KeyWords.front();
		int witest = wtest.Begin() - shift;
		flag = true;
		while( flag == true && !m_Links.empty() ){
			CHyperLink &temp = m_Links.front();
			CHyperLink backup( temp);
			backup.SetBegin( backup.Begin() - shift );
			backup.SetEnd( backup.End() - shift );
			m_Links.pop_front();
			m_Links.push_back( backup );
			if( ((CHyperLink)m_Links.front()).Begin() == (UINT)litest )
				flag = false;
		}
		flag = true;
		while( flag == true && !m_KeyWords.empty() ){
			CKeyWord &temp = m_KeyWords.front();
			CKeyWord backup( temp.Begin()-shift, temp.End()-shift, temp.Color());
			m_KeyWords.pop_front();
			m_KeyWords.push_back( backup );
			if( ((CKeyWord)m_KeyWords.front()).Begin() == (UINT)witest )
				flag = false;
		}
	}
	////////////////////////////////////////////////
	//Bottom: May not be the nicest code but it works.
	////////////////////////////////////////////////
	m_sText+=sText;
	m_Links.push_back(CHyperLink(len, len + sText.GetLength() - 1, sTitle, sCommand, sDirectory));
}

void CPreparedHyperText::AppendHyperLink(const CString& sText, const CString& sTitle, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	if (!sText.GetLength())
		return;
	int len = m_sText.GetLength();
	////////////////////////////////////////////////
	//Top:The Original code didn't check to see if the buffer was full..
	////////////////////////////////////////////////
	bool flag = true;
	if( len > 60000 ){
		m_sText = m_sText.Right(50000);
		int shift = len - m_sText.GetLength();
		while( flag == true ){
			CHyperLink &test = m_Links.front();
			if( !m_Links.empty() ){
				if( test.Begin() < shift )
					m_Links.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		flag = true;
		while( flag == true ){
			CKeyWord &test = m_KeyWords.front();
			if( !m_KeyWords.empty() ){
				if( test.Begin() < shift )
					m_KeyWords.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		len = m_sText.GetLength();
		CHyperLink &ltest = m_Links.front();
		int litest = ltest.Begin() - shift;
		CKeyWord &wtest = m_KeyWords.front();
		int witest = wtest.Begin() - shift;
		flag = true;
		while( flag == true && !m_Links.empty() ){
			CHyperLink &temp = m_Links.front();
			CHyperLink backup( temp);
			backup.SetBegin( backup.Begin() - shift );
			backup.SetEnd( backup.End() - shift );
			m_Links.pop_front();
			m_Links.push_back( backup );
			if( ((CHyperLink)m_Links.front()).Begin() == (UINT)litest )
				flag = false;
		}
		flag = true;
		while( flag == true && !m_KeyWords.empty() ){
			CKeyWord &temp = m_KeyWords.front();
			CKeyWord backup( temp.Begin()-shift, temp.End()-shift, temp.Color());
			m_KeyWords.pop_front();
			m_KeyWords.push_back( backup );
			if( ((CKeyWord)m_KeyWords.front()).Begin() == (UINT)witest )
				flag = false;
		}
	}
	////////////////////////////////////////////////
	//Bottom: May not be the nicest code but it works.
	////////////////////////////////////////////////
	m_sText+=sText;
	m_Links.push_back(CHyperLink(len, len + sText.GetLength() - 1, sTitle, hWnd, uMsg, wParam, lParam));
}

void CPreparedHyperText::AppendKeyWord(const CString& sText, COLORREF iColor){
	if (!sText.GetLength())
		return;
	int len = m_sText.GetLength();
	////////////////////////////////////////////////
	//Top:The Original code didn't check to see if the buffer was full..
	////////////////////////////////////////////////
	bool flag = true;
	if( len > 60000 ){
		m_sText = m_sText.Right(50000);
		int shift = len - m_sText.GetLength();
		while( flag == true ){
			CHyperLink &test = m_Links.front();
			if( !m_Links.empty() ){
				if( test.Begin() < shift )
					m_Links.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		flag = true;
		while( flag == true ){
			CKeyWord &test = m_KeyWords.front();
			if( !m_KeyWords.empty() ){
				if( test.Begin() < shift )
					m_KeyWords.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		len = m_sText.GetLength();
		CHyperLink &ltest = m_Links.front();
		int litest = ltest.Begin() - shift;
		CKeyWord &wtest = m_KeyWords.front();
		int witest = wtest.Begin() - shift;
		flag = true;
		while( flag == true && !m_Links.empty() ){
			CHyperLink &temp = m_Links.front();
			CHyperLink backup( temp);
			backup.SetBegin( backup.Begin() - shift );
			backup.SetEnd( backup.End() - shift );
			m_Links.pop_front();
			m_Links.push_back( backup );
			if( ((CHyperLink)m_Links.front()).Begin() == (UINT)litest )
				flag = false;
		}
		flag = true;
		while( flag == true && !m_KeyWords.empty() ){
			CKeyWord &temp = m_KeyWords.front();
			CKeyWord backup( temp.Begin()-shift, temp.End()-shift, temp.Color());
			m_KeyWords.pop_front();
			m_KeyWords.push_back( backup );
			if( ((CKeyWord)m_KeyWords.front()).Begin() == (UINT)witest )
				flag = false;
		}
	}
	////////////////////////////////////////////////
	//Bottom: May not be the nicest code but it works.
	////////////////////////////////////////////////
	m_sText+=sText;
	m_KeyWords.push_back(CKeyWord(len, len + sText.GetLength() - 1, iColor));
}

//CLinePartInfo

 CLinePartInfo::CLinePartInfo(int iBegin, uint16 iEnd, CHyperLink* pHyperLink, CKeyWord* pKeyWord){
	m_xBegin = (uint16)iBegin;
	m_xEnd = iEnd;
	m_pHyperLink = pHyperLink;
	m_pKeyWord = pKeyWord;
}

 CLinePartInfo::CLinePartInfo(const CLinePartInfo& Src){
	m_xBegin = Src.m_xBegin;
	m_xEnd = Src.m_xEnd;
	m_pHyperLink = Src.m_pHyperLink;
	m_pKeyWord = Src.m_pKeyWord;
}


//CLineInfo

 CLineInfo::CLineInfo(int iBegin, uint16 iEnd){
	m_iBegin = iBegin;
	m_iEnd = iEnd;
}

 CLineInfo::CLineInfo(const CLineInfo& Src){
	m_iBegin = Src.m_iBegin;
	m_iEnd = Src.m_iEnd;
	assign(Src.begin(), Src.end());
}


//CVisPart
 CVisPart::CVisPart(const CLinePartInfo& LinePartInfo, const CRect& rcBounds, int iRealBegin, uint16 iRealLen,CVisPart* pPrev,CVisPart* pNext) : CLinePartInfo(LinePartInfo)
{
	m_rcBounds = rcBounds;
	m_iRealBegin = iRealBegin;
	m_iRealLen = iRealLen;
	m_pPrev = pPrev;
	m_pNext = pNext;
}

 CVisPart::CVisPart(const CVisPart& Src) : CLinePartInfo(Src){
	m_rcBounds = Src.m_rcBounds;
	m_iRealBegin = Src.m_iRealBegin;
	m_iRealLen = Src.m_iRealLen;
	m_pPrev = Src.m_pPrev;
	m_pNext = Src.m_pNext;
}



// --------------------------------------------------------------
// CHyperTextCtrl
IMPLEMENT_DYNAMIC(CHyperTextCtrl, CWnd)

BEGIN_MESSAGE_MAP(CHyperTextCtrl, CWnd)
	ON_MESSAGE(WM_PAINT,OnPaint)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_MESSAGE(WM_SIZE, OnSize)
	ON_MESSAGE(WM_SHOWWINDOW, OnShowWindow)
	ON_MESSAGE(WM_CREATE, OnCreate)
	ON_MESSAGE(WM_DESTROY, OnDestroy)
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	ON_MESSAGE(WM_GETTEXT, OnGetText)
	ON_MESSAGE(WM_SETFONT, OnSetFont)
	ON_MESSAGE(WM_GETFONT, OnGetFont)
	ON_WM_ERASEBKGND()
	ON_MESSAGE(WM_HSCROLL, OnHScroll)
	ON_MESSAGE(WM_VSCROLL, OnVScroll)
	ON_MESSAGE(WM_CAPTURECHANGED, OnCaptureChanged)
	ON_WM_SYSCOLORCHANGE()
	//REFLECT_NOTIFICATIONS()
END_MESSAGE_MAP()

CHyperTextCtrl::CHyperTextCtrl()
{
	m_Text = &standart_Text;
	vscrollon = false;
	m_Font = NULL;
	m_BkColor = RGB(0,0,0);
	m_TextColor = RGB(0,0,0);
	m_LinkColor = RGB(0,0,0);
	m_HoverColor = RGB(0,0,0);
	m_LinkCursor = NULL;
	m_DefaultCursor = NULL;
	vscrollon = false;
	m_iMaxWidth = 0;
	m_iLineHeight = 0;
	m_iLinesHeight = 0;
	m_bDontUpdateSizeInfo = false;
	m_iVertPos = 0;
	m_iHorzPos = 0;
	m_pActivePart = NULL;
	m_iWheelDelta = 0;
}

//message handlers

LRESULT CHyperTextCtrl::OnDestroy(WPARAM wParam, LPARAM lParam){  
	if (m_LinkCursor){
		SetCursor(m_DefaultCursor);
		VERIFY( DestroyCursor(m_LinkCursor) );
	}
	m_LinkCursor = NULL;

	return 0;
}

LRESULT CHyperTextCtrl::OnCreate(WPARAM wParam, LPARAM lParam){  
	//LPCREATESTRUCT lpCreateStruct = (LPCREATESTRUCT)lParam;
	m_iMaxWidth = 0;
	m_iLinesHeight = 0;
	m_bDontUpdateSizeInfo = false;
	m_iHorzPos = 0;
	m_iVertPos = 0;
	m_Font = &theApp.m_fontHyperText;
	SetColors(); 
	LoadHandCursor();
	m_DefaultCursor = LoadCursor(NULL,IDC_ARROW);
	m_pActivePart = NULL;
	m_iWheelDelta = 0;

	// create a tool tip
	m_tip.Create(this);
	if(m_tip)
		m_tip.Activate(TRUE);

	UpdateFonts();
	return 0; 
}

LRESULT CHyperTextCtrl::OnPaint(WPARAM wParam, LPARAM lParam){
	CPaintDC dc(this); // device context for painting
	CFont* hOldFont = dc.SelectObject(m_Font);

	dc.SetBkColor(m_BkColor);

	int ypos = 0;
	LPCTSTR s = m_Text->GetText();

	CRect rc;
	CRect rcClient;
	GetClientRect(rcClient);
	rc.left = dc.m_ps.rcPaint.left;
	rc.right = 2;
	rc.top = dc.m_ps.rcPaint.top;
	rc.bottom = dc.m_ps.rcPaint.bottom;

	CBrush brBk;
	brBk.CreateSolidBrush(m_BkColor);
	dc.FillRect(rc, &brBk);

	for(std::vector<CVisLine>::iterator it = m_VisLines.begin(); it != m_VisLines.end(); it++){
		int iLastX = dc.m_ps.rcPaint.left;

		for(CVisLine::iterator jt = it->begin(); jt != it->end(); jt++){
			if (jt->m_pKeyWord)
				dc.SetTextColor(jt->m_pKeyWord->Color());
			else if(jt->m_pHyperLink == NULL)
				dc.SetTextColor(m_TextColor);
			else{
				if(m_pActivePart != NULL && m_pActivePart->m_pHyperLink == jt->m_pHyperLink){
					dc.SetTextColor(m_HoverColor);
					dc.SelectObject(m_HoverFont);
				}
				else{
					dc.SetTextColor(m_LinkColor);
					dc.SelectObject(m_LinksFont);
				}
			}

			TextOut(dc, jt->m_rcBounds.left, jt->m_rcBounds.top, s + jt->m_iRealBegin, jt->m_iRealLen);

			if(jt->m_pHyperLink != NULL)
				dc.SelectObject(m_Font);

			iLastX = jt->m_rcBounds.right;
		}

		rc.left = iLastX;
		rc.right = dc.m_ps.rcPaint.right;
		rc.top = ypos;
		rc.bottom = ypos + m_iLineHeight;

		dc.FillRect(rc, &brBk);

		ypos+=m_iLineHeight;
	}

	rc.left = dc.m_ps.rcPaint.left;
	rc.right = dc.m_ps.rcPaint.right;
	rc.top = ypos;
	rc.bottom = dc.m_ps.rcPaint.bottom;
	dc.FillRect(rc, &brBk);

	dc.SelectObject(hOldFont);
	return 0;
}

LRESULT CHyperTextCtrl::OnSize(WPARAM wParam, LPARAM lParam){
	WORD cx, cy;
	cx = LOWORD(lParam);
	cy = HIWORD(lParam);

	UpdateSize(IsWindowVisible() == TRUE);
	return 0;
}

LRESULT CHyperTextCtrl::OnShowWindow(WPARAM wParam, LPARAM lParam){
	if(TRUE == (BOOL)wParam)
		UpdateSize(false);
	return 0;
}

LRESULT CHyperTextCtrl::OnSetText(WPARAM wParam, LPARAM lParam){
	m_Text->SetText((LPTSTR)lParam);
	UpdateSize(IsWindowVisible() == TRUE);
	return TRUE;
}

LRESULT CHyperTextCtrl::OnGetText(WPARAM wParam, LPARAM lParam){
	int bufsize = wParam;
	LPTSTR buf = (LPTSTR)lParam;
	if(lParam == NULL || bufsize == 0 || m_Text->GetText().IsEmpty())
		return 0;
	int cpy = m_Text->GetText().GetLength() > (bufsize-1) ? (bufsize-1) : m_Text->GetText().GetLength();
	_tcsncpy(buf, m_Text->GetText(), cpy);
	return cpy;
}

LRESULT CHyperTextCtrl::OnSetFont(WPARAM wParam, LPARAM lParam){
	m_Font = CFont::FromHandle((HFONT)wParam);
	UpdateFonts();
	UpdateSize(LOWORD(lParam) != 0);
	return 0;
}

LRESULT CHyperTextCtrl::OnGetFont(WPARAM wParam, LPARAM lParam){
	return (LRESULT)m_Font->m_hObject;
}

LRESULT CHyperTextCtrl::OnHScroll(WPARAM wParam, LPARAM lParam){
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;

	GetScrollInfo(SB_HORZ, &si);

	switch(LOWORD(wParam))
	{
	case SB_LEFT:
		si.nPos=si.nMin;
		break;

	case SB_RIGHT:
		si.nPos=si.nMax;
		break;

	case SB_LINELEFT:
		if(si.nPos > si.nMin)
			si.nPos-=1;
		break;

	case SB_LINERIGHT:
		if(si.nPos < si.nMax)
			si.nPos+=1;
		break;

	case SB_PAGELEFT:
		if(si.nPos > si.nMin)
			si.nPos-=si.nPage;
		if(si.nPos < si.nMin)
			si.nPos = si.nMin;
		break;

	case SB_PAGERIGHT:
		if(si.nPos < si.nMax)
			si.nPos+=si.nPage;
		if(si.nPos > si.nMax)
			si.nPos = si.nMax;
		break;

	case SB_THUMBTRACK:
		si.nPos=si.nTrackPos;
		break;
	}

	if(si.nMax != si.nMin)
		m_iHorzPos = si.nPos * 100 / (si.nMax - si.nMin);
	SetScrollInfo(SB_HORZ, &si);
	UpdateVisLines();
	InvalidateRect(NULL,FALSE);
	return TRUE;
}

LRESULT CHyperTextCtrl::OnVScroll(WPARAM wParam, LPARAM lParam){
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si);

	switch(LOWORD(wParam))
	{
	case SB_TOP:
		si.nPos=si.nMin;
		break;

	case SB_BOTTOM:
		si.nPos=si.nMax;
		break;

	case SB_LINEUP:
		if(si.nPos > si.nMin)
			si.nPos-=1;
		break;

	case SB_LINEDOWN:
		if(si.nPos < si.nMax)
			si.nPos+=1;
		break;

	case SB_PAGEUP:
		if(si.nPos > si.nMin)
			si.nPos-=si.nPage;
		if(si.nPos < si.nMin)
			si.nPos = si.nMin;
		break;

	case SB_PAGEDOWN:
		if(si.nPos < si.nMax)
			si.nPos+=si.nPage;
		if(si.nPos > si.nMax)
			si.nPos = si.nMax;
		break;

	case SB_THUMBTRACK:
		si.nPos=si.nTrackPos;
		break;
	}

	if(si.nMax != si.nMin)
		m_iVertPos = si.nPos * 100 / (si.nMax - si.nMin);
	SetScrollInfo(SB_VERT, &si);
	UpdateVisLines();
	InvalidateRect(NULL,FALSE);
	return TRUE;
}

void CHyperTextCtrl::OnMouseMove(UINT nFlags,CPoint pt){
	CRect rcClient;
	GetClientRect(rcClient);
	if(PtInRect(rcClient, pt) && m_iLineHeight)
	{
		bool bFound = false;
		UINT i = pt.y / m_iLineHeight;
		if(i < m_VisLines.size())
		{
			std::vector<CVisLine>::iterator it = m_VisLines.begin() + i;
			for(CVisLine::iterator jt = it->begin(); jt != it->end(); jt++)
				if(pt.x >= jt->m_rcBounds.left && pt.x <= jt->m_rcBounds.right)
				{
					if(jt->m_pHyperLink != NULL)
					{
						HighlightLink(&*jt, pt);
						bFound = true;
						if (GetCapture() != this)
							SetCapture();
					}
					break;
				}
		}
		if(!bFound){
			RestoreLink();
			if (GetCapture() == this)
				ReleaseCapture(); 
		}
	}
	else
		ReleaseCapture();
}

void CHyperTextCtrl::OnLButtonDown(UINT nFlags,CPoint pt){
	CRect rcClient;
	GetClientRect(rcClient);
	if(PtInRect(rcClient, pt) && m_iLineHeight)
	{
		bool bFound = false;
		UINT i = pt.y / m_iLineHeight;
		if(i < m_VisLines.size()){
			std::vector<CVisLine>::iterator it = m_VisLines.begin() + i;
			for(CVisLine::iterator jt = it->begin(); jt != it->end(); jt++)
				if(pt.x >= jt->m_rcBounds.left && pt.x <= jt->m_rcBounds.right)
				{
					if(jt->m_pHyperLink != NULL)
					{
						jt->m_pHyperLink->Execute();
						bFound = true;
					}
					break;
				}
		}

	}
	//m_tip.OnLButtonDown(nFlags,pt);
}

BOOL CHyperTextCtrl::OnMouseWheel(UINT nFlags,short zDelta,CPoint pt){
	CRect rc;
	GetWindowRect(rc);
	if(PtInRect(rc, pt))
	{
		int iScrollLines;
		SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 
			0, 
			&iScrollLines, 
			0);

		m_iWheelDelta -= zDelta;
		if(abs(m_iWheelDelta) >= WHEEL_DELTA)
		{
			if(m_iWheelDelta > 0)
			{
				for(int i = 0; i<iScrollLines; i++)
					PostMessage(WM_VSCROLL, SB_LINEDOWN, 0);
			}
			else
			{
				for(int i = 0; i<iScrollLines; i++)
					PostMessage(WM_VSCROLL, SB_LINEUP, 0);
			}

			m_iWheelDelta %= WHEEL_DELTA;
		}
	}
	return true;
	//m_tip.OnMouseWheel(nFlags,zDelta,pt);
}

LRESULT CHyperTextCtrl::OnCaptureChanged(WPARAM wParam, LPARAM lParam){    
	RestoreLink();
	return 0;
}

BOOL CHyperTextCtrl::OnEraseBkgnd(CDC* pDC){
	return TRUE;
}

// Operations
CPreparedHyperText* CHyperTextCtrl::GetHyperText(){
	return m_Text;
}

void CHyperTextCtrl::SetHyperText(CPreparedHyperText* Src, bool bInvalidate){
	if (Src)
		m_Text = Src;
	else
		m_Text = &standart_Text;
	UpdateSize(bInvalidate);
}

void CHyperTextCtrl::AppendText(const CString& sText, bool bInvalidate){
	m_Text->AppendText(sText);
	UpdateSize(bInvalidate);
}

void CHyperTextCtrl::AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory, bool bInvalidate){
	m_Text->AppendHyperLink(sText, sTitle, sCommand, sDirectory);
	UpdateSize(bInvalidate);
}

void CHyperTextCtrl::AppendKeyWord(const CString& sText, COLORREF icolor){
	m_Text->AppendKeyWord(sText,icolor);
	UpdateSize(true);
}

void CHyperTextCtrl::AppendHyperLink(const CString& sText, const CString& sTitle, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool bInvalidate){
	m_Text->AppendHyperLink(sText, sTitle, hWnd, uMsg, wParam, lParam);
	UpdateSize(bInvalidate);
}

void CHyperTextCtrl::SetLinkColor(COLORREF LinkColor, bool bInvalidate){
	m_LinkColor = LinkColor;
	if(bInvalidate)
		InvalidateRect(NULL,FALSE);
}

void CHyperTextCtrl::UpdateSize(bool bRepaint){
	if(m_bDontUpdateSizeInfo)
		return;
	m_bDontUpdateSizeInfo = true;
	DWORD dwStyle = GetWindowLongPtr(m_hWnd,GWL_STYLE);
	bool vscrollneeded = false;

	CClientDC dc(this);
	CFont* hOldFont = dc.SelectObject(m_Font);
	

	int iScrollHeight = GetSystemMetrics(SM_CYHSCROLL);

	m_Lines.clear();
	CRect rc;
	GetClientRect(rc);
	rc.DeflateRect(2,0);
	m_iMaxWidth = 0;
	m_iLinesHeight = 0;
	long iMaxWidthChars = 0;
	SIZE sz;

	if(rc.Width() > 5 && rc.Height() > 5)
	{
		std::list<CHyperLink>::iterator it = m_Text->GetLinks().begin();
		std::list<CKeyWord>::iterator ht = m_Text->GetKeywords().begin();
		LPCTSTR s = m_Text->GetText();
		int len = m_Text->GetText().GetLength();
		int width = rc.Width();

		int npos, // new position
			pos = 0, // current position
			ll, // line length
			rll; // line length with wordwrap (if used)

		while(len>0)
		{
			ll = len;
			npos = ll;
			for(int i = 0; i < len; i++)
			{
				if(s[i] == _T('\r') || s[i] == _T('\n'))
				{
					if(s[i] == _T('\r') && ((i+1) < len) && s[i+1] == _T('\n'))
						npos = i + 2;
					else
						npos = i + 1;

					ll = i;
					break;
				}
			}

			if(!::GetTextExtentExPoint(dc, s , (ll > 512) ? 512 : ll, width, &rll, NULL, &sz) || sz.cy == 0)
			{
				::GetTextExtentExPoint(dc, _T(" ") , 1, 0, NULL, NULL, &sz);
				sz.cx = 0;
				rll = ll;
			}

			if(rll>ll)
				rll = ll;

			if(!check_bits(dwStyle, HTC_WORDWRAP))
				rll = ll;
			else
				if(rll < ll)
					npos = rll;

			if(rll>0)
			{
				if((rll < len) && !_istspace((_TUCHAR)s[rll]))
					for(int i = rll - 1; i >= 0; i--)
						if(_istspace((_TUCHAR)s[i]))
						{
							rll = i;
							npos = i + 1;
							break;
						}
			}

			if(npos == 0)
				npos = 1;

			CLineInfo li(pos, pos + rll - 1);
			CLinePartInfo pl(pos, pos + rll - 1);

			while(it != m_Text->GetLinks().end() && it->End() < pos)
				it++;
			while(ht != m_Text->GetKeywords().end() && ht->End() < pos)
				ht++;

			//split the line into parts of hypertext, normaltext, keywords etc 
			for (int i = pl.Begin(); i < pl.End(); i++){
				if (it != m_Text->GetLinks().end() && i >= it->Begin() && it->End() > i){ // i_a 
					if (i > pl.m_xBegin){
						CLinePartInfo pln(pl.m_xBegin,i-1);
						li.push_back(pln);
					}

					if (it->End() > pl.End()){
						pl.m_xBegin =  pl.End() + 1;
						CLinePartInfo pln(i, pl.End(), &*it);
						li.push_back(pln);
						break;
					}
					else{
						pl.m_xBegin =  it->End() + 1;
						CLinePartInfo pln((uint16)i, (uint16)it->End(), &*it);
						li.push_back(pln);
						i = pl.m_xBegin;
						it++;
					}

				}
				else if (ht != m_Text->GetKeywords().end() && i >= ht->Begin() && ht->End() > i){ // i_a 
					if (i > pl.m_xBegin){
							CLinePartInfo pln(pl.m_xBegin,i-1);
							li.push_back(pln);
					}
					if (ht->End() > pl.End()){
						pl.m_xBegin =  pl.End() + 1;
						CLinePartInfo pln(i, pl.End(),0, &*ht);
						li.push_back(pln);
						break;
					}
					else{
						pl.m_xBegin =  ht->End() + 1;
						CLinePartInfo pln((uint16)i, (uint16)ht->End(),0, &*ht);
						li.push_back(pln);
						i = pl.m_xBegin;
						ht++;
					}

				}

			}

			if(pl.Len()>0)
				li.push_back(pl);

			m_iLineHeight = sz.cy;
			m_iLinesHeight+=m_iLineHeight;
			if(sz.cx > m_iMaxWidth)
				m_iMaxWidth = sz.cx;
			if(iMaxWidthChars < li.Len())
				iMaxWidthChars = li.Len();

			m_Lines.push_back(li);

			pos+=npos;
			s+=npos;
			len-=npos;

			if((m_iLinesHeight + iScrollHeight) > rc.Height()){
				vscrollneeded = true;
			}
		}
		if(bRepaint)
			InvalidateRect(rc);
	}
	
	dc.SelectObject(hOldFont);

	// Update scroll bars
	dwStyle = GetWindowLongPtr(m_hWnd,GWL_STYLE);
	if (check_bits(dwStyle, HTC_AUTO_SCROLL_BARS)){
		if (vscrollneeded){
			if (!vscrollon)
				ShowScrollBar(SB_VERT,TRUE);
			dwStyle|=WS_VSCROLL;
			vscrollon = true;
		}
		else if (!vscrollneeded){
			ShowScrollBar(SB_VERT,FALSE);
			vscrollon = false;
		}
	}

	if(check_bits(dwStyle, HTC_AUTO_SCROLL_BARS) && !check_bits(dwStyle, HTC_WORDWRAP))
	{
		if(m_iMaxWidth > rc.Width())
		{
			ShowScrollBar(SB_HORZ,TRUE);
			dwStyle|=WS_HSCROLL;
		};

	}

	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;

	if(check_bits(dwStyle,WS_HSCROLL) && m_iMaxWidth != 0)
	{
		si.nMin = 0;
		si.nMax = iMaxWidthChars + iMaxWidthChars/2;
		si.nPos = (int)(double(si.nMax) * m_iHorzPos / 100);
		si.nPage = (rc.Width() * si.nMax)/m_iMaxWidth;
		SetScrollInfo(SB_HORZ, &si, FALSE);
	}

	if(check_bits(dwStyle,WS_VSCROLL) && m_iLinesHeight != 0)
	{
		si.nMin = 0;
		si.nMax = (int)m_Lines.size();
		si.nPos = si.nMax;//(int)(double(si.nMax) * m_iVertPos / 100);
		si.nPage = (rc.Height() * si.nMax)/m_iLinesHeight;
		SetScrollInfo(SB_VERT, &si, TRUE);
	}

	m_bDontUpdateSizeInfo = false;
	UpdateVisLines();
}

void CHyperTextCtrl::UpdateFonts(){
	DWORD dwStyle = GetWindowLongPtr(m_hWnd,GWL_STYLE);
	m_LinksFont.DeleteObject();
	m_HoverFont.DeleteObject();
	
	LOGFONT lf;
	m_Font->GetLogFont(&lf);
	if(check_bits(dwStyle, HTC_UNDERLINE_LINKS))
		lf.lfUnderline = TRUE;
	m_LinksFont.CreateFontIndirect(&lf);

	m_Font->GetLogFont(&lf);
	if(check_bits(dwStyle, HTC_UNDERLINE_HOVER))
		lf.lfUnderline = TRUE;
	m_HoverFont.CreateFontIndirect(&lf);
}

void CHyperTextCtrl::UpdateVisLines(){
	RestoreLink();
	DWORD dwStyle = ::GetWindowLongPtr(m_hWnd,GWL_STYLE);
	int id = 1;
	if(check_bits(dwStyle, HTC_ENABLE_TOOLTIPS))
	{
		for(std::vector<CVisLine>::iterator itv = m_VisLines.begin(); itv != m_VisLines.end(); itv++)
			for(CVisLine::iterator jt = itv->begin(); jt != itv->end(); jt++)
			{
				if(jt->m_pHyperLink != NULL)
					m_tip.DelTool(this, id++);
			}
	}

	m_VisLines.clear();

	std::vector<CLineInfo>::iterator it = m_Lines.begin();
	int iVertPos = 0;
	int iHorzPos = 0;
	if(check_bits(dwStyle,WS_VSCROLL))
		iVertPos = GetScrollPos(SB_VERT);
	if(check_bits(dwStyle,WS_HSCROLL))
		iHorzPos = GetScrollPos(SB_HORZ);

	if(iVertPos >= (int)m_Lines.size())
		return;

	it+=iVertPos;

	CClientDC dc(this); // device context for painting

	CFont* hOldFont = dc.SelectObject(m_Font);

	int ypos = 0;
	LPCTSTR s = m_Text->GetText();

	CRect rcClient;
	GetClientRect(rcClient);

	for(; it != m_Lines.end(); it++)
	{
		int XPos = 2;
		UINT LinePos = it->Begin();
		UINT Offset = 0;
		UINT Len = 0;

		CVisLine vl;
		CRect rcBounds;

		std::vector<CLinePartInfo>::iterator jt;

		for(jt = it->begin(); jt != it->end(); jt++)
		{
			if(jt->Begin() <= (LinePos + iHorzPos) && jt->End() >= (LinePos + iHorzPos))
			{
				Offset = LinePos + iHorzPos;
				Len = jt->Len() - ((LinePos + iHorzPos) - jt->Begin());
				break;
			}
		}

		while(jt != it->end())
		{
			if(Len > 0)
			{
				SIZE sz;
				::GetTextExtentExPoint(dc, s + Offset, Len, 0, NULL, NULL, &sz);

				rcBounds.left = XPos;
				XPos += sz.cx;
				rcBounds.right = XPos;
				rcBounds.top = ypos;
				rcBounds.bottom = ypos+m_iLineHeight;

				vl.push_back(CVisPart(*jt, rcBounds, Offset, (uint16)Len, NULL, NULL));
			}

			if(XPos > rcClient.Width())
				break;

			jt++;
			if (jt == it->end())
				break;
			Offset = jt->m_xBegin;
			Len = jt->Len();
			
		}

		m_VisLines.push_back(vl);
		ypos+=m_iLineHeight;
		if(ypos>rcClient.bottom)
			break;
	}

	CVisPart *pPrev = NULL, *pNext;

	id = 1;
	for(std::vector<CVisLine>::iterator it2 = m_VisLines.begin(); it2 != m_VisLines.end(); it2++)
		for(CVisLine::iterator jt = it2->begin(); jt != it2->end(); jt++)
		{
			pNext = &*jt;
			if(pPrev != NULL && 
				pPrev->m_pHyperLink != NULL && 
				pPrev->m_pHyperLink == pNext->m_pHyperLink &&
				pPrev != pNext)
			{
				pPrev->m_pNext = pNext;
				pNext->m_pPrev = pPrev;
			}
			pPrev = pNext;

			if(check_bits(dwStyle, HTC_ENABLE_TOOLTIPS) && jt->m_pHyperLink != NULL)
				m_tip.AddTool(this, (LPCTSTR)jt->m_pHyperLink->Title(), jt->m_rcBounds, id++);
		}

		dc.SelectObject(hOldFont);
}

void CHyperTextCtrl::HighlightLink(CVisPart* Part, const CPoint& MouseCoords){
	if(m_pActivePart == Part)
		return;

	if(m_pActivePart != Part && m_pActivePart != NULL && Part != NULL && m_pActivePart->m_pHyperLink != Part->m_pHyperLink)
		RestoreLink();

	m_pActivePart = Part;
	while(m_pActivePart->m_pPrev != NULL)
		m_pActivePart = m_pActivePart->m_pPrev;

	CClientDC dc(this);
	CFont* hOldFont = dc.SelectObject(&m_HoverFont);
	dc.SetBkColor(m_BkColor);
	dc.SetTextColor(m_HoverColor);
	LPCTSTR s = m_Text->GetText();

	CVisPart* p = m_pActivePart;
	while(p != NULL)
	{
		TextOut(dc, p->m_rcBounds.left, p->m_rcBounds.top, 
			s + p->m_iRealBegin, p->m_iRealLen);
		p = p->m_pNext;
	}

	dc.SelectObject(hOldFont);

	SetCursor(m_LinkCursor);
}

void CHyperTextCtrl::RestoreLink(){
	if(m_pActivePart == NULL)
		return;

	CClientDC dc(this);
	CFont* hOldFont = dc.SelectObject(&m_LinksFont);
	dc.SetBkColor(m_BkColor);
	dc.SetTextColor(m_LinkColor);
	LPCTSTR s = m_Text->GetText();

	CVisPart* p = m_pActivePart;
	while(p != NULL)
	{
		TextOut(dc, p->m_rcBounds.left, p->m_rcBounds.top, 
			s + p->m_iRealBegin, p->m_iRealLen);
		p = p->m_pNext;
	}

	dc.SelectObject(hOldFont);

	m_pActivePart = NULL;
	SetCursor(m_DefaultCursor);
}

void CHyperTextCtrl::OnSysColorChange() {
	//adjust colors
	CWnd::OnSysColorChange();
	SetColors();
}

void CHyperTextCtrl::SetColors() {
	m_BkColor = GetSysColor(COLOR_WINDOW);
	m_TextColor = GetSysColor(COLOR_WINDOWTEXT);
	//perhaps some sort of check against the bk and text color can be made
	//before blindly using these default link colors?
	m_LinkColor = RGB(0,0,255);
	m_HoverColor = RGB(255,0,0);
}

void CHyperTextCtrl::LoadHandCursor()
{
	CString windir;
	(void)GetWindowsDirectory(windir.GetBuffer(MAX_PATH), MAX_PATH);
	windir.ReleaseBuffer();
	windir += _T("\\winhlp32.exe");
	HMODULE hModule = LoadLibrary(windir);
	ASSERT( m_LinkCursor == NULL );
	if (hModule)
	{
		HCURSOR hTempCursor = ::LoadCursor(hModule, MAKEINTRESOURCE(106));
		if (hTempCursor)
			m_LinkCursor = CopyCursor(hTempCursor);
		FreeLibrary(hModule);
	}

	if (m_LinkCursor == NULL)
		m_LinkCursor = CopyCursor(::LoadCursor(NULL,IDC_ARROW));
}
