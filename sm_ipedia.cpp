#include <windows.h>
#include <wingdi.h>
#ifdef WIN32_PLATFORM_WFSP
#include <tpcshell.h>
#endif

#include <sms.h>
#include <uniqueid.h>

#include <WinSysUtils.hpp>
#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include <LookupManagerBase.hpp>
#include <LookupHistory.hpp>
#include <Definition.hpp>
#include <DefinitionElement.hpp>
#include <GenericTextElement.hpp>
#include <Geometry.hpp>
#include <Debug.hpp>
#include <FontEffects.hpp>

#include <Text.hpp>

#include <EnterRegCodeDialog.hpp>
#include <StringListDialog.hpp>

#include "ProgressReporters.h"
#include "ExtSearchResultsDlg.hpp"
#include "DefaultArticles.hpp"
#include "sm_ipedia.h"
#include "ipedia_rsc.h"

using namespace ArsLexis;

// holds new registration code being checked while we send the request to
// server to check it.
// TODO: we should stick it in preferences
String g_newRegCode;

bool g_fRegistration = false;

bool g_lbuttondown = false;

enum ScrollUnit
{
    scrollLine,
    scrollPage,
    scrollHome,
    scrollEnd,
    scrollPosition
};

static RECT g_progressRect = { 0, 0, 0, 0 };
static RenderingProgressReporter* g_RenderingProgressReporter = NULL;
static RenderingProgressReporter* g_RegistrationProgressReporter = NULL;

iPediaApplication iPediaApplication::instance_;

Definition *g_definition = new Definition();
Definition *g_about = new Definition();
Definition *g_register = new Definition();
Definition *g_tutorial = new Definition();
Definition *g_wikipedia = new Definition();
DisplayMode g_displayMode = showAbout;

SHACTIVATEINFO g_sai;

long g_articleCountSet = -1;

bool  g_forceLayoutRecalculation = false;
bool  g_forceAboutRecalculation  = false;
bool  g_recalculationInProgress = false;
DWORD g_recalculationData  = 0;

