//
//  CLanguageSupport : Language selection support and resource DLLs management.

#include "stdafx.h"
#include "resource.h"
#include "LanguageSupport.h"
#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Forces automatic link of version.lib
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "version.lib")

// Private data
const TCHAR CLanguageSupport::szLanguageId[]=_T("LanguageId");

#ifndef countof
#define countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

//
//  LoadBestLanguage : Loads the resource DLL matching the LanguageID stored in the registry (app settings).
//    If the DLL is not found, we attempt to load the resource DLL that best matches user preferences.
//
void CLanguageSupport::LoadBestLanguage()
{ 
  // Be sure the registry key was set prior to calling this function
  // If this ASSERT fires, it means that you didn't call SetRegistryKey() before calling this function
  ASSERT(AfxGetApp()->m_pszRegistryKey!=NULL && AfxGetApp()->m_pszRegistryKey[0]!=0);

  // Load previously selected language (if any)
  m_nCurrentLanguage=(LANGID)AfxGetApp()->GetProfileInt(_T(""),szLanguageId,0);
  if (LoadLanguage())
    return; // Language loaded successfully.

  // No previous selection -> Attempt to load the same language as the Windows UI language for the current user (MUI only)
  m_nCurrentLanguage= GetUserUILanguage();
  if (LoadLanguage())
    return; // Language loaded successfully.

  // Failed again -> Try Windows default UI language
  m_nCurrentLanguage= GetSystemUILanguage();
  if (LoadLanguage())
    return; // Language loaded successfully.

  // Failed again -> Try user's preferred user language (not the Windows UI language)
  m_nCurrentLanguage= GetUserDefaultLangID();
  if (LoadLanguage())
    return; // Language loaded successfully.

  // Failed again -> Try system's preferred language for non-Unicode programs (not the Windows UI language)
  m_nCurrentLanguage= GetSystemDefaultLangID();
  if (LoadLanguage())
    return; // Language loaded successfully.

  // Failed again -> Keep currentlanguage (exe file)
  m_nCurrentLanguage= m_nExeLanguage;
  VERIFY(LoadLanguage()); // Cannot fail since there's no actual DLL loading. 
                          // The call is necessary to make sure we don't stay with a previously loaded DLL.
}

bool CLanguageSupport::LoadLanguage()
{ // Try to load the language m_nCurrentLanguage.
  // If it doesn't exist, try to fallback on the neutral or default sublanguage for this language.

  if (m_nCurrentLanguage==0)
    return false; // Language not specified !

  if (LoadLanguageDll())
    return true; // Language loaded successfully.

  // Failed -> Try to load the corresponding neutral language
  WORD wSubLanguage= SUBLANGID(m_nCurrentLanguage);
  if (wSubLanguage!=SUBLANG_NEUTRAL)
  {
    m_nCurrentLanguage= MAKELANGID( PRIMARYLANGID(m_nCurrentLanguage), SUBLANG_NEUTRAL );
    if (LoadLanguageDll())
      return true; // Language loaded successfully.
  }

  // Failed again -> Look for 'main' sublanguage. e.g.: German (Swiss) would fallback on German (Germany)
  if (wSubLanguage!=SUBLANG_DEFAULT)
  {
    m_nCurrentLanguage= MAKELANGID( PRIMARYLANGID(m_nCurrentLanguage), SUBLANG_DEFAULT );
    if (LoadLanguageDll())
      return true; // Language loaded successfully.
  }

  // Failed again: We don't have any DLL matching this language
  return false;
}

typedef LANGID (WINAPI*PFNGETUSERDEFAULTUILANGUAGE)();
typedef LANGID (WINAPI*PFNGETSYSTEMDEFAULTUILANGUAGE)();

#if _MFC_VER<0x0700 // for VC6 users. Depending on your version of the platform SDK, you may need these lines or not.
typedef LPARAM LONG_PTR;
#endif

static BOOL CALLBACK _EnumResLangProc(HMODULE /*hModule*/, LPCTSTR /*pszType*/, 
	LPCTSTR /*pszName*/, WORD langid, LONG_PTR lParam)
{ // Helper used to identify the language in NTDLL.DLL (used for NT4)

	if(lParam == NULL) // lParam = ptr to var that receives the LangId
		return FALSE;
		
	LANGID* plangid = reinterpret_cast< LANGID* >( lParam );
	*plangid = langid;

	return TRUE;
}

