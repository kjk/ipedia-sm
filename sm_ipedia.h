
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

extern HWND g_hwndMain;
extern HINSTANCE g_hInst;
extern Definition *g_definition;
extern ArsLexis::String recentWord;
extern ArsLexis::String searchWord;


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

class CommonProgressReporter
{
protected:
    DWORD ticksAtUpdate_;
    DWORD ticksAtStart_;
    uint_t lastPercent_;
    bool showProgress_:1;
    bool afterTrigger_:1;
    CommonProgressReporter();
    void update(uint_t percent);
    void setTicksAtUpdate(DWORD ticks)
    {ticksAtUpdate_ = ticks;}
    bool shallShow()
    {return showProgress_;}
};

class RenderingProgressReporter: public Definition::RenderingProgressReporter, CommonProgressReporter
{
    HWND hwndMain_;
    ArsLexis::String waitText_;
    
public:
    
    RenderingProgressReporter(HWND hwnd);
    
    virtual void reportProgress(uint_t percent);
};

class SmartPhoneProgressReported: public ArsLexis::DefaultLookupProgressReporter, CommonProgressReporter
{
    void showProgress(const ArsLexis::LookupProgressReportingSupport& support, ArsLexis::Graphics& gr, const ArsLexis::Rectangle& bounds, bool clearBkg=true);
};

struct ErrorsTableEntry
{
    int errorCode;
    ArsLexis::String title;
    ArsLexis::String message;

    ErrorsTableEntry(int eCode, ArsLexis::String tit, ArsLexis::String msg):
        errorCode(eCode),
        title(tit),
        message(msg)
    {}

};

#endif // !defined(AFX_SM_IPEDIA_H__6B0A6D56_EEA8_48CF_B48B_C0C09E513635__INCLUDED_)