RenderingProgressReporter* rep;
LRESULT handleMenuCommand(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

static int  g_stressModeCnt = 0;

bool g_uiEnabled = true;

HWND      g_hwndEdit         = NULL;
HWND      g_hwndScroll       = NULL;
HWND      g_hWndMenuBar      = NULL;   // Current active menubar
HWND      g_hwndSearchButton = NULL;
WNDPROC   g_oldEditWndProc   = NULL;

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

/*DWORD WINAPI formattingThreadProc(LPVOID lpParameter)
{
    RepaintDefiniton(0);
    g_recalculationInProgress = false;
    return 0;
}*/

DisplayMode displayMode()
{return g_displayMode;}

void SetDisplayMode(DisplayMode displayMode)
{
    g_displayMode=displayMode;
}

Definition& currentDefinition()
{
    switch( displayMode() )
    {
        case showArticle:
            return (*g_definition);
        case showAbout:
            return (*g_about);
        case showRegister:
            return (*g_register);
        case showTutorial:
            return (*g_tutorial);
        case showWikipedia:
            return (*g_wikipedia);
        default:
            // shouldn't happen
            assert(0);
            break;
    }
    return (*g_about);
}

static void DrawTextInRect(Graphics& gr, ArsLexis::Rectangle& rect, const char_t *text)
{
    HDC hdc = gr.handle();
    HBRUSH hbr = CreateSolidBrush(RGB(255,255,255));
    HBRUSH oldbr = (HBRUSH)SelectObject(hdc, hbr);
    int x = rect.x();
    int y = rect.y();
    int width = rect.width();
    int height = rect.height();
    ::Rectangle(hdc, x, y, x + width, y+height);
    
    POINT points[4];
    points[0].x = points[3].x = x +2;
    points[0].y = points[1].y = y +2;
    points[2].y = points[3].y = y + height - 3;
    points[1].x = points[2].x = x + width - 3;   
    Polygon(hdc, points,4);

    SelectObject(hdc, oldbr);
    DeleteObject(hbr);

    uint_t rectDx = (uint_t)rect.dx();
    uint_t txtDx = rectDx;
    uint_t length = tstrlen(text);
    gr.charsInWidth(text, length, txtDx);
    uint_t txtXOffsetCentered = 0;
    if (rectDx>txtDx)
        txtXOffsetCentered = (rectDx-txtDx)/2;

    uint_t dy = gr.fontHeight();
    uint_t rectDy = (uint_t)rect.dy();
    uint_t txtYOffsetCentered = 0;
    if (rectDy>dy)
        txtYOffsetCentered = (rectDy-dy)/2;
    
    Point p(rect.x()+txtXOffsetCentered, rect.y()+txtYOffsetCentered);
    gr.drawText(text, length, p);
}

static void ShowEstablishingConnection()
{
    iPediaApplication& app=GetApp();
    Graphics gr(GetDC(app.getMainWindow()), app.getMainWindow());

    ArsLexis::Rectangle ar = g_progressRect;
    Graphics::FontSetter setFont(gr, Font(11));
    Graphics::ColorSetter setBg(gr, Graphics::colorBackground, RGB(255,255,255));
    Graphics::ColorSetter setFg(gr, Graphics::colorText, RGB(0,0,0));
    DrawTextInRect(gr, ar, _T("Establishing connection..."));
}

// try to establish internet connection.
// If can't (e.g. because tcp/ip stack is not working), display a dialog box
// informing about that and return false
// Return true if established connection.
// Can be called multiple times - will do nothing if connection is already established.
static bool fInitConnection()
{
    if (FDataConnectionEstablished())
        return true;

#if WIN32_PLATFORM_WFSP
    // we only want to show that on a smartphone
    // we also should on Pocket PC Phone Edition but I don't know
    // how to distinguish between PPC and PPC Phone Edition
    ShowEstablishingConnection();
#endif
    if (InitDataConnection())
        return true;

#ifdef WIN32_PLATFORM_PSPC
    // just ignore on Pocket PC. I don't know how to make it work 
    // reliably across both Pocket PC and Pocket PC Phone Edition
    return true;
#else
    String errorMsg = _T("Unable to connect");
    errorMsg.append(_T(". Verify your dialup or proxy settings are correct, and try again."));
    iPediaApplication& app = GetApp();
    MessageBox(app.getMainWindow(), errorMsg.c_str(), _T("Error"), MB_OK | MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND );
    return false;
#endif
}

static void OnLinkedArticles(HWND hwnd)
{
     if (currentDefinition().empty())
       return;
    
    CharPtrList_t strList;

    Definition::ElementPosition_t posCur;
    Definition::ElementPosition_t posStart = currentDefinition().firstElementPosition();
    Definition::ElementPosition_t posEnd   = currentDefinition().lastElementPosition();
    DefinitionElement *currEl;
    const char_t *articleTitle;
    for (posCur=posStart; posCur!=posEnd; posCur++)
    {
        currEl = *posCur;
        if (currEl->isTextElement())
        {
            GenericTextElement *txtEl=(GenericTextElement*)currEl;
            if ((txtEl->isHyperlink()) &&
                ((txtEl->hyperlinkProperties()->type==hyperlinkTerm) ||
                 (txtEl->hyperlinkProperties()->type==hyperlinkExternal)))
            {
                articleTitle = txtEl->hyperlinkProperties()->resource.c_str();
                strList.push_back(articleTitle);
            }
        }
    }

    if (strList.empty())
        return;

    String selectedString;
    bool fSelected = FGetStringFromListRemoveDups(hwnd, strList, selectedString);
    if (!fSelected)
        return;

    assert (!selectedString.empty());

    GenericTextElement *txtElMatching = NULL;
    for (posCur=posStart; posCur!=posEnd; posCur++)
    {
        currEl = *posCur;
        if (currEl->isTextElement())
        {
            GenericTextElement *txtEl=(GenericTextElement*)currEl;
            if ((txtEl->isHyperlink()) &&
                ((txtEl->hyperlinkProperties()->type==hyperlinkTerm) ||
                 (txtEl->hyperlinkProperties()->type==hyperlinkExternal)))
            {
                if (txtEl->hyperlinkProperties()->resource == selectedString)
                {
                    txtElMatching = txtEl;
                    break;
                }
            }
        }
    }

    assert(NULL!=txtElMatching);

    if (NULL!=txtElMatching)
    {
        txtElMatching->performAction(currentDefinition());
    }
}

static void SetScrollBar(Definition* definition)
{
    int first = 0;
    int total = 0;
    int shown = 0;
    if ((NULL!=definition) && (!definition->empty()))
    {
        first=definition->firstShownLine();
        total=definition->totalLinesCount();
        shown=definition->shownLinesCount();
    }
    SetScrollPos(g_hwndScroll, SB_CTL, first, TRUE);
    SetScrollRange(g_hwndScroll, SB_CTL, 0, total-shown, TRUE);
}

static void SetMenuBarButtonState(int buttonID, bool fEnabled)
{
    TBBUTTONINFO    buttonInfo = {0};
    buttonInfo.cbSize = sizeof( buttonInfo );
    buttonInfo.dwMask = TBIF_STATE;
    if (fEnabled)
        buttonInfo.fsState = TBSTATE_ENABLED;
    else
        buttonInfo.fsState = TBSTATE_INDETERMINATE;
    SendMessage( g_hWndMenuBar, TB_SETBUTTONINFO, buttonID, (LPARAM)(LPTBBUTTONINFO)&buttonInfo );
}

static bool FDefinitionHasLinks(Definition& def)
{
    Definition::ElementPosition_t posCur;
    Definition::ElementPosition_t posStart = def.firstElementPosition();
    Definition::ElementPosition_t posEnd   = def.lastElementPosition();
    DefinitionElement *currEl;
    for (posCur=posStart; posCur!=posEnd; posCur++)
    {
        currEl = *posCur;
        if (currEl->isTextElement())
        {
            GenericTextElement *txtEl = (GenericTextElement*)currEl;
            if ((txtEl->isHyperlink()) && (txtEl->hyperlinkProperties()->type==hyperlinkTerm))
            {
                return true;
            }
        }
    }
    return false;
}

static void SetMenu(HWND hwnd)
{
    HWND hwndMB = SHFindMenuBar (hwnd);
    if (!hwndMB) 
        return;

    bool isNextHistoryElement = false;
    HMENU hMenu;
    hMenu = (HMENU)SendMessage(hwndMB, SHCMBM_GETSUBMENU, 0, ID_MENU_BTN);
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager(true);

    unsigned int menuState = MF_ENABLED;
    if (lookupManager->lastExtendedSearchResults().empty())
        menuState = MF_GRAYED;
    EnableMenuItem(hMenu, IDM_MENU_RESULTS, menuState);

    menuState = MF_ENABLED;
    if (lookupManager->lastReverseLinks().empty())
        menuState = MF_GRAYED;
    EnableMenuItem(hMenu, IDM_MENU_REVERSE_LINKS, menuState);

    menuState = MF_GRAYED;
    if (lookupManager->hasNextHistoryTerm())
        menuState = MF_ENABLED;
    EnableMenuItem(hMenu, IDM_MENU_NEXT, menuState);

    menuState = MF_GRAYED;
    if (lookupManager->hasPreviousHistoryTerm() ||
        ( (displayMode()!=showArticle) && (!g_definition->empty()) )
       )
    {
        menuState = MF_ENABLED;
    }

    EnableMenuItem(hMenu, IDM_MENU_PREV, menuState);
    
    menuState = MF_GRAYED;
    Definition &def = currentDefinition();
    if (!def.empty())
    {
        menuState = MF_ENABLED;
    }
    EnableMenuItem(hMenu, IDM_MENU_CLIPBOARD, menuState);

#ifdef WIN32_PLATFORM_PSPC
    if (g_uiEnabled)
    {
        bool fEnabled = false;
        if (lookupManager->hasNextHistoryTerm())
            fEnabled = true;
        SetMenuBarButtonState(ID_NEXT_BTN, fEnabled);

        fEnabled = false;
        if (lookupManager->hasPreviousHistoryTerm() || 
            ((displayMode()!=showArticle) && (!g_definition->empty())) )
        {
            fEnabled = true;
        }
        SetMenuBarButtonState(ID_PREV_BTN, fEnabled);
    }
#endif

    if (FDefinitionHasLinks(def))
        EnableMenuItem(hMenu, IDM_MENU_HYPERS, MF_ENABLED);
    else
        EnableMenuItem(hMenu, IDM_MENU_HYPERS, MF_GRAYED);
}

static void SetupAboutWindow()
{
    iPediaApplication &app = GetApp();
    if (-1!=app.preferences().articleCount)
    {
        g_articleCountSet = app.preferences().articleCount;
        updateArticleCountEl(app.preferences().articleCount, app.preferences().databaseTime);
        prepareAbout(g_about);
        g_forceAboutRecalculation = true;
    }    
}

void SetUIState(bool enabled)
{
    // TODO: for testing, always enable
    enabled = true;

    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager(true);

#ifdef WIN32_PLATFORM_PSPC
    SetMenuBarButtonState(ID_MENU_BTN, enabled);
    SetMenuBarButtonState(ID_OPTIONS_MENU_BTN, enabled);
    if (enabled)
    {
        SetMenuBarButtonState(ID_NEXT_BTN, lookupManager->hasNextHistoryTerm());
        SetMenuBarButtonState(ID_PREV_BTN, lookupManager->hasPreviousHistoryTerm());
    }
    else
    {
        SetMenuBarButtonState(ID_NEXT_BTN, false);
        SetMenuBarButtonState(ID_PREV_BTN, false);
    }
    EnableWindow(g_hwndSearchButton, enabled);
#else
    SetMenuBarButtonState(ID_MENU_BTN, enabled);
    SetMenuBarButtonState(ID_SEARCH, enabled);
#endif

    g_uiEnabled = enabled;
}

static void HandleExtendSelection(HWND hwnd, int x, int y, bool finish)
{
    iPediaApplication& app = GetApp();
    if (app.fLookupInProgress())
        return;
    Definition &def = currentDefinition();
    if (def.empty())
        return;
    ArsLexis::Point point(x,y);
    Graphics graphics(GetDC(hwnd), hwnd);
    def.extendSelection(graphics, app.preferences().renderingPreferences, point, finish);         
}

static void RepaintDefiniton(int scrollDelta)
{
    iPediaApplication& app = GetApp();
    const RenderingPreferences& prefs = app.preferences().renderingPreferences;
    Definition &def = currentDefinition();

    RECT clientRect;
    GetClientRect(app.getMainWindow(), &clientRect);
    ArsLexis::Rectangle bounds = clientRect;

    RECT defRectTmp = clientRect;
    defRectTmp.top    += 24;
    defRectTmp.left   += 2;
    defRectTmp.right  -= 2 + GetScrollBarDx();
    defRectTmp.bottom -= 2;

    ArsLexis::Rectangle defRect = defRectTmp;

    Graphics gr(GetDC(app.getMainWindow()), app.getMainWindow());
    if ( (true == g_forceAboutRecalculation) && (displayMode() == showAbout) )
    {
        g_forceLayoutRecalculation = true;
        g_forceAboutRecalculation = false;
    }   

    bool fDidDoubleBuffer=false;
    HDC offscreenDc=::CreateCompatibleDC(gr.handle());
    if (offscreenDc) 
    {
        HBITMAP bitmap=::CreateCompatibleBitmap(gr.handle(), bounds.width(), bounds.height());
        if (bitmap) 
        {
            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
            Graphics offscreen(offscreenDc, NULL);
            gr.copyArea(defRect, offscreen, defRect.topLeft);
            if (0 != scrollDelta)
                def.scroll(offscreen, prefs, scrollDelta);
            else
                def.render(offscreen, defRect, prefs, g_forceLayoutRecalculation);
            offscreen.copyArea(defRect, gr, defRect.topLeft);
            ::SelectObject(offscreenDc, oldBitmap);
            ::DeleteObject(bitmap);
            fDidDoubleBuffer = true;
        }
        ::DeleteDC(offscreenDc);
    }

    if (!fDidDoubleBuffer)
    {
        if (0 != scrollDelta)
            def.scroll(gr, prefs, scrollDelta);
        else
            def.render(gr, defRect, prefs, g_forceLayoutRecalculation);
    }

    SetScrollBar(&def);
    if (g_forceLayoutRecalculation)
        PostMessage(app.getMainWindow(),WM_COMMAND, IDM_ENABLE_UI, 0);
    g_forceLayoutRecalculation = false;
}

void ScrollDefinition(int units, ScrollUnit unit, bool updateScrollbar)
{
    iPediaApplication& app = GetApp();
    if (app.fLookupInProgress())
        return;

    if (g_recalculationInProgress)
        return;

    Definition &def = currentDefinition();
    if (def.empty())
        return;

    switch (unit)
    {
        case scrollPage:
            units = units * def.shownLinesCount();
            break;
        case scrollEnd:
            units = def.totalLinesCount();
            break;
        case scrollHome:
            units = -(int)def.totalLinesCount();
            break;
        case scrollPosition:
            units = units - def.firstShownLine();
            break;
    }
    RepaintDefiniton(units);
}

void *g_ClipboardText = NULL;

static void FreeClipboardData()
{
    if (NULL!=g_ClipboardText)
    {
        LocalFree(g_ClipboardText);
        g_ClipboardText = NULL;
    }
}

static void* CreateNewClipboardData(const String& str)
{
    FreeClipboardData();

    int     strLen = str.length();

    g_ClipboardText = LocalAlloc(LPTR, (strLen+1)*sizeof(char_t));
    if (NULL!=g_ClipboardText)
        return NULL;

    ZeroMemory(g_ClipboardText, (strLen+1)*sizeof(char_t));
    memcpy(g_ClipboardText, str.c_str(), strLen*sizeof(char_t));
    return g_ClipboardText;
}

// copy definition to clipboard
static void CopyToClipboard(HWND hwndMain, Definition *def)
{
    void *    clipData;
    String    text;

    if (def->empty())
        return;

    if (!OpenClipboard(hwndMain))
        return;

    // TODO: should we put it anyway?
    if (!EmptyClipboard())
        goto Exit;
    
    def->selectionToText(text);

    clipData = CreateNewClipboardData(text);
    if (NULL==clipData)
        goto Exit;
    
    SetClipboardData(CF_UNICODETEXT, clipData);
Exit:
    CloseClipboard();
}

static void MoveHistoryForwardOrBack(bool forward)
{
    iPediaApplication& app = GetApp();

    if (displayMode()!=showArticle)
    {
        if (!g_definition->empty())
        {
            SetDisplayMode(showArticle);
            SetScrollBar(g_definition);
            InvalidateRect(app.getMainWindow(), NULL, false);
            return;
        }
    }

    LookupManager* lookupManager=app.getLookupManager(true);
    if (app.fLookupInProgress())
        return;

    if (!fInitConnection())
        return;
    lookupManager->moveHistory(forward);
    SetUIState(false);
}

void MoveHistoryForward()
{
    MoveHistoryForwardOrBack(true);
}

void MoveHistoryBack()
{
    MoveHistoryForwardOrBack(false);
}

static void SimpleOrExtendedSearch(HWND hwnd, String& term, bool simple)
{
    if (term.empty())
        return;

    iPediaApplication& app = GetApp();
    if (app.fLookupInProgress())
        return;

    LookupManager* lookupManager = app.getLookupManager(true);
    if (NULL==lookupManager)
        return;

    if (simple && !lookupManager->lastSearchTermDifferent(term))
    {
        if (displayMode()!=showArticle)
        {
            SetDisplayMode(showArticle);
            SetScrollBar(g_definition);
            SetUIState(true);
            InvalidateRect(hwnd,NULL,FALSE);                            
        }
        return;
    }

    if (!fInitConnection())
        return;

    if (simple)
    {
        bool fDifferent = lookupManager->lookupIfDifferent(term);
        assert(fDifferent); // we checked for that above so it must be true
        SetUIState(false);
        InvalidateRect(hwnd,NULL,FALSE);
    }
    else
    {
        lookupManager->search(term);
        SetUIState(false);
        InvalidateRect(hwnd,NULL,FALSE);
    }
}

static void DoSimpleSearch(HWND hwnd, String& term)
{
    SimpleOrExtendedSearch(hwnd, term, true);
}

static void DoExtendedSearch(HWND hwnd, String& term)
{
    SimpleOrExtendedSearch(hwnd, term, false);
}

static void DoExtSearchResults(HWND hwnd)
{
    iPediaApplication& app=GetApp();
    if (app.fLookupInProgress())
        return;

    LookupManager* lookupManager = app.getLookupManager(true);
    String lastSearchResults = lookupManager->lastExtendedSearchResults();

    CharPtrList_t strList;
    AddLinesToList(lastSearchResults, strList);
    if (strList.empty())
        return;

    String strResult;
    int res = ExtSearchResultsDlg(hwnd, strList, strResult);

    FreeStringsFromCharPtrList(strList);

    if (EXT_RESULTS_REFINE==res)
    {
        String newExtendedSearchTerm;
        newExtendedSearchTerm.assign(lookupManager->lastExtendedSearchTerm());
        newExtendedSearchTerm.append(_T(" "));
        newExtendedSearchTerm.append(strResult);
        DoExtendedSearch(hwnd, newExtendedSearchTerm);
    } 
    else if (EXT_RESULTS_SELECT==res)
    {
        DoSimpleSearch(hwnd, strResult);
    }
}    

static void OnReverseLinks(HWND hwnd)
{
    if (currentDefinition().empty())
      return;

    iPediaApplication& app=GetApp();
    LookupManager *lookupManager = app.getLookupManager();
    if (NULL==lookupManager)
        return;
    String& reverseLinks = lookupManager->lastReverseLinks();

    char_t *str = (char_t*)reverseLinks.c_str();
    replaceCharInString(str, _T('_'), _T(' '));

    CharPtrList_t strList;

    AddLinesToList(reverseLinks, strList);

    if (strList.empty())
        return;

    String selectedString;
    bool fSelected = FGetStringFromListRemoveDups(hwnd, strList, selectedString);

    FreeStringsFromCharPtrList(strList);

    if (!fSelected)
        return;

    assert (!selectedString.empty());

    DoSimpleSearch(hwnd, selectedString);
}

static void DoRandom()
{
    iPediaApplication& app=GetApp();
    if (app.fLookupInProgress())
        return;
    if (!fInitConnection())
        return;
    LookupManager* lookupManager=app.getLookupManager(true);
    lookupManager->lookupRandomTerm();
    SetUIState(false);
}

static void DoSearch(HWND hwnd)
{
    String term;
    GetEditWinText(g_hwndEdit, term);            
    DoSimpleSearch(hwnd,term);
}

static void DoRegister(HWND hwnd, const String& oldRegCode)
{
    String newRegCode;

    bool fOk = FGetRegCodeFromUser(hwnd, oldRegCode, newRegCode);
    if (!fOk)
        return;

    assert(!g_newRegCode.empty());

    iPediaApplication& app = GetApp();
    if (app.fLookupInProgress())
        return;

    if (!fInitConnection())
        return;

    g_newRegCode = newRegCode;
    LookupManager* lookupManager = app.getLookupManager(true);
    lookupManager->verifyRegistrationCode(g_newRegCode);
    g_fRegistration = true;
    SetUIState(false);
}

static void DoExtendedSearchMenu(HWND hwnd)
{
    String term;
    GetEditWinText(g_hwndEdit, term);            
    DoExtendedSearch(hwnd,term);
}

static void OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint (hwnd, &ps);

    RECT rcpaint = ps.rcPaint;

    RECT rect;
    GetClientRect (hwnd, &rect);
    ArsLexis::Rectangle rin(rcpaint);
    ArsLexis::Rectangle rout(g_progressRect);
    
    iPediaApplication& app=GetApp();
    bool fLookupInProgress = app.fLookupInProgress();

    bool onlyProgress=false;

    if (fLookupInProgress &&
        (rout && rin.topLeft) && (rout.extent.x>=rin.extent.x)
        && (rout.extent.y>=rin.extent.y))
    {
            onlyProgress = true;
    }

    rect.top    += 22;
    rect.left   += 2;
    rect.right  -= (2+GetScrollBarDx());
    rect.bottom -= 2;

    if ( !onlyProgress && !g_recalculationInProgress)
    {
        Definition &def = currentDefinition();
        if (g_forceLayoutRecalculation)
        {
            /*
            DWORD threadID;
            g_recalculationInProgress = true;
            HANDLE hThread;
            hThread = CreateThread(NULL, 0, formattingThreadProc, &g_recalculationData, 0, &threadID);
            if (NULL!=hThread)
                CloseHandle(hThread);*/
            RepaintDefiniton(0);
        }
        else
        {
            // TODO: a bit of a hack - we shouldn't be getting repaint if we
            // started the download
            if (!fLookupInProgress)
            {
                RepaintDefiniton(0);
            }
        }
    }

    if (fLookupInProgress)
    {
        if (g_fRegistration)
        {
            g_RegistrationProgressReporter->reportProgress(-1);
        }
        else
        {
            Graphics gr(GetDC(app.getMainWindow()), app.getMainWindow());    
            ArsLexis::Rectangle progressArea(g_progressRect);
            app.getLookupManager()->showProgress(gr, progressArea);
        }
    }

    EndPaint (hwnd, &ps);
}

