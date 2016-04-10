//
// UPnPFinder.cpp
//
// Copyright (c) Shareaza Development Team, 2002-2005.
// This file is part of SHAREAZA (www.shareaza.com)
//
// this file is part of eMule
// Copyright (C)2007 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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

#include "StdAfx.h"
#include "emule.h"
#include "preferences.h"
#include "UPnPImplWinServ.h"
#include "Log.h"
#include "Otherfunctions.h"

#include <algorithm>
#include <map>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CUPnPImplWinServ::CUPnPImplWinServ()
:	m_pDevices(),
	m_pServices(),
	m_bCOM( false ),
	m_pDeviceFinder( NULL ),
	m_nAsyncFindHandle( 0 ),
	m_bAsyncFindRunning( false ),
	m_bADSL( false ),
	m_ADSLFailed( false ),
	m_bPortIsFree( true ),
	m_sLocalIP(),
	m_sExternalIP(),
	m_tLastEvent( GetTickCount() ),
	m_pDeviceFinderCallback( NULL ),
	m_pServiceCallback( NULL ),
	m_bUPnPDeviceConnected( TRIS_FALSE),
	m_bInited(false),
	m_hADVAPI32_DLL(0),
	m_pfnOpenSCManager(0),
	m_pfnOpenService(0),
	m_pfnQueryServiceStatusEx(0),
	m_pfnCloseServiceHandle(0),
	m_pfnStartService(0),
	m_bSecondTry(false),
	m_pfGetBestInterface(NULL),
	m_pfGetIpAddrTable(NULL),
	m_pfGetIfEntry(NULL),
	m_hIPHLPAPI_DLL(NULL),
	m_bServiceStartedByEmule(false)
{
	m_bDisableWANIPSetup = thePrefs.GetSkipWANIPSetup();
	m_bDisableWANPPPSetup = thePrefs.GetSkipWANPPPSetup();
}

void CUPnPImplWinServ::Init(){
	if (!m_bInited){
		DebugLog(_T("Using Windows Service based UPnP Implementation"));
		m_hADVAPI32_DLL = LoadLibrary(_T("Advapi32.dll"));
		if (m_hADVAPI32_DLL != 0) {
			(FARPROC&)m_pfnOpenSCManager = GetProcAddress(	m_hADVAPI32_DLL, "OpenSCManagerW" );
			(FARPROC&)m_pfnOpenService = GetProcAddress( m_hADVAPI32_DLL, "OpenServiceW" );
			(FARPROC&)m_pfnQueryServiceStatusEx = GetProcAddress( m_hADVAPI32_DLL, "QueryServiceStatusEx" );
			(FARPROC&)m_pfnCloseServiceHandle = GetProcAddress( m_hADVAPI32_DLL, "CloseServiceHandle" );
			(FARPROC&)m_pfnStartService = GetProcAddress( m_hADVAPI32_DLL, "StartServiceW" );
			(FARPROC&)m_pfnControlService = GetProcAddress( m_hADVAPI32_DLL, "ControlService" );
		}
		m_hIPHLPAPI_DLL = LoadLibrary(_T("iphlpapi.dll"));
		if (m_hIPHLPAPI_DLL != 0) {
			(FARPROC&)m_pfGetBestInterface = GetProcAddress(m_hIPHLPAPI_DLL, "GetBestInterface" );
			(FARPROC&)m_pfGetIpAddrTable = GetProcAddress(m_hIPHLPAPI_DLL, "GetIpAddrTable" );
			(FARPROC&)m_pfGetIfEntry = GetProcAddress(m_hIPHLPAPI_DLL, "GetIfEntry" );
		}
		if (m_pfGetBestInterface == NULL || m_pfGetIpAddrTable == NULL || m_pfGetIfEntry == NULL){
			DebugLogError(_T("Failed to load functions from iphlpapi.dll for UPnP"));
			ASSERT( false );
		}
		HRESULT hr = CoInitialize( NULL );
		m_bCOM = (hr == S_OK || hr == S_FALSE);
		m_pDeviceFinder = CreateFinderInstance();
		m_pServiceCallback = new CServiceCallback( *this );
		m_pDeviceFinderCallback = new CDeviceFinderCallback( *this );
		m_bInited = true;
	}
}

FinderPointer CUPnPImplWinServ::CreateFinderInstance()
{
	void* pNewDeviceFinder = NULL;
	if ( FAILED( CoCreateInstance( CLSID_UPnPDeviceFinder, NULL, CLSCTX_INPROC_SERVER,
							IID_IUPnPDeviceFinder, &pNewDeviceFinder ) ) )
	{
		// Should we ask to disable auto-detection?
		DebugLogWarning(_T("UPnP discovery is not supported or not installed - CreateFinderInstance() failed"));

		throw UPnPError();
	}
	return FinderPointer( static_cast< IUPnPDeviceFinder* >( pNewDeviceFinder ), false );
}

CUPnPImplWinServ::~CUPnPImplWinServ()
{	
	if (m_hADVAPI32_DLL != 0){
		FreeLibrary(m_hADVAPI32_DLL);
		m_hADVAPI32_DLL = 0;
	}
	if (m_hIPHLPAPI_DLL != 0){
		FreeLibrary(m_hIPHLPAPI_DLL);
		m_hIPHLPAPI_DLL = 0;
	}
	m_pDevices.clear();
	m_pServices.clear();

	if (m_bCOM)
		CoUninitialize();
}

// Helper function to check if UPnP Device Host service is healthy
// Although SSPD service is dependent on this service but sometimes it may lock up.
// This will result in application lockup when we call any methods of IUPnPDeviceFinder.
// ToDo: Add a support for WinME.
bool CUPnPImplWinServ::IsReady()
{
	switch (thePrefs.GetWindowsVersion()){
		case _WINVER_ME_:
			return true;
		case _WINVER_2K_:
		case _WINVER_XP_:
		case _WINVER_2003_:
		case _WINVER_VISTA_:
		case _WINVER_7_:
			break;
		default:
			return false;
	}
	Init();

	bool bResult = false;
	if ( m_pfnOpenSCManager && m_pfnOpenService && 
		m_pfnQueryServiceStatusEx && m_pfnCloseServiceHandle && m_pfnStartService )
	{
		SC_HANDLE schSCManager;
		SC_HANDLE schService;

		// Open a handle to the Service Control Manager database
		schSCManager = m_pfnOpenSCManager( 
			NULL,				// local machine 
			NULL,				// ServicesActive database 
			GENERIC_READ );		// for enumeration and status lookup 

		if ( schSCManager == NULL )
			return false;

		schService = m_pfnOpenService( schSCManager, L"upnphost", GENERIC_READ ); 
		if ( schService == NULL )
		{
			m_pfnCloseServiceHandle( schSCManager );
			return false;
		}

		SERVICE_STATUS_PROCESS ssStatus; 
		DWORD nBytesNeeded;

		if ( m_pfnQueryServiceStatusEx( schService, SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &nBytesNeeded ) )
		{
			if ( ssStatus.dwCurrentState == SERVICE_RUNNING )
				bResult = true;
		}
		m_pfnCloseServiceHandle( schService );

		if ( !bResult )
		{
			schService = m_pfnOpenService( schSCManager, L"upnphost", SERVICE_START );
			if ( schService )
			{
				// Power users have only right to start service, thus try to start it here
				if ( m_pfnStartService( schService, 0, NULL ) ){
					bResult = true;
					m_bServiceStartedByEmule = true;
				}
				m_pfnCloseServiceHandle( schService );
			}
		}
		m_pfnCloseServiceHandle( schSCManager );
	}

	if ( !bResult )
	{
		Log(GetResString(IDS_UPNP_NOSERVICE));
	}

	return bResult;
}

