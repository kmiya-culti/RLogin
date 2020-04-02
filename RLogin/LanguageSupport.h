#pragma once

#include <afxcoll.h>


#define ID_LANGUAGE_FIRST 0x6F00
#define ID_LANGUAGE_LAST  0x6FFF

class CLanguageSupport
{
public:
  CLanguageSupport();
  ~CLanguageSupport();

  void CreateMenu(CCmdUI *pCmdUI, UINT nFirstItemId= ID_LANGUAGE_FIRST); // Called from Language menu item's update handler
  void OnSwitchLanguage(UINT nId, bool bLoadNewLanguageImmediately= false); // Called from the language sub-menu items handler

  void LoadBestLanguage(); // Called on start-up to load the required language dll

protected:
  CWordArray m_aLanguages;
  LANGID m_nExeLanguage;
  LANGID m_nCurrentLanguage;

  HINSTANCE m_hDll; // Satellite DLL.

  static const TCHAR szLanguageId[]; // Registry value name for current language
  static LANGID GetLangIdFromFile(LPCTSTR pszFilename);
  static CString GetLanguageName(LANGID wLangId);

  static LANGID GetUserUILanguage();
  static LANGID GetSystemUILanguage();

  void GetAvailableLanguages(); // Fills m_aLanguages with the list of languages available (as resource DLLs + the language of the exe)

  bool LoadLanguage(); // Try to load the language m_nCurrentLanguage.
                       // If it doesn't exist, try to fallback on the neutral or default sublanguage for this language.

  bool LoadLanguageDll(); // Identifies and loads the resource DLL for language m_nCurrentLanguage

  void LookupExeLanguage(); // Initializes m_nExeLanguage with the original language of the app (stored in the exe)

  void UnloadResourceDll(); // Unloads the current resource DLL (if any)

  static void SetResourceHandle(HINSTANCE hDll); // Tells MFC where the resources are stored.
};
  