
#if !defined(AFX_SM_IPEDIA_H__6B0A6D56_EEA8_48CF_B48B_C0C09E513635__INCLUDED_)
#define AFX_SM_IPEDIA_H__6B0A6D56_EEA8_48CF_B48B_C0C09E513635__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

#define MENU_HEIGHT 26
#include <windows.h>
#include <Definition.hpp>
#include <BaseTypes.hpp>
#include <LookupManagerBase.hpp>
#include <Definition.hpp>

//extern HWND             g_hwndMain;
//extern HINSTANCE        g_hInst;
extern Definition *     g_definition;
extern ArsLexis::String g_recentWord;
extern ArsLexis::String g_searchWord;

bool GotoURL(LPCTSTR lpszUrl);
void setupAboutWindow();
void setMenu(HWND hwnd);
void setUIState(bool enabled = true);

extern Definition *g_about;
extern Definition *g_register;
extern Definition *g_tutorial;
extern Definition *g_wikipedia;
extern GenericTextElement* g_articleCountElement;
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

#endif // !defined(AFX_SM_IPEDIA_H__6B0A6D56_EEA8_48CF_B48B_C0C09E513635__INCLUDED_)