LANGID CLanguageSupport::GetUserUILanguage()
{ // On MUI systems, get the (current) user-specific Windows UI language (On non MUI systems, returns 0)
  HINSTANCE hKernel32= ::GetModuleHandle(_T("kernel32.dll"));
	ASSERT(hKernel32 != NULL);
	
  PFNGETUSERDEFAULTUILANGUAGE pfnGetUserDefaultUILanguage = 
    (PFNGETUSERDEFAULTUILANGUAGE)::GetProcAddress(hKernel32, "GetUserDefaultUILanguage"); // NB: GetProcAddress() takes an ANSI string

	if(pfnGetUserDefaultUILanguage != NULL)
	{ // This is a MUI-enabled system (Win2000+) -> Get user's UI language
		return pfnGetUserDefaultUILanguage();
  }
	else
	{	// We're not on an MUI-capable system : NT4 or Win9x
    return 0;
  }
}

LANGID CLanguageSupport::GetSystemUILanguage()
{ // On MUI systems, get the default Windows UI language
  // On non MUI systems, returns the Windows UI language (there's only one)

  // Note: This code is inspired of AfxLoadLangResourceDLL() (see MFC source : appcore.cpp)

  HINSTANCE hKernel32= ::GetModuleHandle(_T("kernel32.dll"));
	ASSERT(hKernel32 != NULL);
	
  PFNGETSYSTEMDEFAULTUILANGUAGE pfnGetSystemDefaultUILanguage = 
    (PFNGETSYSTEMDEFAULTUILANGUAGE)::GetProcAddress(hKernel32, "GetSystemDefaultUILanguage"); // NB: GetProcAddress() takes an ANSI string

	if(pfnGetSystemDefaultUILanguage != NULL)
	{ // This is a MUI-enabled system (Win2000+) -> Get user's UI language
		return pfnGetSystemDefaultUILanguage();
  }
	else
	{	// We're not on an MUI-capable system : NT4, Win9x
		if (::GetVersion()&0x80000000)
		{	// We're on Windows 9x, so look in the registry for the UI language
	    DWORD dwLangID= 0; // Assume error

			HKEY hKey = NULL;
			LONG nResult = ::RegOpenKeyEx(HKEY_CURRENT_USER, 
				_T( "Control Panel\\Desktop\\ResourceLocale" ), 0, KEY_READ, &hKey);

			if (nResult == ERROR_SUCCESS)
			{
				DWORD dwType;
				TCHAR szValue[16];
				ULONG nBytes = sizeof( szValue );
				nResult = ::RegQueryValueEx(hKey, NULL, NULL, &dwType, LPBYTE( szValue ), &nBytes );
				if (nResult==ERROR_SUCCESS && dwType==REG_SZ)
					_stscanf( szValue, _T( "%x" ), &dwLangID );

				::RegCloseKey(hKey);
			}

      return (LANGID)dwLangID;
		}
		else
		{	// We're on NT 4.  The UI language is the same as the language of the version resource in ntdll.dll
		  LANGID LangId = 0;
			HMODULE hNTDLL = ::GetModuleHandle( _T( "ntdll.dll" ) );
			if (hNTDLL != NULL)
			{
				::EnumResourceLanguages( hNTDLL, RT_VERSION, MAKEINTRESOURCE( 1 ), 
					_EnumResLangProc, reinterpret_cast< LONG_PTR >( &LangId ) );
			}

      return LangId;
		}
	}
}

bool CLanguageSupport::LoadLanguageDll()
{ // Identifies and loads the resource DLL for language m_nCurrentLanguage

  // Do we really have to load a satellite DLL ?
  if (m_nCurrentLanguage==m_nExeLanguage)
  { // No. The selected language is stored in the exe. Make sure the resources are loaded from the exe.
    // (in case a previous call 
    TRACE0("Resource DLL is... the EXE.\n");
    UnloadResourceDll(); // Unloads possibly currently loaded Resource DLL
    return true; 
  }

  // Get abbreviation for lang name
  TCHAR szAbbrevName[4];
  if (GetLocaleInfo(MAKELCID(m_nCurrentLanguage, SORT_DEFAULT), LOCALE_SABBREVLANGNAME, szAbbrevName, 4)==0)
  { // Invalid language
    TRACE1("Attempt to load DLL for unsupported language. Language = %u\n", m_nCurrentLanguage);
    return false; 
  }

  // Build the DLL filename
  TCHAR szFilename[MAX_PATH];
  DWORD cch= GetModuleFileName( NULL, szFilename, MAX_PATH);
  ASSERT(cch!=0);

  LPTSTR pszExtension= PathFindExtension(szFilename);
  lstrcpy(pszExtension, szAbbrevName);
  lstrcat(pszExtension, _T(".dll"));

  // Load DLL
  HINSTANCE hDll = LoadLibrary(szFilename);
  if (hDll != NULL)
  { // OK.
    TRACE1("Resource DLL %s loaded successfully\n",szFilename);
    UnloadResourceDll(); // Unloads possibly currently loaded Resource DLL

    // Set loaded DLL as default resource container
    m_hDll= hDll;
    SetResourceHandle(m_hDll);	

    return true;
  }
  else
    return false; // DLL does not exist.
}