void CUPnPImplWinServ::StopUPnPService(){
	ASSERT( m_bServiceStartedByEmule );
	if ( m_bInited && m_pfnOpenService && m_pfnCloseServiceHandle && m_pfnControlService &&  m_pfnOpenSCManager)
	{
		SC_HANDLE schSCManager;
		SC_HANDLE schService;
		m_bServiceStartedByEmule = false;

		// Open a handle to the Service Control Manager database
		schSCManager = m_pfnOpenSCManager( 
			NULL,				// local machine 
			NULL,				// ServicesActive database 
			GENERIC_READ );		// for enumeration and status lookup 

		if (schSCManager == NULL)
			return;

		schService = m_pfnOpenService(schSCManager, L"upnphost", SERVICE_STOP);
		if (schService){
			SERVICE_STATUS structServiceStatus; 
			if (m_pfnControlService( schService, SERVICE_CONTROL_STOP, &structServiceStatus) != 0){
				DebugLog(_T("Shutting down UPnP Service: Succeeded"));
			}
			else{
				DebugLogWarning(_T("Shutting down UPnP Service: Failed, ErrorCode: %u"), GetLastError());
			}
			m_pfnCloseServiceHandle( schService );
		}
		else
			DebugLogWarning(_T("Shutting down UPnP Service: Unable to open service"));
		m_pfnCloseServiceHandle( schSCManager );
	}
}


// Helper function for processing the AsyncFind search
void CUPnPImplWinServ::ProcessAsyncFind(CComBSTR bsSearchType)
{
	// We have to start the AsyncFind.
	if ( m_pDeviceFinderCallback == NULL )
		return DebugLogError(_T("DeviceFinderCallback object is not available."));

	HRESULT res = m_pDeviceFinder->CreateAsyncFind(bsSearchType, NULL, m_pDeviceFinderCallback, &m_nAsyncFindHandle);
	if ( FAILED( res ) )
		return DebugLogError(_T("CreateAsyncFind failed in UPnP finder."));

	m_bAsyncFindRunning = true;
	m_tLastEvent = GetTickCount();

	if ( FAILED( m_pDeviceFinder->StartAsyncFind( m_nAsyncFindHandle ) ) )
	{
		if ( FAILED( m_pDeviceFinder->CancelAsyncFind( m_nAsyncFindHandle ) ) )
			DebugLogError(_T("CancelAsyncFind failed in UPnP finder."));
		
		m_bAsyncFindRunning = false;
		return;
	} 
}

// Helper function for stopping the async find if proceeding
void CUPnPImplWinServ::StopAsyncFind()
{
	// This will stop the async find if it is in progress
	// ToDo: Locks up in WinME, cancelling is required <- critical

	if ( m_bInited && thePrefs.GetWindowsVersion() != _WINVER_ME_ && IsAsyncFindRunning() )
	{
		if ( FAILED( m_pDeviceFinder->CancelAsyncFind( m_nAsyncFindHandle ) ) )
			DebugLogError(_T("Cancel AsyncFind failed in UPnP finder."));
	}
	
	m_bAsyncFindRunning = false;
}

// Start the discovery of the UPnP gateway devices
void CUPnPImplWinServ::StartDiscovery(uint16 nTCPPort, uint16 nUDPPort, uint16 nTCPWebPort, bool bSecondTry)
{
	if (bSecondTry && m_bSecondTry) // already did 2 tries
		return;

	Init();
	if (!bSecondTry){
		m_bCheckAndRefresh = false;
	}

	// On tests, in some cases the search for WANConnectionDevice had no results and only a search for InternetGatewayDevice
	// showed up the UPnP root Device which contained the WANConnectionDevice as a child. I'm not sure if there are cases
	// where search for InternetGatewayDevice only would have similar bad effects, but to be sure we do "normal" search first
	// and one for InternetGateWayDevice as fallback
    static const CString strDeviceType1( L"urn:schemas-upnp-org:device:WANConnectionDevice:1");
	static const CString strDeviceType2( L"urn:schemas-upnp-org:device:InternetGatewayDevice:1");

	if (nTCPPort != 0){
		m_nTCPPort = nTCPPort;
		m_nUDPPort = nUDPPort;
		m_nTCPWebPort = nTCPWebPort;
	}

	m_bSecondTry = bSecondTry;
    StopAsyncFind();		// If AsyncFind is in progress, stop it

	m_bPortIsFree = true;
	m_bUPnPPortsForwarded = TRIS_UNKNOWN;
	//ClearDevices();
	//ClearServices();

	// We have to process the AsyncFind
	ProcessAsyncFind( CComBSTR( bSecondTry ? strDeviceType2 : strDeviceType1 ) );

	// We should not release the device finder object
	return;
}

