#ifndef _IPEDIA_CE_H_
#define _IPEDIA_CE_H_

#include <windows.h>
#include <Definition.hpp>
#include <BaseTypes.hpp>
#include <LookupManagerBase.hpp>
#include "resource.h"

#define MENU_HEIGHT 26

//extern HWND             g_hwndMain;
//extern HINSTANCE        g_hInst;
extern Definition *     g_definition;
extern ArsLexis::String g_recentWord;
extern ArsLexis::String g_searchWord;

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

#endif