static LRESULT OnCommand(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    iPediaApplication& app = GetApp();
    LookupManager* lookupManager=app.getLookupManager(true);
    iPediaApplication::Preferences& prefs = GetPrefs();

    // ignore them while we're doing network transfer
    if ( (NULL==lookupManager) || app.fLookupInProgress())
        return TRUE;

    LRESULT lResult = TRUE;

    switch (wp)
    {   
        case IDM_MENU_CLIPBOARD:
            CopyToClipboard(app.getMainWindow(),g_definition);
            break;
        
        case IDM_MENU_STRESS_MODE:
            g_stressModeCnt = 100;
            SendMessage(hwnd, WM_COMMAND, IDM_MENU_RANDOM, 0);
            break;
        
        case IDM_ENABLE_UI:
            SetUIState(true);
            if (g_stressModeCnt>0)
            {
                g_stressModeCnt--;
                SendMessage(hwnd, WM_COMMAND, IDM_MENU_RANDOM, 0);
            }
            break;
        
        case IDOK:
            SendMessage(hwnd,WM_CLOSE,0,0);
            break;
        
        case IDM_MENU_HOME:
#ifdef WIN32_PLATFORM_PSPC
            GotoURL(_T("http://arslexis.com/pda/ppc.html"));
#else
            GotoURL(_T("http://arslexis.com/pda/sm.html"));
#endif
            break;
        
        case IDM_MENU_UPDATES:
#ifdef WIN32_PLATFORM_PSPC
            GotoURL(_T("http://arslexis.com/updates/ppc-ipedia-1-0.html"));
#else
            GotoURL(_T("http://arslexis.com/updates/sm-ipedia-1-0.html"));
#endif
            break;
        
        case IDM_MENU_ABOUT:
            SetDisplayMode(showAbout);
            SetMenu(hwnd);
            SetScrollBar(g_about);
            InvalidateRect(hwnd,NULL,FALSE);
            break;
        
        case IDM_MENU_TUTORIAL:
            SetDisplayMode(showTutorial);
            SetMenu(hwnd);
            SetScrollBar(g_about);
            InvalidateRect(hwnd,NULL,FALSE);
            break;
        
        case IDM_EXT_SEARCH:
            OnPaint(hwnd);
            DoExtendedSearchMenu(hwnd);
            break;
        
        case ID_SEARCH_BTN:
        // intentional fall-through
        case ID_SEARCH_BTN2:
        // intentional fall-through
        case ID_SEARCH:
        // intentional fall-through
            OnPaint(hwnd);
            DoSearch(hwnd);
            break;
        
        case IDM_MENU_HYPERS:
            OnLinkedArticles(hwnd);
            break;

        case IDM_MENU_REVERSE_LINKS:
            OnReverseLinks(hwnd);
            break;

        case IDM_MENU_RANDOM:
            OnPaint(hwnd);
            DoRandom();
            break;

        case IDM_MENU_REGISTER:
            OnPaint(hwnd);
            DoRegister(hwnd, prefs.regCode);
            break;

        case IDM_MENU_PREV:
            // intentionally fall through
        case ID_PREV_BTN:
            OnPaint(hwnd);
            MoveHistoryBack();
            break;

        case IDM_MENU_NEXT:
            // intentionally fall through
        case ID_NEXT_BTN:
            OnPaint(hwnd);
            MoveHistoryForward();
            break;

        case IDM_MENU_RESULTS:
            DoExtSearchResults(hwnd);
            break;
        default:
            lResult  = DefWindowProc(hwnd, msg, wp, lp);
    }
    return lResult;
}

