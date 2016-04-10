// ------------------------------------------------------------
//  BEGIN_MESSAGE_MAP_TEMPLATE
//  MFC (VisualC++ 6/7) BEGIN_MESSAGE_MAP template compatibility
// ------------------------------------------------------------
//  AfxBeginMsgMapTemplate.hpp
//  zegzav - 1/10/2002 - eMule project (http://www.emule-project.net)
// ------------------------------------------------------------
#pragma once

#if _MFC_VER >= 0x0600 && _MFC_VER < 0x0700

// MFC for Visual C++ 6.0 (MFC 4.x)

#ifdef _AFXDLL
#define BEGIN_MESSAGE_MAP_TEMPLATE(templateList, templateClass, theClass, baseClass) \
    templateList const AFX_MSGMAP* PASCAL templateClass::_GetBaseMessageMap() \
        { return &baseClass::messageMap; } \
    templateList const AFX_MSGMAP* templateClass::GetMessageMap() const \
        { return &theClass::messageMap; } \
    templateList AFX_COMDAT AFX_DATADEF const AFX_MSGMAP templateClass::messageMap = \
        { &theClass::_GetBaseMessageMap, &theClass::_messageEntries[0] }; \
    templateList AFX_COMDAT const AFX_MSGMAP_ENTRY templateClass::_messageEntries[] = \
        {
#else
#define BEGIN_MESSAGE_MAP_TEMPLATE(templateList, templateClass, theClass, baseClass) \
    templateList const AFX_MSGMAP* templateClass::GetMessageMap() const \
        { return &theClass::messageMap; } \
    templateList AFX_COMDAT AFX_DATADEF const AFX_MSGMAP templateClass::messageMap = \
        { &baseClass::messageMap, &theClass::_messageEntries[0] }; \
    templateList AFX_COMDAT const AFX_MSGMAP_ENTRY templateClass::_messageEntries[] = \
        {
#endif

#elif _MFC_VER >= 0x0700 && _MFC_VER < 0x0800

// MFC for Visual C++ 7.0 (MFC 7.x)

#ifdef _AFXDLL
#define BEGIN_MESSAGE_MAP_TEMPLATE(templateList, templateClass, theClass, baseClass) \
    templateList const AFX_MSGMAP* PASCAL templateClass::GetThisMessageMap() \
        { return &theClass::messageMap; } \
    templateList const AFX_MSGMAP* templateClass::GetMessageMap() const \
        { return &theClass::messageMap; } \
    templateList /*AFX_COMDAT*/ const AFX_MSGMAP templateClass::messageMap = \
        { &baseClass::GetThisMessageMap, &theClass::_messageEntries[0] }; \
    templateList /*AFX_COMDAT*/ const AFX_MSGMAP_ENTRY templateClass::_messageEntries[] = \
        {
#else
#define BEGIN_MESSAGE_MAP_TEMPLATE(templateList, templateClass, theClass, baseClass) \
    templateList const AFX_MSGMAP* templateClass::GetMessageMap() const \
        { return &theClass::messageMap; } \
    templateList /*AFX_COMDAT*/ const AFX_MSGMAP templateClass::messageMap = \
        { &baseClass::messageMap, &theClass::_messageEntries[0] }; \
    templateList /*AFX_COMDAT*/ const AFX_MSGMAP_ENTRY templateClass::_messageEntries[] = \
        {
#endif

#elif _MFC_VER>=0x0800

// MFC for Visual C++ 7.0 (MFC 7.x)

#ifdef _AFXDLL
#define BEGIN_MESSAGE_MAP_TEMPLATE(templateList, templateClass, theClass, baseClass) \
    templateList const AFX_MSGMAP* PASCAL templateClass::GetThisMessageMap() \
        { return &theClass::messageMap; } \
    templateList const AFX_MSGMAP* templateClass::GetMessageMap() const \
        { return &theClass::messageMap; } \
    templateList /*AFX_COMDAT*/ const AFX_MSGMAP templateClass::messageMap = \
        { &baseClass::GetThisMessageMap, &theClass::_messageEntries[0] }; \
    templateList /*AFX_COMDAT*/ const AFX_MSGMAP_ENTRY templateClass::_messageEntries[] = \
        {
#else
#define BEGIN_MESSAGE_MAP_TEMPLATE(templateList, templateClass, theClass, baseClass) \
	PTM_WARNING_DISABLE \
    templateList const AFX_MSGMAP* templateClass::GetMessageMap() const \
        { return GetThisMessageMap(); } \
    templateList const AFX_MSGMAP* PASCAL templateClass::GetThisMessageMap() \
		{ \
			typedef theClass ThisClass;						   \
			typedef baseClass TheBaseClass;					   \
			static const AFX_MSGMAP_ENTRY _messageEntries[] =  \
			{
#endif

#else

#error "MFC version not supported"

#endif