// Helper function for adding devices to the list
// This is called by the devicefinder callback object (DeviceAdded func)
void CUPnPImplWinServ::AddDevice(DevicePointer device, bool bAddChilds, int nLevel)
{
	if (nLevel > 10){
		ASSERT( false );
		return;
	}
	//We are going to add a device 
	CComBSTR bsFriendlyName, bsUniqueName;

	m_tLastEvent = GetTickCount();
	HRESULT hr = device->get_FriendlyName( &bsFriendlyName );

	if ( FAILED( hr ) )
		return (void)UPnPMessage( hr );

	hr = device->get_UniqueDeviceName( &bsUniqueName );

	if ( FAILED( hr ) )
		return (void)UPnPMessage( hr );

	// Add the item at the end of the device list if not found
	std::vector< DevicePointer >::iterator deviceSet
		= std::find_if( m_pDevices.begin(), m_pDevices.end(), FindDevice( bsUniqueName ) );

	if ( deviceSet == m_pDevices.end() )
	{
		m_pDevices.push_back( device );
		DebugLog(_T("Found UPnP device: %s (ChildLevel: %i, UID: %s)"), CString(bsFriendlyName), nLevel, CString(bsUniqueName));
	}

	if (!bAddChilds)
		return;

	// Recursive add any child devices, see comment on StartDiscovery
	IUPnPDevices* piChildDevices = NULL;
	if (SUCCEEDED(device->get_Children(&piChildDevices))){
		IUnknown *pUnk;
		HRESULT hr = piChildDevices->get__NewEnum(&pUnk);
		piChildDevices->Release();
		if (FAILED(hr)) {
			ASSERT( false );
			return;
		}

		IEnumVARIANT *pEnum;
		hr = pUnk->QueryInterface(IID_IEnumVARIANT,(void**)&pEnum);
		pUnk->Release();
		if (FAILED(hr)) {
			ASSERT( false );
			return;
		}		

		VARIANT var;
		ULONG lFetch;
		IDispatch *pDisp;

		VariantInit(&var);
		hr = pEnum->Next(1, &var, &lFetch);
		while(hr == S_OK){
			if (lFetch == 1){
				pDisp = V_DISPATCH(&var);
				DevicePointer pChildDevice;
				pDisp->QueryInterface(IID_IUPnPDevice, (void**)&pChildDevice);
				if (SUCCEEDED(pChildDevice->get_FriendlyName(&bsFriendlyName)) && SUCCEEDED(pChildDevice->get_UniqueDeviceName(&bsUniqueName))){
					AddDevice(pChildDevice, true, nLevel + 1);
				}
			}
			VariantClear(&var);
			hr = pEnum->Next(1, &var, &lFetch);
		};
		pEnum->Release();
	}
}

// Helper function for removing device from the list
// This is called by the devicefinder callback object (DeviceRemoved func)
void CUPnPImplWinServ::RemoveDevice(CComBSTR bsUDN)
{
	DebugLog(_T("Finder asked to remove: %s"), bsUDN );

	std::vector< DevicePointer >::iterator device
		= std::find_if( m_pDevices.begin(), m_pDevices.end(), FindDevice( bsUDN ) );

	if ( device != m_pDevices.end() )
	{
		DebugLog(_T("Device removed: %s"), bsUDN);
		m_pDevices.erase( device );
	}
}

bool CUPnPImplWinServ::OnSearchComplete()
{
    ATLTRACE2( atlTraceCOM, 1, L"CUPnPImplWinServ(%p)->OnSearchComplete\n", this );
	
	if ( m_pDevices.empty() )
	{
		if (m_bSecondTry){
			DebugLog(_T("Found no UPnP gateway devices"));
			m_bUPnPPortsForwarded = TRIS_FALSE;
			m_bUPnPDeviceConnected = TRIS_FALSE;
			if (m_bServiceStartedByEmule)
				StopUPnPService();
			SendResultMessage();
		}
		else
			DebugLog(_T("Found no UPnP gateway devices - will retry with different parameters"));
		
		return false; // no devices found
	}
	
	for ( std::size_t pos = 0; pos != m_pDevices.size(); ++pos )
	{
		GetDeviceServices( m_pDevices[ pos ] );
		StartPortMapping();

		if ( ! m_bPortIsFree ) // warn only once
		{
			// Add more descriptive explanation!!!
			DebugLogError(_T("UPnP port mapping failed because the port(s) are already redirected to another IP."));
			break;
		}
	}
	if (m_bUPnPPortsForwarded == TRIS_UNKNOWN){
		m_bUPnPPortsForwarded = TRIS_FALSE;
		if (m_bServiceStartedByEmule)
			StopUPnPService();
		SendResultMessage();
	}
	return true;
}

// Function to populate the service list for the device
HRESULT	CUPnPImplWinServ::GetDeviceServices(DevicePointer pDevice)
{
	if ( pDevice == NULL )
		return E_POINTER;

	HRESULT hr = S_OK;

	m_pServices.clear();
	_com_ptr_t<_com_IIID<IUPnPServices,&IID_IUPnPServices> > pServices_ = NULL;
	if ( FAILED( hr = pDevice->get_Services( &pServices_ ) ) )
		return UPnPMessage( hr ), hr;
	_com_ptr_t<_com_IIID<IUPnPServices,&IID_IUPnPServices> > pServices( pServices_ );

	LONG nCount = 0;
	if ( FAILED( hr = pServices->get_Count( &nCount ) ) )
		return UPnPMessage( hr ), hr;

	if ( nCount == 0 )
	{
		// Should we ask a user to disable auto-detection?
		DebugLog(_T("Found no services for the current UPnP device."));
		return hr;
	}

	UnknownPtr pEU_ = NULL;
	// We have to get a IEnumUnknown pointer
	if ( FAILED( hr = pServices->get__NewEnum( &pEU_ ) ) )
		return UPnPMessage( hr ), hr;

	EnumUnknownPtr pEU( pEU_ );
	if ( pEU == NULL )
		return hr;

	hr = SaveServices( pEU, nCount );
	
	return hr;
}

// Saves services from enumeration to member m_pServices
HRESULT CUPnPImplWinServ::SaveServices(EnumUnknownPtr pEU, const LONG nTotalItems)
{
	HRESULT hr = S_OK;
	CComBSTR bsServiceId;

	for ( LONG nIndex = 0 ; nIndex < nTotalItems ; nIndex++ )
	{
		UnknownPtr punkService_ = NULL;
		hr = pEU->Next( 1, &punkService_, NULL );
		UnknownPtr punkService( punkService_ );

		if ( FAILED( hr ) )
		{
			// Happens with MS ICS sometimes when the device is disconnected, reboot fixes that
			DebugLogError(_T("Traversing the service list of UPnP device failed."));
			return UPnPMessage( hr ), hr;
		}

		// Get a IUPnPService pointer to the service just got
		ServicePointer pService( punkService );

		if ( FAILED( hr = pService->get_Id( &bsServiceId ) ) )
			return UPnPMessage( hr ), hr;

		DebugLog(_T("Found UPnP service: %s"), bsServiceId);
		m_pServices.push_back( pService );
		bsServiceId.Empty();
	}

	return hr;
}

