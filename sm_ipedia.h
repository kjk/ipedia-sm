#ifndef _IPEDIA_CE_H_
#define _IPEDIA_CE_H_

#include <windows.h>
#include <Definition.hpp>
#include <BaseTypes.hpp>
#include <LookupManagerBase.hpp>
#include "resource.h"

#define MENU_HEIGHT 26

#ifdef WIN32_PLATFORM_PSPC
#define MAIN_MENU_BUTTON _T("Main")
#define REGISTER_MENU_ITEM _T("Options/Register")
#define UPDATES_MENU_ITEM _T("Options/Check updates")
#else
#define MAIN_MENU_BUTTON _T("Menu")
#define REGISTER_MENU_ITEM _T("Menu/Register")
#define UPDATES_MENU_ITEM _T("Options/About/Check updates")
#endif

extern ArsLexis::String g_recentWord;
extern ArsLexis::String g_searchWord;

void SetupAboutWindow();
void setMenu(HWND hwnd);
void setUIState(bool enabled = true);

extern long g_articleCountSet;
extern bool g_forceLayoutRecalculation;

enum DisplayMode
{
    showAbout,
    showTutorial,
    showRegister,
    showArticle,
    showWikipedia
};

void setDisplayMode(DisplayMode mode);
DisplayMode displayMode();
Definition& currentDefinition();
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

#endif