static void OnSettingChange(HWND hwnd, WPARAM wp, LPARAM lp)
{
    SHHandleWMSettingChange(hwnd, wp, lp, &g_sai);
}

static void OnActivateMain(HWND hwnd, WPARAM wp, LPARAM lp)
{
    SHHandleWMActivate(hwnd, wp, lp, &g_sai, 0);
}

static void OnHibernate(HWND hwnd, WPARAM wp, LPARAM lp)
{
    // do nothing
}

static void OnCreate(HWND hwnd)
{
    iPediaApplication &app = GetApp();
    app.loadPreferences();

    app.setMainWindow(hwnd);

    LookupManager* lookupManager=app.getLookupManager(true);
    lookupManager->setProgressReporter(new DownloadingProgressReporter());
    g_RenderingProgressReporter = new RenderingProgressReporter(hwnd, g_progressRect, _T("Formatting article..."));
    g_definition->setRenderingProgressReporter(g_RenderingProgressReporter);
    g_definition->setHyperlinkHandler(&app.hyperlinkHandler());
    
    g_RegistrationProgressReporter = new RenderingProgressReporter(hwnd, g_progressRect, _T("Registering application..."));

    g_articleCountSet = app.preferences().articleCount;
    
    g_about->setHyperlinkHandler(&app.hyperlinkHandler());
    g_tutorial->setHyperlinkHandler(&app.hyperlinkHandler());
    g_register->setHyperlinkHandler(&app.hyperlinkHandler());
    g_wikipedia->setHyperlinkHandler(&app.hyperlinkHandler());

    prepareAbout(g_about);
    prepareHowToRegister(g_register);
    prepareTutorial(g_tutorial);
    prepareWikipedia(g_wikipedia);

    // create the menu bar
    SHMENUBARINFO mbi = {0};
    mbi.cbSize = sizeof(mbi);
    mbi.hwndParent = hwnd;
    mbi.nToolBarId = IDR_MAIN_MENUBAR;
#ifdef WIN32_PLATFORM_PSPC
    mbi.nBmpId     = IDB_TOOLBAR;
    mbi.cBmpImages = 3;
#endif

    mbi.hInstRes = app.getApplicationHandle();

    if (!SHCreateMenuBar(&mbi)) {
        PostQuitMessage(0);
    }
    g_hWndMenuBar = mbi.hwndMB;

    OverrideBackButton(mbi.hwndMB);

#ifdef WIN32_PLATFORM_PSPC
    ZeroMemory(&g_sai, sizeof(g_sai));
    g_sai.cbSize = sizeof(g_sai);

    // size the main window taking into account SIP state and menu bar size
    SIPINFO si = {0};
    si.cbSize = sizeof(si);
    SHSipInfo(SPI_GETSIPINFO, 0, (PVOID)&si, FALSE);

    int visibleDx = RectDx(&si.rcVisibleDesktop);
    int visibleDy = RectDy(&si.rcVisibleDesktop);

    if ( FDesktopIncludesMenuBar(&si) )
    {
        RECT rectMenuBar;
        GetWindowRect(mbi.hwndMB, &rectMenuBar);
        int menuBarDy = RectDy(&rectMenuBar);
        visibleDy -= menuBarDy;
    }
    SetWindowPos(hwnd, NULL, 0, 0, visibleDx, visibleDy, SWP_NOMOVE | SWP_NOZORDER);
#endif

    g_hwndEdit = CreateWindow(
        TEXT("edit"),
        NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | 
        WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        0,0,0,0,hwnd,
        (HMENU) ID_EDIT,
        app.getApplicationHandle(),
        NULL);

#ifdef WIN32_PLATFORM_PSPC
    g_hwndSearchButton = CreateWindow(
        _T("button"),  
        _T("Search"),
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON ,//| BS_OWNERDRAW,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        CW_USEDEFAULT, CW_USEDEFAULT,
        hwnd, (HMENU)ID_SEARCH_BTN, app.getApplicationHandle(), NULL);
#endif

    g_oldEditWndProc=(WNDPROC)SetWindowLong(g_hwndEdit, GWL_WNDPROC, (LONG)EditWndProc);

    SetEditWinText(g_hwndEdit, _T(""));

    g_hwndScroll = CreateWindow(
        TEXT("scrollbar"),
        NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP| SBS_VERT,
        0,0, CW_USEDEFAULT, CW_USEDEFAULT, hwnd,
        (HMENU) ID_SCROLL,
        app.getApplicationHandle(),
        NULL);

    SetScrollBar(g_about);

    SetMenu(hwnd);
    SetFocus(g_hwndEdit);
}