CLanguageSupport::CLanguageSupport()
{
  m_hDll= NULL;
  m_nCurrentLanguage= 0;
  LookupExeLanguage();
}

CLanguageSupport::~CLanguageSupport()
{
  UnloadResourceDll();
}

void CLanguageSupport::UnloadResourceDll()
{
  if (m_hDll!=NULL)
  {
    SetResourceHandle(AfxGetApp()->m_hInstance); // Restores the EXE as the resource container.
    FreeLibrary(m_hDll);
    m_hDll= NULL;
  }
}

void CLanguageSupport::SetResourceHandle(HINSTANCE hDll)
{ // Tells MFC where the resources are stored.
  AfxSetResourceHandle(hDll);

#if _MFC_VER>=0x0700
  _AtlBaseModule.SetResourceInstance(hDll);
#endif
}

//
//  GetLangIdFromFile : Get the language ID and abbreviated name of the version info
//    block stored in file pszFilename.
//
//  Returns : The language ID (0 = failure) + Abbrev name (szLanguageSuffix)
//
LANGID CLanguageSupport::GetLangIdFromFile(LPCTSTR pszFilename)
{
  // Get VersionInfo size
  DWORD dwHandle;
  DWORD dwLength=GetFileVersionInfoSize((LPTSTR)pszFilename,&dwHandle);
  if (dwLength==0)
  {
    TRACE(_T("Failed to read file's version info. Error= %1!u!\n"), GetLastError());
    return 0; // Not a valid DLL
  }

  // Get VersionInfo data
  LANGID nLangId=0; // Assume failure
  CByteArray abData;
  abData.SetSize(dwLength);

  if (GetFileVersionInfo((LPTSTR)pszFilename,dwHandle,dwLength,(LPVOID)abData.GetData()))
  { // Extract Language Id from version info
    LANGID *pLanguageId; // NB: LANGID = WORD
    if (VerQueryValue(abData.GetData(),_T("\\VarFileInfo\\Translation"),(void**)&pLanguageId,(PUINT)&dwLength))
      nLangId=*pLanguageId;
  }

  return nLangId;
}

void CLanguageSupport::OnSwitchLanguage(UINT nId, bool bLoadNewLanguageImmediately)
{
  // Get language for this menu item
  int nLanguageIndex= nId-ID_LANGUAGE_FIRST;

  if (nLanguageIndex<0 || nLanguageIndex>=m_aLanguages.GetSize())
    return; // invalid id

  LANGID LangId= m_aLanguages[nLanguageIndex];

  // Write new language id into the registry
  AfxGetApp()->WriteProfileInt(_T(""),szLanguageId,(int)LangId); 

  if (bLoadNewLanguageImmediately)
  { // On-the-fly language switch
    LoadBestLanguage();
  }
  else
  { // No on-the-fly switch (default behaviour) : Tells user to restart
    LANGID nCurrentLanguage= m_nCurrentLanguage;

    // Load string from new language dll
    m_nCurrentLanguage= LangId;
    VERIFY(LoadLanguage());

    CString csFormat(MAKEINTRESOURCE(IDS_RESTART));    // Don't forget to add a string in the String Table :

    // Restore current language
    m_nCurrentLanguage= nCurrentLanguage;
    VERIFY(LoadLanguage());

    // Display message box
    CString csMessage;
    csMessage.FormatMessage(csFormat, LPCTSTR(AfxGetAppName()));  // IDS_RESTART : Please restart %1.
    AfxMessageBox(csMessage,MB_ICONINFORMATION); // Please restart MyApp.
  }
}