HRESULT CUPnPImplWinServ::MapPort(const ServicePointer& service)
{
	CComBSTR bsServiceId;
	
	HRESULT hr = service->get_Id( &bsServiceId );
	if ( FAILED( hr ) )
		return UPnPMessage( hr );

	CString strServiceId( bsServiceId );

	if ( m_bADSL ) // not a very reliable way to detect ADSL, since WANEthLinkC* is optional
	{
		if ( m_bUPnPPortsForwarded == TRIS_TRUE ) // another physical device or the setup was ran again manually
		{
			// Reset settings and recheck ( is there a better solution? )
			m_bDisableWANIPSetup = false;
			m_bDisableWANPPPSetup = false;
			m_bADSL = false;
			m_ADSLFailed = false;
		}
		else if ( !m_ADSLFailed )
		{
			DebugLog(_T("ADSL device detected. Disabling WANIPConn setup..."));
			m_bDisableWANIPSetup = true;
			m_bDisableWANPPPSetup = false;
		}
	}
	
	// We expect that the first device in ADSL routers is WANEthLinkC.
	// The problem is that it's unclear if the order of services is always the same...
	// But looks like it is.
	if ( !m_bADSL )
	{
		m_bADSL = !( strServiceId.Find( L"urn:upnp-org:serviceId:WANEthLinkC" ) == -1 ) ||
				  !( strServiceId.Find( L"urn:upnp-org:serviceId:WANDSLLinkC" ) == -1 );
	}

	bool bPPP = stristr(strServiceId, L"urn:upnp-org:serviceId:WANPPPC") != NULL;
	bool bIP  = stristr(strServiceId, L"urn:upnp-org:serviceId:WANIPC")  != NULL;

	if ( ((thePrefs.GetSkipWANPPPSetup() || m_bDisableWANPPPSetup) && bPPP) ||
		 ((thePrefs.GetSkipWANIPSetup() || m_bDisableWANIPSetup) && bIP) ||
		 !bPPP && !bIP )
		return S_OK;

	// For ICS we can query variables, for router devices we need to use
	// actions to get the ConnectionStatus state variable; recommended to use actions
	// "GetStatusInfo" returns state variables: 
	//		|ConnectionStatus|LastConnectionError|Uptime|

	CString strResult;
	hr = InvokeAction( service, L"GetStatusInfo", NULL, strResult );

	if ( strResult.IsEmpty() )
		return hr;

	DebugLog(_T("Got status info from the service %s: %s"), strServiceId, strResult );

	if ( stristr( strResult, L"|VT_BSTR=Connected|" ) != NULL )
	{
		// Add a callback to detect device status changes
		// ??? How it will work if two devices are active ???
		hr = service->AddCallback( m_pServiceCallback );
		if ( FAILED( hr ) ) 
			UPnPMessage( hr );
		else
			DebugLog(_T("Callback added for the service %s"), strServiceId );

		// Delete old and add new port mappings
		m_sLocalIP = GetLocalRoutableIP( service );
		if ( ! m_sLocalIP.IsEmpty() )
		{
			DeleteExistingPortMappings( service );
			CreatePortMappings( service );
			m_bUPnPDeviceConnected = TRIS_TRUE;
		}
	}
	else if ( stristr( strResult, L"|VT_BSTR=Disconnected|" ) != NULL && m_bADSL && bPPP )
	{
		DebugLog(_T("Disconnected PPP service in ADSL device..."));
		m_bDisableWANIPSetup = false;
		m_bDisableWANPPPSetup = true;
		m_ADSLFailed = true;
	}
	else if ( stristr( strResult, L"|VT_BSTR=Disconnected|" ) != NULL && m_bADSL && bIP )
	{
		DebugLog(_T("Disconnected IP service in ADSL device..."));
		m_bDisableWANIPSetup = true;
		m_bDisableWANPPPSetup = false;
		m_ADSLFailed = true;
	}
	return S_OK;
}

void CUPnPImplWinServ::StartPortMapping()
{
   std::vector< ServicePointer >::iterator Iter;
   for ( Iter =  m_pServices.begin( ) ; Iter !=  m_pServices.end( ) ; Iter++ )
	   MapPort(*Iter);

   if (m_bADSL && !m_ADSLFailed && m_bUPnPPortsForwarded == TRIS_UNKNOWN && !thePrefs.GetSkipWANIPSetup()){
		m_ADSLFailed = true;
		DebugLog(_T("ADSL device configuration failed. Retrying with WANIPConn setup..."));
		m_bDisableWANIPSetup = false;
		m_bDisableWANPPPSetup = true;
		for ( Iter =  m_pServices.begin( ) ; Iter !=  m_pServices.end( ) ; Iter++ )
			MapPort(*Iter);
   }
}

void CUPnPImplWinServ::DeletePorts()
{
   if (!m_bInited)
	   return;
   std::vector< ServicePointer >::iterator Iter;
   for ( Iter =  m_pServices.begin( ) ; Iter !=  m_pServices.end( ) ; Iter++ ){
	   if ((ServicePointer)*Iter != NULL)
		   DeleteExistingPortMappings(*Iter);
   }
}

// Finds a local IP address routable from UPnP device
CString CUPnPImplWinServ::GetLocalRoutableIP(ServicePointer pService)
{
	CString strExternalIP;
	HRESULT hr = InvokeAction( pService, L"GetExternalIPAddress", NULL, strExternalIP );
	int nEqualPos = strExternalIP.Find( '=' );
	strExternalIP = strExternalIP.Mid( nEqualPos + 1 ).Trim( '|' );

	if ( FAILED( hr ) || strExternalIP.IsEmpty() )
		return CString();

	CT2CA pszExternalIP(strExternalIP);
	DWORD nInterfaceIndex = 0;
	DWORD ip = inet_addr( pszExternalIP );

	// Get the interface through which the UPnP device has a route
	HRESULT hrRes = (HRESULT)-1;
	if (m_pfGetBestInterface != NULL){
		try {	// just to be sure; another call from iphlpapi used earlier in eMule seemed to crash on some systems according to dumps
			hrRes = m_pfGetBestInterface( ip, &nInterfaceIndex );
		}
		catch(...) {
			ASSERT( false );
		}
	}
	if ( ip == INADDR_NONE || hrRes != NO_ERROR ) 
		return CString();

	MIB_IFROW ifRow;
	ZeroMemory(&ifRow, sizeof(MIB_IFROW));
	ifRow.dwIndex = nInterfaceIndex;
	hrRes = (HRESULT)-1;
	if (m_pfGetIfEntry != NULL){
		try {	// just to be sure; another call from iphlpapi used earlier in eMule seemed to crash on some systems according to dumps
			hrRes = m_pfGetIfEntry( &ifRow );
		}
		catch(...) {
			ASSERT( false );
		}
	}
	if ( hrRes != NO_ERROR )
		return CString();

	// Take an IP address table
	char mib[ sizeof(MIB_IPADDRTABLE) + 32 * sizeof(MIB_IPADDRROW) ];
	ULONG nSize = sizeof(mib);
	PMIB_IPADDRTABLE ipAddr = (PMIB_IPADDRTABLE)mib;

	hrRes = (HRESULT)-1;
	if (m_pfGetIpAddrTable != NULL){
		try {	// just to be sure; another call from iphlpapi used earlier in eMule seemed to crash on some systems according to dumps
			hrRes = m_pfGetIpAddrTable( ipAddr, &nSize, FALSE );
		}
		catch(...) {
			ASSERT( false );
		}
	}
	if ( hrRes != NO_ERROR )
		return CString();

	DWORD nCount = ipAddr->dwNumEntries;
	CString strLocalIP;

	// Look for IP associated with the interface in the address table
	// Loopback addresses are functional for ICS? (at least Windows maps them fine)
	for ( DWORD nIf = 0 ; nIf < nCount ; nIf++ )
	{
		if ( ipAddr->table[ nIf ].dwIndex == nInterfaceIndex )
		{
			ip = ipAddr->table[ nIf ].dwAddr;
			strLocalIP.Format( L"%d.%d.%d.%d", ( ip & 0x0000ff ),
                ( ( ip & 0x00ff00 ) >> 8 ), ( ( ip & 0xff0000 ) >> 16 ),
                ( ip >> 24 ) );
			break;
		}
	}

	if ( ! strLocalIP.IsEmpty() && ! strExternalIP.IsEmpty() )
		DebugLog(_T("UPnP route: %s->%s"), strLocalIP, strExternalIP);

	return strLocalIP;
}