static void DoHandleInvalidRegCode(HWND hwnd)
{
    g_fRegistration = false;
    SetUIState(true);
    int res = MessageBox(hwnd, 
        _T("Wrong registration code. Please contact support@arslexis.com if problem persists.\n\nRe-enter the code?"),
        _T("Wrong reg code"), MB_YESNO | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );

    if (IDNO==res)
    {
        // not re-entering the reg code - clear it out
        // (since it was invalid)
        iPediaApplication::Preferences& prefs = GetPrefs();
        prefs.regCode = _T("");
        iPediaApplication &app = GetApp();
        app.savePreferences();
    }
    else
    {
        DoRegister(hwnd, g_newRegCode);
    }
}

static void DoHandleValidRegCode(HWND hwnd)
{
    SetUIState(true);
    g_fRegistration = false;
    assert(!g_newRegCode.empty());
    // TODO: assert that it consists of numbers only
    iPediaApplication::Preferences& prefs = GetPrefs();
    if (g_newRegCode!=prefs.regCode)
    {
        assert(g_newRegCode.length()<=prefs.regCodeLength);
        prefs.regCode = g_newRegCode;
        iPediaApplication &app = GetApp();
        app.savePreferences();
    }   

    MessageBox(hwnd, 
        _T("Thank you for registering iPedia."), 
        _T("Registration successful"), 
        MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
}

static void DoHandleArticleBody()
{   
    iPediaApplication &app = GetApp();
    LookupManager *lookupManager = app.getLookupManager();
    assert(lookupManager!=0);
    if (lookupManager)
    {
        g_definition->replaceElements(lookupManager->lastDefinitionElements());
        g_forceLayoutRecalculation=true;
        SetEditWinText(g_hwndEdit,lookupManager->lastSearchTerm());
        SendMessage(g_hwndEdit, EM_SETSEL, 0, -1);
    }
    SetScrollBar(g_definition);
    SetDisplayMode(showArticle);
    SetFocus(g_hwndEdit);
    InvalidateRect(app.getMainWindow(), NULL, FALSE);
}

using ArsLexis::EventData;

static EventData ExtractEventData(WPARAM wp, LPARAM lp)
{
    EventData eventData;
    eventData.wParam=wp;
    eventData.lParam=lp;
    return eventData;
}

static LookupFinishedEventData ExtractLookupFinishedEventData(WPARAM wp, LPARAM lp)
{
    EventData eventData = ExtractEventData(wp,lp);
    LookupFinishedEventData data;
    memcpy((void*)&data, (void*)&eventData, sizeof(data));
    return data;
}

static DisplayAlertEventData ExtractDisplayAlertEventData(WPARAM wp, LPARAM lp)
{
    EventData eventData = ExtractEventData(wp,lp);
    DisplayAlertEventData data;
    memcpy((void*)&data, (void*)&eventData, sizeof(data));
    return data;
}

static void DoHandleLookupFinished(HWND hwnd, WPARAM wp, LPARAM lp)
{
    iPediaApplication& app = GetApp();
    LookupManager* lookupManager = app.getLookupManager(true);

    LookupFinishedEventData data = ExtractLookupFinishedEventData(wp,lp);
    switch (data.outcome)
    {
        case LookupFinishedEventData::outcomeArticleBody:
            DoHandleArticleBody();
            break;

        case LookupFinishedEventData::outcomeList:
            SendMessage(hwnd, WM_COMMAND, IDM_MENU_RESULTS, 0);
            SetUIState(true);
            break;

        case LookupFinishedEventData::outcomeRegCodeValid:
            DoHandleValidRegCode(hwnd);
            break;

        case LookupFinishedEventData::outcomeRegCodeInvalid:
            DoHandleInvalidRegCode(hwnd);
            break;
    }

    SetupAboutWindow();
    lookupManager->handleLookupFinishedInForm(data);
    SetMenu(hwnd);
    InvalidateRect(hwnd,NULL,FALSE);
}

static void DoDisplayAlert(HWND hwnd, WPARAM wp, LPARAM lp, bool fCustom)
{
    iPediaApplication& app=GetApp();

    DisplayAlertEventData data = ExtractDisplayAlertEventData(wp, lp);

    String msg;
    app.getErrorMessage(data.alertId, fCustom, msg);

    const char_t *title = getErrorTitle(data.alertId);

    if (lookupLimitReachedAlert == data.alertId)
    {
        int res = MessageBox(app.getMainWindow(), msg.c_str(), title,
                  MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_SETFOREGROUND );
        if (IDYES == res)
        {
            SendMessage(hwnd, WM_COMMAND, IDM_MENU_REGISTER, 0);
        }
    }
    else
    {
        // we need to do make it MB_APPLMODAL - if we don't if we switch
        // to other app and return here, the dialog is gone but the app
        // is blocked
        MessageBox(app.getMainWindow(), msg.c_str(), title,
                   MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_SETFOREGROUND );
    }

    SetUIState(true);
}

static void OnSize(HWND hwnd, LPARAM lp)
{
    int dx = LOWORD(lp);
    int dy = HIWORD(lp);

#ifdef WIN32_PLATFORM_PSPC
    int searchButtonDX = 50;
    int searchButtonX = dx - searchButtonDX - 2;

    MoveWindow(g_hwndSearchButton, searchButtonX, 2, searchButtonDX, 20, TRUE);
    MoveWindow(g_hwndEdit, 2, 2, searchButtonX - 6, 20, TRUE);
#else
    MoveWindow(g_hwndEdit, 2, 2, dx-4, 20, TRUE);
#endif

    int scrollStartY = 24;
    int scrollDy = dy - scrollStartY - 2;
    MoveWindow(g_hwndScroll, dx-GetScrollBarDx(), scrollStartY, GetScrollBarDx(), scrollDy, FALSE);

    g_progressRect.left = (dx - GetScrollBarDx() - 155)/2;
    g_progressRect.top  = (dy-45)/2;
    g_progressRect.right = g_progressRect.left + 155;
#ifdef WIN32_PLATFORM_PSPC
    g_progressRect.bottom = g_progressRect.top + 50;
#else
    g_progressRect.bottom = g_progressRect.top + 45;
#endif
    g_RenderingProgressReporter->setProgressArea(g_progressRect);
    g_RegistrationProgressReporter->setProgressArea(g_progressRect);
}

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    iPediaApplication& app=GetApp();
    if (WM_KEYDOWN==msg)
    {
        if (VK_TACTION==wp)
        {
            if (g_uiEnabled)
            {
                SendMessage(app.getMainWindow(),WM_COMMAND, ID_SEARCH, 0);
            }
            return 0;
        } else if (VK_DOWN==wp)
        {
            ScrollDefinition(1, scrollPage, true);
            return 0;
        } else if (VK_UP==wp)
        {
            ScrollDefinition(-1, scrollPage, true);
            return 0;
        }
    }
    return CallWindowProc(g_oldEditWndProc, hwnd, msg, wp, lp);
}