void CLanguageSupport::GetAvailableLanguages()
{ // Fills m_aLanguages with the list of languages available (as resource DLLs + the language of the exe)

  // Start with an empty list of languages
  m_aLanguages.SetSize(0);

  // Look up language of exe file
  if (m_nExeLanguage!=0)
    m_aLanguages.Add(m_nExeLanguage);

  // Look for satellites DLL files
  TCHAR szFileMask[MAX_PATH+10];
  DWORD cch= GetModuleFileName( NULL, szFileMask, MAX_PATH);
  ASSERT(cch!=0);
  LPTSTR pszExtension= PathFindExtension(szFileMask);
  lstrcpy(pszExtension, _T("???.dll"));

  CFileFind finder;
  BOOL bWorking = finder.FindFile(szFileMask);
  while (bWorking)
  { // for each satellite DLL...
    bWorking = finder.FindNextFile();

    // Extract the language ID of the DLL and add it to the list of supported languages
    LANGID nLanguageID=GetLangIdFromFile(finder.GetFilePath());
    if (nLanguageID!=0)
      m_aLanguages.Add(nLanguageID);
  }
}

void CLanguageSupport::CreateMenu(CCmdUI *pCmdUI, UINT nFirstItemId)
{
  // Load list of available languages
  GetAvailableLanguages();

  // Create sub-menu with all supported languages
  UINT nCurrentItem= 0;
  CMenu SubMenu;
  SubMenu.CreatePopupMenu();
  for (int i=0; i<m_aLanguages.GetSize(); i++)
  {
    SubMenu.AppendMenu(MF_STRING, ID_LANGUAGE_FIRST+i, GetLanguageName(m_aLanguages[i]));
    if (m_nCurrentLanguage==m_aLanguages[i])
      nCurrentItem= ID_LANGUAGE_FIRST+i;
  }

  // Place a radio-button mark in front of the current selection
  if (nCurrentItem!=0)
    SubMenu.CheckMenuRadioItem(ID_LANGUAGE_FIRST, ID_LANGUAGE_FIRST+(int)m_aLanguages.GetSize()-1, nCurrentItem, MF_BYCOMMAND);
  
  // Insert the submenu behind the given menu item
  MENUITEMINFO mii= { sizeof(mii) };
  mii.fMask= MIIM_SUBMENU;
  mii.hSubMenu= SubMenu.m_hMenu;

  ::SetMenuItemInfo(pCmdUI->m_pMenu->m_hMenu, pCmdUI->m_nID, FALSE, &mii);
  pCmdUI->Enable();

  SubMenu.Detach();
}

CString CLanguageSupport::GetLanguageName(LANGID wLangId)
{
  TCHAR szLanguage[200], szCP[10];

  // Get ANSI codepage for language
  GetLocaleInfo( MAKELCID(wLangId, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szCP, 10);
  int nAnsiCodePage= _ttoi(szCP);

  // Get native name of language
  int cch= GetLocaleInfo( MAKELCID(wLangId, SORT_DEFAULT), LOCALE_SNATIVELANGNAME, szLanguage, countof(szLanguage));
  if (cch!=0)
  { // OK -> If ANSI build -> check if we can write this native name
#ifndef UNICODE
    // Convert native name to Unicode (using native codepage)
    wchar_t szLanguageW[200];
    cch= MultiByteToWideChar(nAnsiCodePage, MB_ERR_INVALID_CHARS, szLanguage, -1, szLanguageW, countof(szLanguageW));

    // Try to convert to the current codepage
    BOOL bUsed= FALSE;
    cch= WideCharToMultiByte(CP_ACP, 0, szLanguageW, -1, szLanguage, countof(szLanguage), "X", &bUsed);
    if (bUsed || nAnsiCodePage==0)
    { // Failed to convert to the current codepage -> we'll fallback to name in current language
      cch= 0;
    }
#endif
  }
  // else: Can't get native name -> we'll fallback to name in current language
  
  // Can we display the native name ?
  if (cch==0)
  { // No -> fallback to name in current language
    cch= GetLocaleInfo( MAKELCID(wLangId, SORT_DEFAULT), LOCALE_SLANGUAGE, szLanguage, countof(szLanguage));
    if (cch==0)
      _stprintf(szLanguage, _T("%u - ???"), wLangId); // Ouch ! We can't even display the name in the current language !
  }

  return szLanguage;
}

void CLanguageSupport::LookupExeLanguage()
{ // Initializes m_nExeLanguage with the original language of the app (stored in the exe)

  // Get executable filename
  TCHAR szFilename[MAX_PATH]={0};
  DWORD cch= GetModuleFileName( NULL, szFilename, MAX_PATH);
  ASSERT(cch!=0);

  // Look up language of exe file
  m_nExeLanguage= GetLangIdFromFile(szFilename);
}