// Walks through all port mappings and searches for "eMule" string.
// Deletes when it has the same IP as local, otherwise quits and sets 
// m_bPortIsFree to false after 10 attempts to use a random port; 
// this member will be used to determine if we have to create new port maps.
void CUPnPImplWinServ::DeleteExistingPortMappings(ServicePointer pService)
{
	// Port mappings are numbered starting from 0 without gaps between;
	// So, we will loop until we get an empty string or failure as a result.
	CString strInArgs;
	USHORT nEntry = 0; // PortMappingNumberOfEntries is of type VT_UI2
	if ( m_sLocalIP.IsEmpty() )
		return;

	HRESULT hr = S_OK;
	//int nAttempts = 10;
					
	// ICS returns computer name instead of IP, thus we need to compare not IPs
	TCHAR szComputerName[ MAX_COMPUTERNAME_LENGTH + 1] = {0};
	DWORD nMaxLen = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerName( szComputerName, &nMaxLen );
	
	CString strActionResult;
	do
	{
		HRESULT hrDel = (HRESULT)-1;	
		strInArgs.Format( _T("|VT_UI2=%hu|"), nEntry );
		hr = InvokeAction( pService, 
			 L"GetGenericPortMappingEntry", strInArgs, strActionResult );
		
		if ( SUCCEEDED( hr ) && ! strActionResult.IsEmpty() )
		{
			// It returned in the following format and order:
			//
			// VT_BSTR	RemoteHost = "" (i.e. any)
			// VT_UI2	ExternalPort = 6346
			// VT_BSTR	PortMappingProtocol = "TCP"
			// VT_UI2	InternalPort = 6346
			// VT_BSTR	InternalClient = "192.168.0.1"
			// VT_BOOL	PortMappingEnabled = True
			// VT_BSTR	PortMappingDescription = "eMule TCP"
			// VT_UI4	PortMappingLeaseDuration = 0 (i.e. any)
			
			// DeletePortMapping action takes 3 arguments: 
			//		RemoteHost, ExternalPort and PortMappingProtocol

			CString strHost, strPort, strProtocol, strLocalIP;

			CArray< CString > oTokens;
			int nPos = 0;
			CString strToken = strActionResult.Tokenize(L"|", nPos);
			while (!strToken.IsEmpty())
			{
				oTokens.Add(strToken);
				strToken = strActionResult.Tokenize(L"|", nPos);
			}

			if ( oTokens.GetCount() != 8 ){
				DebugLogWarning(_T("GetGenericPortMappingEntry delivered mailformed response: '%s'"), strActionResult);
				break;
			}

			if ( stristr( strActionResult, L"|VT_BSTR=eMule TCP|" ) != NULL ||
				stristr( strActionResult, L"|VT_BSTR=eMule UDP|" ) != NULL )
			{


				strHost		= _T('|') + oTokens[ 0 ];
				strPort		= _T('|') + oTokens[ 1 ];
				strProtocol	= _T('|') + oTokens[ 2 ] + _T('|');

				// verify types
				if ( stristr( strHost, L"VT_BSTR" ) == NULL
						|| stristr( strPort, L"VT_UI2" ) == NULL
						|| stristr( strProtocol, L"VT_BSTR" ) == NULL )
					break;

				if ( _tcsstr( oTokens[ 4 ], m_sLocalIP ) != NULL || 
					 stristr( oTokens[ 4 ], szComputerName ) != NULL )
				{
					CString str;
					hrDel = InvokeAction( pService, L"DeletePortMapping", 
						strHost + strPort + strProtocol, str );
					if ( FAILED( hrDel ) ){
						UPnPMessage( hrDel );
					}
					else
					{
						DebugLog(_T("Old port mapping deleted: %s"), strPort + strProtocol );
					}
				}
				else // different IP found in the port mapping entry
				{
					DebugLog(_T("Port %s is used by %s"), (LPCTSTR)oTokens[ 1 ], (LPCTSTR)oTokens[ 4 ] );
					CString strUDPPort, strTCPPort, strTCPWebPort;
					strUDPPort.Format(_T("|VT_UI2=%u"), m_nUDPPort);
					strTCPPort.Format(_T("|VT_UI2=%u"), m_nTCPPort);
					strTCPWebPort.Format(_T("|VT_UI2=%u"), m_nTCPWebPort);
					if ((strTCPPort.CompareNoCase(strPort) == 0 && strProtocol.CompareNoCase(_T("|VT_BSTR=TCP|")) == 0)
						|| (strUDPPort.CompareNoCase(strPort) == 0 && strProtocol.CompareNoCase(_T("|VT_BSTR=UDP|")) == 0)
						|| (strTCPWebPort.CompareNoCase(strPort) == 0 && strProtocol.CompareNoCase(_T("|VT_BSTR=TCP|")) == 0))
					{
						m_bPortIsFree = false;
					}
				}
			}
		}

		if ( FAILED( hrDel ) )
			nEntry++; // Entries are pushed from bottom to top after success
		
		if (nEntry > 30){
			// FIXME for next release
			// this is a sanitize check, since some routers seem to reponse to invalid GetGenericPortMappingEntry numbers
			// proper way would be to get the actualy portmapping count, but needs testing before
			DebugLogError(_T("GetGenericPortMappingEntry maximal count exceeded, quiting"));
			break;
		}
	}
	while ( SUCCEEDED( hr ) && !strActionResult.IsEmpty());
}