static void OnScroll(WPARAM wp)
{
    int code = LOWORD(wp);
    switch (code)
    {
        case SB_TOP:
            ScrollDefinition(0, scrollHome, false);
            break;
        case SB_BOTTOM:
            ScrollDefinition(0, scrollEnd, false);
            break;
        case SB_LINEUP:
            ScrollDefinition(-1, scrollLine, false);
            break;
        case SB_LINEDOWN:
            ScrollDefinition(1, scrollLine, false);
            break;
        case SB_PAGEUP:
            ScrollDefinition(-1, scrollPage, false);
            break;
        case SB_PAGEDOWN:
            ScrollDefinition(1, scrollPage, false);
            break;

        case SB_THUMBPOSITION:
        {
            SCROLLINFO info = {0};
            info.cbSize = sizeof(info);
            info.fMask = SIF_TRACKPOS;
            GetScrollInfo(g_hwndScroll, SB_CTL, &info);
            ScrollDefinition(info.nTrackPos, scrollPosition, true);
        }
     }
}

static void DoForceUpgrade(HWND hwnd)
{           
    int res = MessageBox(hwnd, 
        _T("You need to upgrade iPedia to a newer version. Upgrade now?"),
        _T("Upgrade required"),
        MB_YESNO | MB_APPLMODAL | MB_SETFOREGROUND );
    if (IDYES == res)
        GotoURL(_T("http://arslexis.com/updates/sm-ipedia-1-0.html"));
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT     lResult = TRUE;
    iPediaApplication& app = GetApp();

    // I don't know why on PPC in first WM_SIZE mesaage hieght of menu
    // bar is not taken into account, in next WM_SIZE messages (e.g. 
    // after SIP usage) the height of menu is taken into account
    switch (msg)
    {
        case WM_CREATE:
            OnCreate(hwnd);
            break;

        case WM_SETTINGCHANGE:
            OnSettingChange(hwnd, wp, lp);
            break;

        case WM_ACTIVATE:
            OnActivateMain(hwnd, wp, lp);
            break;

        case WM_SIZE:
            OnSize(hwnd, lp);
            break;

        case WM_SETFOCUS:
            SetFocus(g_hwndEdit);
            break;

        case WM_COMMAND:
            OnCommand(hwnd, msg, wp, lp);
            break;

        case WM_PAINT:
            OnPaint(hwnd);
            break;

#ifdef WIN32_PLATFORM_WFSP
        case WM_HOTKEY:
            SHSendBackToFocusWindow(msg, wp, lp);
            break;
#endif
        
        case WM_LBUTTONDOWN:
            g_lbuttondown = true;
            HandleExtendSelection(hwnd,LOWORD(lp), HIWORD(lp), false);
            break;
        
        case WM_MOUSEMOVE:
            if (g_lbuttondown)
                HandleExtendSelection(hwnd,LOWORD(lp), HIWORD(lp), false);
            break;
        
        case WM_LBUTTONUP:
            g_lbuttondown = false;
            HandleExtendSelection(hwnd,LOWORD(lp), HIWORD(lp), true);
            break;

        case WM_VSCROLL:
            OnScroll(wp);
            break;

        case WM_DESTROYCLIPBOARD:
            FreeClipboardData();
            break;

        case iPediaApplication::appForceUpgrade:
            DoForceUpgrade(hwnd);
            break;

        case iPediaApplication::appDisplayCustomAlertEvent:
            DoDisplayAlert(hwnd, wp, lp, true);
            break;
        
        case iPediaApplication::appDisplayAlertEvent:
            DoDisplayAlert(hwnd, wp,lp, false);
            break;

        case LookupManager::lookupStartedEvent:
        case LookupManager::lookupProgressEvent:
            InvalidateRect(app.getMainWindow(), &g_progressRect, FALSE);
            break;

        case LookupManager::lookupFinishedEvent:
            DoHandleLookupFinished(hwnd, wp, lp);
            break;            

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            lResult = DefWindowProc(hwnd, msg, wp, lp);
            break;
    }
    return lResult;
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPWSTR     lpCmdLine,
                   int        CmdShow)

{
    // if we're already running, bring our window to front and exit
    HWND hwndExisting = FindWindow(APP_NAME, NULL);
    if (hwndExisting) 
    {
        // without | 0x01 we would block if MessageBox was displayed
        // I haven't seen it documented anywhere but official MSDN examples do that:
        // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/win_ce/html/pwc_ASmartphoneApplication.asp
        SetForegroundWindow ((HWND)(((DWORD)hwndExisting) | 0x01));    
        return 0;
    }

    String cmdLine(lpCmdLine);
    iPediaApplication& app = GetApp();
    if (!app.initApplication(hInstance, hPrevInstance, cmdLine, CmdShow))
        return FALSE;

    SetupAboutWindow();
    SetMenu(app.getMainWindow());

    int retVal = app.runEventLoop();

    DeinitDataConnection();
    
    delete g_definition;
    delete g_about;
    delete g_register;
    delete g_tutorial;
    delete g_wikipedia;

    return retVal;
}

void ArsLexis::handleBadAlloc()
{
    RaiseException(1,0,0,NULL);    
}