// Creates TCP and UDP port mappings
void CUPnPImplWinServ::CreatePortMappings(ServicePointer pService)
{
	if ( m_sLocalIP.IsEmpty() || !m_bPortIsFree )
		return;

	CString strPortTCP, strPortTCPWeb, strPortUDP, strInArgs, strFormatString, strResult;
	
	strFormatString = L"|VT_BSTR=|VT_UI2=%s|VT_BSTR=%s|VT_UI2=%s|VT_BSTR=%s|"
		L"VT_BOOL=True|VT_BSTR=eMule %s|VT_UI4=0|";

	strPortTCP.Format( L"%hu", m_nTCPPort );
	strPortTCPWeb.Format( L"%hu", m_nTCPWebPort );
	strPortUDP.Format( L"%hu", m_nUDPPort );

	// First map UDP if some buggy router overwrites TCP on top
	HRESULT hr;
	if (m_nUDPPort != 0){
		strInArgs.Format( strFormatString, strPortUDP, L"UDP", strPortUDP, m_sLocalIP, L"UDP" );
		hr = InvokeAction( pService, L"AddPortMapping", strInArgs, strResult );
		if ( FAILED( hr ) )
			return (void)UPnPMessage( hr );
	}

	strInArgs.Format( strFormatString, strPortTCP, L"TCP", strPortTCP, m_sLocalIP, L"TCP" );
	hr = InvokeAction( pService, L"AddPortMapping", strInArgs, strResult );
	if ( FAILED( hr ) )
		return (void)UPnPMessage( hr );

	if (m_nTCPWebPort != 0){
		strInArgs.Format( strFormatString, strPortTCPWeb, L"TCP", strPortTCPWeb, m_sLocalIP, L"TCP" );
		hr = InvokeAction( pService, L"AddPortMapping", strInArgs, strResult );
		if ( FAILED( hr ) )
			DebugLogWarning(_T("UPnP: WinServImpl: Mapping Port for Webinterface failed, continuing anyway"));
	}

	m_bUPnPPortsForwarded = TRIS_TRUE;
	SendResultMessage();
	
	// Leave the message loop, since events may take more time.
	// Assuming that the user doesn't use several devices
	
	m_bAsyncFindRunning = false;
}

// Invoke the action for the selected service.
// OUT arguments or return value is packed in strResult.
HRESULT CUPnPImplWinServ::InvokeAction(ServicePointer pService, 
	CComBSTR action, LPCTSTR pszInArgString, CString& strResult) 
{
	if ( pService == NULL || action == NULL )
		return E_POINTER;

	m_tLastEvent = GetTickCount();
	CString strInArgs;
	strInArgs.SetString( pszInArgString ? pszInArgString : _T("") );

	HRESULT hr = S_OK;

	CComVariant	vaActionArgs, vaArray, vaOutArgs, vaRet;
	VARIANT**  ppVars = NULL;
	SAFEARRAY* psaArgs = NULL;
	LONG nPos = 0;

	INT_PTR nArgs = CreateVarFromString( strInArgs, &ppVars );
	if ( nArgs < 0 ) return E_FAIL;

	hr = CreateSafeArray( VT_VARIANT, (ULONG)nArgs, &psaArgs );
	if ( FAILED( hr ) ) return hr;

	vaArray.vt = VT_VARIANT | VT_ARRAY | VT_BYREF;
	vaArray.pparray = &psaArgs;

	vaActionArgs.vt = VT_VARIANT | VT_BYREF;
	vaActionArgs.pvarVal = &vaArray;

	vaArray.pparray = &psaArgs;

	for( INT_PTR nArg = 0 ; nArg < nArgs ; nArg++ )
	{
		nPos = nArg + 1;
		(void)SafeArrayPutElement( psaArgs, &nPos, ppVars[ nArg ] );
	}

	hr = pService->InvokeAction( action, vaActionArgs, &vaOutArgs, &vaRet);

	if ( SUCCEEDED( hr ) )
	{
		// In connection services return value is empty 
		// when OUT arguments are returned
		if ( vaRet.vt != VT_EMPTY )
		{
			bool bInvalid = false;

			if ( vaRet.vt == VT_BSTR )
				strResult = L"|VT_BSTR=";
			else if ( vaRet.vt == VT_UI2 )
				strResult = L"|VT_UI2=";
			else if ( vaRet.vt == VT_UI4 )
				strResult = L"|VT_UI4=";
			else if ( vaRet.vt == VT_BOOL )
				strResult = L"|VT_BOOL=";
			else
				bInvalid = true;

			if ( ! bInvalid )
			{
				hr = VariantChangeType( &vaRet, &vaRet, VARIANT_ALPHABOOL, VT_BSTR );
				if ( SUCCEEDED( hr ) )
				{
					CString str( vaRet.bstrVal );
					strResult += str;
					strResult += L"|";
				}
				else strResult.Empty();
			}
		}
		else
			GetStringFromOutArgs( &vaOutArgs, strResult );
	}

	if ( ppVars != NULL ) DestroyVars( nArgs, &ppVars ); 
	if ( psaArgs != NULL ) SafeArrayDestroy( psaArgs );

	return hr;
}

// Creates a SafeArray
// vt--VariantType
// nArgs--Number of Arguments
// ppsa--Created safearray
HRESULT CUPnPImplWinServ::CreateSafeArray(const VARTYPE vt, const ULONG nArgs, SAFEARRAY** ppsa)
{
    SAFEARRAYBOUND aDim[ 1 ]; 

	if ( nArgs == 0 )
	{
        aDim[ 0 ].lLbound = 0; 
        aDim[ 0 ].cElements = 0; 
    }
    else
	{
        aDim[ 0 ].lLbound = 1; 
        aDim[ 0 ].cElements = nArgs; 
    }
    
    *ppsa = SafeArrayCreate( vt, 1, aDim );
   
	if( NULL == *ppsa ) return E_OUTOFMEMORY;
    return S_OK;
}

// Creates argument variants from the string
// The string format is "|variant_type1=value1|variant_type2=value2|"
// The most common types used for UPnP values are:
//		VT_BSTR, VT_UI2, VT_UI4, VT_BOOL
// Returns: number of arguments or -1 if invalid string/values.

INT_PTR CUPnPImplWinServ::CreateVarFromString(const CString& strArgs, VARIANT*** pppVars)
{
	if ( strArgs.IsEmpty() )
	{
		*pppVars = NULL;
		return 0;
	}
	
	CArray< CString > oTokens;
	CString strToken, strType, strValue;
	BOOL bInvalid = FALSE;

	int nPos = 0;
	CString strTokenize = strArgs.Tokenize(_T("|"), nPos);
	while (!strTokenize.IsEmpty())
	{
		oTokens.Add(strTokenize);
		strTokenize = strArgs.Tokenize(_T("|"), nPos);
	}

	INT_PTR nArgs = oTokens.GetCount();
	*pppVars = new VARIANT* [ nArgs ]();

	for ( INT_PTR nArg = 0 ; nArg < nArgs ; nArg++ )
	{
		strToken = oTokens.GetAt( nArg );
		int nEqualPos = strToken.Find( '=' );
		
		// Malformatted string test
		if ( nEqualPos == -1 ) { bInvalid = TRUE; break; }

		strType.SetString( strToken.Left( nEqualPos ).Trim() );
		strValue.SetString( strToken.Mid( nEqualPos + 1 ).Trim() );

		(*pppVars)[ nArg ] = new VARIANT;
		VariantInit( (*pppVars)[ nArg ] );

		// Assign value
		if ( strType == _T("VT_BSTR") )
		{
			(*pppVars)[ nArg ]->vt = VT_BSTR;
			(*pppVars)[ nArg ]->bstrVal = strValue.AllocSysString();
		}
		else if ( strType == _T("VT_UI2") )
		{
			USHORT nValue = 0;
			bInvalid = _stscanf( strValue, _T("%hu"), &nValue ) != 1;
			if ( bInvalid ) break;

			(*pppVars)[ nArg ]->vt = VT_UI2;
			(*pppVars)[ nArg ]->uiVal = nValue;
		}
		else if ( strType == _T("VT_UI4") )
		{
			ULONG nValue = 0;
			bInvalid = _stscanf( strValue, _T("%lu"), &nValue ) != 1;
			if ( bInvalid ) break;

			(*pppVars)[ nArg ]->vt = VT_UI4;
			(*pppVars)[ nArg ]->ulVal = nValue;
		}
		else if ( strType == _T("VT_BOOL") )
		{
			VARIANT_BOOL va = 1;
			if ( strValue.CompareNoCase( _T("true") ) == 0 )
				va = VARIANT_TRUE;
			else if ( strValue.CompareNoCase( _T("false") ) == 0 )
				va = VARIANT_FALSE;
			else
				bInvalid = TRUE;
			if ( bInvalid ) break;

			(*pppVars)[ nArg ]->vt = VT_BOOL;
			(*pppVars)[ nArg ]->boolVal = va;	
		}
		else
		{
			bInvalid = TRUE; // no other types are supported
			break;
		}
	}

	if ( bInvalid ) // cleanup if invalid
	{
		DestroyVars( nArgs, pppVars );
		return -1;
	}
	return nArgs;
}

// Creates a string in format "|variant_type1=value1|variant_type2=value2|"
// from OUT variant returned by service.
// Returns: number of arguments or -1 if not applicable.

INT_PTR	CUPnPImplWinServ::GetStringFromOutArgs(const VARIANT* pvaOutArgs, CString& strArgs)
{
	LONG nLBound = 0L, nUBound = 0L;
	HRESULT hr = GetSafeArrayBounds( pvaOutArgs->parray, &nLBound, &nUBound );
	bool bInvalid = FAILED( hr );
	CString strResult, strToken;

	if ( ! bInvalid ) // We have got the bounds of the arguments
	{
		CComVariant vaOutElement;
		strResult = '|';

		for ( LONG nIndex = nLBound ; nIndex <= nUBound && ! bInvalid ; ++nIndex )
		{
			vaOutElement.Clear();
			hr = GetVariantElement( pvaOutArgs->parray, nIndex, &vaOutElement );
			
			if ( SUCCEEDED( hr ) )
			{
				if ( vaOutElement.vt == VT_BSTR )
					strToken = L"VT_BSTR=";
				else if ( vaOutElement.vt == VT_UI2 )
					strToken = L"VT_UI2=";
				else if ( vaOutElement.vt == VT_UI4 )
					strToken = L"VT_UI4=";
				else if ( vaOutElement.vt == VT_BOOL )
					strToken = L"VT_BOOL=";
				else
				{
					bInvalid = true;
					break;
				}

				hr = VariantChangeType( &vaOutElement, &vaOutElement, 
							VARIANT_ALPHABOOL, VT_BSTR );
				if ( SUCCEEDED( hr ) )
				{
					CString str( vaOutElement.bstrVal );
					strToken += str;
					strToken += L"|";
					strResult += strToken;
				}
				else bInvalid = true;
			}
			else
				bInvalid = true;
		} // For loop
	}
	
	if ( bInvalid || nLBound > nUBound ) return -1;

	strArgs = strResult;
	return  nUBound - nLBound + 1;
}

// Get SafeArray bounds
HRESULT CUPnPImplWinServ::GetSafeArrayBounds(SAFEARRAY* psa, LONG* pLBound, LONG* pUBound)
{
	ASSERT( psa != NULL );

	HRESULT hr = SafeArrayGetLBound( psa, 1, pLBound );
	if ( FAILED( hr ) )
		return hr;

	return SafeArrayGetUBound( psa, 1, pUBound );
}

// Get Variant Element
// psa--SafeArray; nPosition--Position in the array; pvar--Variant Element being set
HRESULT CUPnPImplWinServ::GetVariantElement(SAFEARRAY* psa, LONG pos, VARIANT* pvar)
{
	ASSERT( psa != NULL );

	return SafeArrayGetElement( psa, &pos, pvar );
}


// Destroys argument variants
void CUPnPImplWinServ::DestroyVars(const INT_PTR nCount, VARIANT*** pppVars)
{
	VARIANT* pVar = NULL;

	ASSERT( pppVars && *pppVars );

	if( nCount == 0 ) return;

	for ( INT_PTR nArg = 0 ; nArg < nCount ; nArg++ )
	{
		pVar = (*pppVars)[ nArg ];
		if ( pVar != NULL )
		{
			VariantClear( pVar );
			delete pVar;
			pVar = NULL;
		}
	}

	delete [] *pppVars;
	*pppVars = NULL;
}

///////////////////////////////////////////////////////////////////
//   CDeviceFinderCallback
///////////////////////////////////////////////////////////////////

// Called when a device is added
// nFindData--AsyncFindHandle; pDevice--COM interface pointer of the device being added
HRESULT CDeviceFinderCallback::DeviceAdded(LONG /*nFindData*/, IUPnPDevice* pDevice)
{
	ATLTRACE2( atlTraceCOM, 1, L"Device Added\n" );
	m_instance.AddDevice(pDevice, true);
	return S_OK;
}

// Called when a device is removed
// nFindData--AsyncFindHandle; bsUDN--UDN of the device being removed
HRESULT CDeviceFinderCallback::DeviceRemoved(LONG /*nFindData*/, BSTR bsUDN)
{
	ATLTRACE2( atlTraceCOM, 1, "Device Removed: %s\n", bsUDN );
	m_instance.RemoveDevice( bsUDN );
	return S_OK;
}

// Called when the search is complete; nFindData--AsyncFindHandle
HRESULT CDeviceFinderCallback::SearchComplete(LONG /*nFindData*/)
{
	// StopAsyncFind must be here, do not move to OnSearchComplete
	// Otherwise, "Service died" message is shown, and it means
	// that the service still was active.
	bool bRetry = !m_instance.OnSearchComplete();
	m_instance.StopAsyncFind();
	if (bRetry)
		m_instance.StartDiscovery(0, 0, 0, true);
	return S_OK;
}

HRESULT CDeviceFinderCallback::QueryInterface(REFIID iid, LPVOID* ppvObject){
	HRESULT hr = S_OK;
	if(NULL == ppvObject){
		hr = E_POINTER;
	}
	else
		*ppvObject = NULL;

	if(SUCCEEDED(hr)){
		if(IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IUPnPDeviceFinderCallback)){
			*ppvObject = static_cast<IUPnPDeviceFinderCallback*>(this);
			AddRef();
		}
		else
			hr = E_NOINTERFACE;
	}
	return hr;
};

ULONG CDeviceFinderCallback::AddRef(){
	return ::InterlockedIncrement(&m_lRefCount);
};

ULONG CDeviceFinderCallback::Release(){
	LONG lRefCount = ::InterlockedDecrement(&m_lRefCount);
	if(0 == lRefCount)
		delete this;
	return lRefCount;
};

////////////////////////////////////////////////////////////////////////////////
//   CServiceCallback
////////////////////////////////////////////////////////////////////////////////

//! Called when the state variable is changed
//! \arg pus             COM interface pointer of the service;
//! \arg pszStateVarName State Variable Name;
//! \arg varValue        State Variable Value
HRESULT CServiceCallback::StateVariableChanged(IUPnPService* pService, 
			LPCWSTR pszStateVarName, VARIANT varValue)
{
	CComBSTR bsServiceId;
	m_instance.m_tLastEvent = GetTickCount();

	HRESULT hr = pService->get_Id( &bsServiceId );
	if ( FAILED( hr ) )
		return UPnPMessage( hr );
	if ( FAILED( hr = VariantChangeType( &varValue, &varValue, VARIANT_ALPHABOOL, VT_BSTR ) ) )
		return UPnPMessage( hr );

	CString strValue( varValue.bstrVal );

	// Re-examine state variable change only when discovery was finished
	// We are not interested in the initial values; we will request them explicitly
	if ( !m_instance.IsAsyncFindRunning() )
	{
		if ( _wcsicmp( pszStateVarName, L"ConnectionStatus" ) == 0 )
		{
			m_instance.m_bUPnPDeviceConnected = strValue.CompareNoCase( L"Disconnected" ) == 0
					? TRIS_FALSE
					: ( strValue.CompareNoCase( L"Connected" ) == 0 )
						? TRIS_TRUE
						: TRIS_UNKNOWN;
		}
	}

	DebugLog(_T("UPnP device state variable %s changed to %s in %s"),
		pszStateVarName, strValue.IsEmpty()? L"NULL" : strValue.GetString(), bsServiceId.m_str );

	return hr;
}

//! Called when the service dies
HRESULT CServiceCallback::ServiceInstanceDied(IUPnPService* pService)
{
	CComBSTR bsServiceId;

	HRESULT hr = pService->get_Id( &bsServiceId );
	if ( SUCCEEDED( hr ) )
	{
		DebugLogError(_T("UPnP service %s died"), bsServiceId );
		return hr;
	}

	return UPnPMessage( hr );
}

HRESULT CServiceCallback::QueryInterface(REFIID iid, LPVOID* ppvObject)
{
	HRESULT hr = S_OK;
	if(NULL == ppvObject){
		hr = E_POINTER;
	}
	else
		*ppvObject = NULL;

	if(SUCCEEDED(hr)){
		if(IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IUPnPServiceCallback)){
			*ppvObject = static_cast<IUPnPServiceCallback*>(this);
			AddRef();
		}
		else
			hr = E_NOINTERFACE;
	}
	return hr;
};

ULONG CServiceCallback::AddRef(){
	return ::InterlockedIncrement(&m_lRefCount);
};

ULONG CServiceCallback::Release(){
	LONG lRefCount = ::InterlockedDecrement(&m_lRefCount);
	if(0 == lRefCount)
		delete this;
	return lRefCount;
};

////////////////////////////////////////////////////////////////////////////////
// Prints the appropriate UPnP error text

CString translateUPnPResult(HRESULT hr)
{
	static std::map<HRESULT, std::string> messages;

	if ( hr >= UPNP_E_ACTION_SPECIFIC_BASE && hr <= UPNP_E_ACTION_SPECIFIC_MAX )
		return _T("Action Specific Error");

	messages[ 0 ] = "";
	messages[ UPNP_E_ROOT_ELEMENT_EXPECTED ] =      "Root Element Expected";
	messages[ UPNP_E_DEVICE_ELEMENT_EXPECTED ] =    "Device Element Expected";
	messages[ UPNP_E_SERVICE_ELEMENT_EXPECTED ] =   "Service Element Expected";
	messages[ UPNP_E_SERVICE_NODE_INCOMPLETE ] =    "Service Node Incomplete";
	messages[ UPNP_E_DEVICE_NODE_INCOMPLETE ] =     "Device Node Incomplete";
	messages[ UPNP_E_ICON_ELEMENT_EXPECTED ] =      "Icon Element Expected";
	messages[ UPNP_E_ICON_NODE_INCOMPLETE ] =       "Icon Node Incomplete";
	messages[ UPNP_E_INVALID_ACTION ] =             "Invalid Action";
	messages[ UPNP_E_INVALID_ARGUMENTS ] =          "Invalid Arguments";
	messages[ UPNP_E_OUT_OF_SYNC ] =                "Out of Sync";
	messages[ UPNP_E_ACTION_REQUEST_FAILED ] =      "Action Request Failed";
	messages[ UPNP_E_TRANSPORT_ERROR ] =            "Transport Error";
	messages[ UPNP_E_VARIABLE_VALUE_UNKNOWN ] =     "Variable Value Unknown";
	messages[ UPNP_E_INVALID_VARIABLE ] =           "Invalid Variable";
	messages[ UPNP_E_DEVICE_ERROR ] =               "Device Error";
	messages[ UPNP_E_PROTOCOL_ERROR ] =             "Protocol Error";
	messages[ UPNP_E_ERROR_PROCESSING_RESPONSE ] =  "Error Processing Response";
	messages[ UPNP_E_DEVICE_TIMEOUT ] =             "Device Timeout";
	messages[ UPNP_E_INVALID_DOCUMENT ] =           "Invalid Document";
	messages[ UPNP_E_EVENT_SUBSCRIPTION_FAILED ] =  "Event Subscription Failed";
	messages[ E_FAIL ] =                            "Generic failure";

	return CString(messages[ hr ].c_str());
}

HRESULT UPnPMessage(HRESULT hr)
{
	CString strError = translateUPnPResult( hr );
	if ( ! strError.IsEmpty() )
		DebugLogWarning(_T("upnp: ") + strError );
	return hr;
}
