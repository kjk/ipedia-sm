#include <windows.h>
#include <aygshell.h>
#include <wingdi.h>

#include <objbase.h>
#include <initguid.h>
#include <connmgr.h>
#include <sms.h>
#include <uniqueid.h>

#ifdef WIN32_PLATFORM_WFSP
#include <tpcshell.h>
#endif

#include <SysUtils.hpp>
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

#include "ProgressReporters.h"
#include "LastResults.h"
#include "Hyperlinks.h"
#include "DefaultArticles.h"
#include "sm_ipedia.h"
#include "ipedia_rsc.h"

using ArsLexis::String;
using ArsLexis::Graphics;
using ArsLexis::char_t;

// holds new registration code being checked while we send the request to
// server to check it.
// TODO: we should stick it in preferences
ArsLexis::String g_newRegCode;

bool g_fRegistration = false;

int  g_scrollBarDx = 0;
int  g_menuDy = 0;
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

GenericTextElement* g_articleCountElement = NULL;
long g_articleCountSet = -1;

bool  g_forceLayoutRecalculation = false;
bool  g_forceAboutRecalculation  = false;
bool  g_recalculationInProgress = false;
DWORD g_recalculationData  = 0;

RenderingProgressReporter* rep;
LRESULT handleMenuCommand(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void CopyToClipboard();

static int  g_stressModeCnt = 0;

bool g_uiEnabled = true;

String g_recentWord;
String g_searchWord;

HWND      g_hwndEdit         = NULL;
HWND      g_hwndScroll       = NULL;
HWND      g_hWndMenuBar      = NULL;   // Current active menubar
HWND      g_hwndSearchButton = NULL;
WNDPROC   g_oldEditWndProc   = NULL;

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void paint(HWND hwnd, HDC hdc, RECT rcpaint);
void handleExtendSelection(HWND hwnd, int x, int y, bool finish);

void repaintDefiniton(int scrollDelta = 0);
void moveHistoryForward();
void moveHistoryBack();
void setScrollBar(Definition* definition);
void scrollDefinition(int units, ScrollUnit unit, bool updateScrollbar);
void SimpleOrExtendedSearch(HWND hwnd, bool simple);

DWORD WINAPI formattingThreadProc(LPVOID lpParameter)
{
    repaintDefiniton();
    g_recalculationInProgress = false;
    return 0;
}

DisplayMode displayMode()
{return g_displayMode;}

void setDisplayMode(DisplayMode displayMode)
{g_displayMode=displayMode;}

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

HANDLE    g_hConnection = NULL;
// try to establish internet connection.
// If can't (e.g. because tcp/ip stack is not working), display a dialog box
// informing about that and return false
// Return true if established connection.
// Can be called multiple times - will do nothing if connection is already established.
static bool fInitConnection()
{
#ifdef WIN32_PLATFORM_PSPC
    return true; // not needed on Pocket PC
#endif
    if (NULL!=g_hConnection)
        return true;
    iPediaApplication &app = iPediaApplication::instance();
    CONNMGR_CONNECTIONINFO ccInfo;
    memset(&ccInfo, 0, sizeof(CONNMGR_CONNECTIONINFO));
    ccInfo.cbSize      = sizeof(CONNMGR_CONNECTIONINFO);
    ccInfo.dwParams    = CONNMGR_PARAM_GUIDDESTNET;
    ccInfo.dwFlags     = CONNMGR_FLAG_PROXY_HTTP;
    ccInfo.dwPriority  = CONNMGR_PRIORITY_USERINTERACTIVE;
    ccInfo.guidDestNet = IID_DestNetInternet;
    
    DWORD dwStatus  = 0;
    DWORD dwTimeout = 5000;     // connection timeout: 5 seconds
    HRESULT res = ConnMgrEstablishConnectionSync(&ccInfo, &g_hConnection, dwTimeout, &dwStatus);

    if (FAILED(res))
    {
        //assert(NULL==g_hConnection);
        g_hConnection = NULL;
    }

    if (NULL==g_hConnection)
    {
#ifdef DEBUG
        ArsLexis::String errorMsg = _T("Unable to connect to ");
        errorMsg += iPediaApplication::instance().server();
#else
        ArsLexis::String errorMsg = _T("Unable to connect");
#endif
        errorMsg.append(_T(". Verify your dialup or proxy settings are correct, and try again."));
        MessageBox(app.getMainWindow(), errorMsg.c_str(), _T("Error"), MB_OK | MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND );
        return false;
    }
    else
        return true;
}

static void deinitConnection()
{
    if (NULL != g_hConnection)
    {
        ConnMgrReleaseConnection(g_hConnection,1);
        g_hConnection = NULL;
    }
}

void OnCreate(HWND hwnd)
{
    iPediaApplication &app = iPediaApplication::instance();
    g_scrollBarDx = GetSystemMetrics(SM_CXVSCROLL);
    g_menuDy = 0;
#ifdef WIN32_PLATFORM_PSPC
    g_menuDy = GetSystemMetrics(SM_CYMENU);
#endif

    // create the menu bar
    SHMENUBARINFO mbi;
    ZeroMemory(&mbi, sizeof(SHMENUBARINFO));
    mbi.cbSize = sizeof(SHMENUBARINFO);
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
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON ,//| BS_OWNERDRAW,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        CW_USEDEFAULT, CW_USEDEFAULT,
        hwnd, (HMENU)ID_SEARCH_BTN, app.getApplicationHandle(), NULL);
#endif

    g_oldEditWndProc=(WNDPROC)SetWindowLong(g_hwndEdit, GWL_WNDPROC, (LONG)EditWndProc);
    
    SendMessage(g_hwndEdit, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)_T(""));

    g_hwndScroll = CreateWindow(
        TEXT("scrollbar"),
        NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP| SBS_VERT,
        0,0, CW_USEDEFAULT, CW_USEDEFAULT, hwnd,
        (HMENU) ID_SCROLL,
        app.getApplicationHandle(),
        NULL);

    setScrollBar(g_about);
    (void)SendMessage(
        mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY)
        );

    app.setMainWindow(hwnd);

    LookupManager* lookupManager=app.getLookupManager(true);
    lookupManager->setProgressReporter(new DownloadingProgressReporter());
    g_RenderingProgressReporter = new RenderingProgressReporter(hwnd, g_progressRect, String(_T("Formatting article...")));
    g_definition->setRenderingProgressReporter(g_RenderingProgressReporter);
    g_definition->setHyperlinkHandler(&app.hyperlinkHandler());
    
    g_RegistrationProgressReporter = new RenderingProgressReporter(hwnd, g_progressRect, String(_T("Registering application...")));


    g_articleCountSet = app.preferences().articleCount;
    
    g_about->setHyperlinkHandler(&app.hyperlinkHandler());
    g_tutorial->setHyperlinkHandler(&app.hyperlinkHandler());
    g_register->setHyperlinkHandler(&app.hyperlinkHandler());
    g_wikipedia->setHyperlinkHandler(&app.hyperlinkHandler());

    prepareAbout();
    prepareHowToRegister();
    prepareTutorial();
    prepareWikipedia();

    setMenu(hwnd);
    SetFocus(g_hwndEdit);
}

static void OnRegister(HWND hwnd)
{
    if (!fInitConnection())
        return;

    String newRegCode;
    iPediaApplication& app=GetApp();
    String oldRegCode = app.preferences().regCode;

    bool fOk = FGetRegCodeFromUser(hwnd, oldRegCode, newRegCode);
    if (!fOk)
        return;

    assert(!g_newRegCode.empty());

    g_newRegCode = newRegCode;

    LookupManager* lookupManager = app.getLookupManager(true);
    if (lookupManager && !lookupManager->lookupInProgress())
    {
        lookupManager->verifyRegistrationCode(g_newRegCode);
        setUIState(false);
        g_fRegistration = true;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT     lResult = TRUE;
    HDC         hdc;
    bool        customAlert = false;
    // I don't know why on PPC in first WM_SIZE mesaage hieght of menu
    // bar is not taken into account, in next WM_SIZE messages (e.g. 
    // after SIP usage) the height of menu is taken into account
    static      firstWmSizeMsg = true;
    switch(msg)
    {
        case WM_CREATE:
            OnCreate(hwnd);
            break;

        case WM_SETTINGCHANGE:
        {
            SHACTIVATEINFO sai;
            if (SPI_SETSIPINFO == wp){
                memset(&sai, 0, sizeof(SHACTIVATEINFO));
                SHHandleWMSettingChange(hwnd, -1 , 0, &sai);
            }
            break;
        }
        case WM_ACTIVATE:
        {
            SHACTIVATEINFO sai;
            if (SPI_SETSIPINFO == wp){
                memset(&sai, 0, sizeof(SHACTIVATEINFO));
                SHHandleWMActivate(hwnd, wp, lp, &sai, 0);
            }
            break;
        }
        case WM_SIZE:
        {
            if (!firstWmSizeMsg)
                g_menuDy = 0;
            int height = HIWORD(lp);
            int width = LOWORD(lp);
            
#ifdef WIN32_PLATFORM_PSPC
            int searchButtonDX = 50;
            int searchButtonX = LOWORD(lp) - searchButtonDX - 2;

            MoveWindow(g_hwndSearchButton, searchButtonX, 2, searchButtonDX, 20, TRUE);
            MoveWindow(g_hwndEdit, 2, 2, searchButtonX - 6, 20, TRUE);
#else
            MoveWindow(g_hwndEdit, 2, 2, LOWORD(lp)-4, 20, TRUE);
#endif

            MoveWindow(g_hwndScroll,width-g_scrollBarDx , 28 , g_scrollBarDx, height-28-g_menuDy, FALSE);

            g_progressRect.left = (width - g_scrollBarDx - 155)/2;
            g_progressRect.top = (height-45)/2;
            g_progressRect.right = g_progressRect.left + 155;   
            g_progressRect.bottom = g_progressRect.top + 45;  
            g_RenderingProgressReporter->setProgressArea(g_progressRect);
            g_RegistrationProgressReporter->setProgressArea(g_progressRect);
            firstWmSizeMsg = false;
            break;
        }
        case WM_SETFOCUS:
            SetFocus(g_hwndEdit);
            break;

        case WM_COMMAND:
            handleMenuCommand(hwnd, msg, wp, lp);
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT	ps;
            hdc = BeginPaint (hwnd, &ps);
            paint(hwnd, hdc, ps.rcPaint);
            EndPaint (hwnd, &ps);
            break;
        }
        
        case WM_HOTKEY:
        {
            iPediaApplication &app = iPediaApplication::instance();
            HWND hwnd = app.getMainWindow();
            Graphics gr(GetDC(hwnd), hwnd);
            switch(HIWORD(lp))
            {
#ifdef WIN32_PLATFORM_WFSP
                case VK_TBACK:
                    if ( 0 != (MOD_KEYUP & LOWORD(lp)))
                        SHSendBackToFocusWindow( msg, wp, lp );
                    break;
#endif
                case VK_TDOWN:
                    scrollDefinition(1, scrollPage, true);
                    break;
            }
            break;
        }    
        
        case WM_LBUTTONDOWN:
            g_lbuttondown = true;
            handleExtendSelection(hwnd,LOWORD(lp), HIWORD(lp), false);
            break;
        
        case WM_MOUSEMOVE:
            if (g_lbuttondown)
                handleExtendSelection(hwnd,LOWORD(lp), HIWORD(lp), false);
            break;
        
        case WM_LBUTTONUP:
            g_lbuttondown = false;
            handleExtendSelection(hwnd,LOWORD(lp), HIWORD(lp), true);
            break;
        case WM_VSCROLL:
        {
            switch (LOWORD(wp))
            {
                case SB_TOP:
                    scrollDefinition(0, scrollHome, false);
                    break;
                case SB_BOTTOM:
                    scrollDefinition(0, scrollEnd, false);
                    break;
                case SB_LINEUP:
                    scrollDefinition(-1, scrollLine, false);
                    break;
                case SB_LINEDOWN:
                    scrollDefinition(1, scrollLine, false);
                    break;
                case SB_PAGEUP:
                    scrollDefinition(-1, scrollPage, false);
                    break;
                case SB_PAGEDOWN:
                    scrollDefinition(1, scrollPage, false);
                    break;
                case SB_THUMBPOSITION:
                {
                    SCROLLINFO info;
                    ZeroMemory(&info, sizeof(SCROLLINFO));
                    info.cbSize = sizeof(info);
                    info.fMask = SIF_TRACKPOS;
                    GetScrollInfo(g_hwndScroll, SB_CTL, &info);
                    scrollDefinition(info.nTrackPos, scrollPosition, true);
                    break;
                }
             }
            break;
        }

        case iPediaApplication::appForceUpgrade:
        {           
            int res = MessageBox(hwnd, 
                _T("You need to upgrade iPedia to a newer version. You can do it right now or later using 'Options/Check for updates'. Would you like to do it now ?"),
                _T("Upgrade required"),
                MB_YESNO | MB_APPLMODAL | MB_SETFOREGROUND );
            if (IDYES == res)
                GotoURL(_T("http://arslexis.com/updates/sm-ipedia-1-0.html"));
            break;
        }

        case iPediaApplication::appDisplayCustomAlertEvent:
            customAlert = true;
            // intentional fall-through
        
        case iPediaApplication::appDisplayAlertEvent:
        {
            iPediaApplication& app=iPediaApplication::instance();
            iPediaApplication::DisplayAlertEventData data;
            ArsLexis::EventData i;
            i.wParam=wp; i.lParam=lp;
            memcpy(&data, &i, sizeof(data));
            
            
            String msg;
            app.getErrorMessage(data.alertId, customAlert,msg);
            String title;
            app.getErrorTitle(data.alertId, title);

            if (lookupLimitReachedAlert == data.alertId)
            {
                int res = MessageBox(app.getMainWindow(), msg.c_str(), title.c_str(),
                          MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_SETFOREGROUND );
                if (IDYES == res)
                    SendMessage(hwnd, WM_COMMAND, IDM_MENU_REGISTER, 0);
            }
            else
            {
                // we need to do make it MB_APPLMODAL - if we don't if we switch
                // to other app and return here, the dialog is gone but the app
                // is blocked
                MessageBox(app.getMainWindow(), msg.c_str(), title.c_str(),
                           MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_SETFOREGROUND );
            }

            setUIState(true);
            break;
        }

        case LookupManager::lookupStartedEvent:
        case LookupManager::lookupProgressEvent:
        {
            iPediaApplication& app=iPediaApplication::instance();
            InvalidateRect(app.getMainWindow(), &g_progressRect, FALSE);
            break;
        }

        case LookupManager::lookupFinishedEvent:
        {
            iPediaApplication& app=iPediaApplication::instance();
            LookupManager* lookupManager=app.getLookupManager(true);
            LookupFinishedEventData data;
            ArsLexis::EventData i;
            i.wParam=wp; i.lParam=lp;
            memcpy(&data, &i, sizeof(data));
            switch (data.outcome)
            {
                case LookupFinishedEventData::outcomeArticleBody:
                {   
                    assert(lookupManager!=0);
                    if (lookupManager)
                    {
                        g_definition->replaceElements(lookupManager->lastDefinitionElements());
                        g_forceLayoutRecalculation=true;
                        SendMessage(g_hwndEdit, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)lookupManager->lastInputTerm().c_str());
                        //int len = SendMessage(g_hwndEdit, EM_LINELENGTH, 0,0);
                        SendMessage(g_hwndEdit, EM_SETSEL, 0, -1);
                    }
                    setScrollBar(g_definition);
                    setDisplayMode(showArticle);
                    SetFocus(g_hwndEdit);
                    InvalidateRect(app.getMainWindow(), NULL, FALSE);
                    break;
                }

                case LookupFinishedEventData::outcomeList:
                    g_recentWord.assign(lookupManager->lastInputTerm());
                    SendMessage(hwnd, WM_COMMAND, IDM_MENU_RESULTS, 0);
                    setUIState(true);
                    break;

                case LookupFinishedEventData::outcomeRegCodeValid:
                {
                    setUIState(true);
                    g_fRegistration = false;
                    assert(!g_newRegCode.empty());
                    // TODO: assert that it consists of numbers only
                    iPediaApplication::Preferences& prefs=iPediaApplication::instance().preferences();
                    if (g_newRegCode!=prefs.regCode)
                    {
                        assert(g_newRegCode.length()<=prefs.regCodeLength);
                        prefs.regCode = g_newRegCode;
                        app.savePreferences();
                    }   
                    MessageBox(hwnd, 
                        _T("Thank you for registering iPedia."), 
                        _T("Registration successful"), 
                        MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
                    break;
                }

                case LookupFinishedEventData::outcomeRegCodeInvalid:
                {
                    g_fRegistration = false;
                    setUIState(true);
                    iPediaApplication::Preferences& prefs=iPediaApplication::instance().preferences();
                    int res = MessageBox(hwnd, 
                        _T("Wrong registration code. Please contact support@arslexis.com if problem persists.\n\nRe-enter the code?"),
                        _T("Wrong reg code"), MB_YESNO | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );

                    if (IDNO==res)
                    {
                        // not re-entering the reg code - clear it out
                        // (since it was invalid)
                        prefs.regCode = _T("");
                        app.savePreferences();
                    }
                    else
                    {
                        OnRegister(hwnd);
                        break;
                    }
                }
            }   
            setupAboutWindow();
            lookupManager->handleLookupFinishedInForm(data);
            setMenu(hwnd);
            InvalidateRect(hwnd,NULL,FALSE);
        }
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

LRESULT handleMenuCommand(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager(true);

    if ( (NULL==lookupManager) || lookupManager->lookupInProgress())
        return TRUE;

    LRESULT lResult = TRUE;

    switch (wp)
    {   
        case IDM_MENU_CLIPBOARD:
            CopyToClipboard();
            break;
        
        case IDM_MENU_STRESS_MODE:
            g_stressModeCnt = 100;
            SendMessage(hwnd, WM_COMMAND, IDM_MENU_RANDOM, 0);
            break;
        
        case IDM_ENABLE_UI:
            setUIState(true);
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
            // Try to open hyperlink
            GotoURL(_T("http://arslexis.com/pda/sm.html"));
            break;
        
        case IDM_MENU_UPDATES:
            GotoURL(_T("http://arslexis.com/updates/sm-ipedia-1-0.html"));
            break;
        
        case IDM_MENU_ABOUT:
            setDisplayMode(showAbout);
            setMenu(hwnd);
            setScrollBar(g_about);
            InvalidateRect(hwnd,NULL,FALSE);
            break;
        
        case IDM_MENU_TUTORIAL:
            setDisplayMode(showTutorial);
            setMenu(hwnd);
            setScrollBar(g_about);
            InvalidateRect(hwnd,NULL,FALSE);
            break;
        
        case IDM_EXT_SEARCH:
            SimpleOrExtendedSearch(hwnd, false);
            break;
        
        case ID_SEARCH_BTN:
        // intentional fall-through
        case ID_SEARCH_BTN2:
        // intentional fall-through
        case ID_SEARCH:
        // intentional fall-through
            SimpleOrExtendedSearch(hwnd, true);
            break;
        
        case IDM_MENU_HYPERS:
        {
            // TODO: do we need fInitConnection() here?
            if (!fInitConnection())
                break;
            Definition &def = currentDefinition();
            if (!def.empty())
                DialogBox(app.getApplicationHandle(), MAKEINTRESOURCE(IDD_HYPERLINKS), hwnd,HyperlinksDlgProc);
            break;
        }
        case IDM_MENU_RANDOM:
        {
            if (!fInitConnection())
                break;
            if (lookupManager && !lookupManager->lookupInProgress())
            {
                setUIState(false);
                lookupManager->lookupRandomTerm();
            }
            break;
        }
        case IDM_MENU_REGISTER:
        {
            OnRegister(hwnd);
            break;
        }
        case IDM_MENU_PREV:
            // intentionally fall through
        case ID_PREV_BTN:
            moveHistoryBack();
            break;

        case IDM_MENU_NEXT:
            // intentionally fall through
        case ID_NEXT_BTN:
            moveHistoryForward();
            break;

        case IDM_MENU_RESULTS:
        {
            int res=DialogBox(app.getApplicationHandle(), MAKEINTRESOURCE(IDD_LAST_RESULTS), hwnd,LastResultsDlgProc);
            if (res)
            {
                if (lookupManager && !lookupManager->lookupInProgress())
                {
                    if (1==res)
                        lookupManager->lookupIfDifferent(g_searchWord);
                    else
                        lookupManager->search(g_searchWord);
                    setUIState(false);
                }
                InvalidateRect(hwnd,NULL,FALSE);
            }
            break;
        }
        default:
        lResult  = DefWindowProc(hwnd, msg, wp, lp);
    }
    return lResult;
}
        

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPWSTR     lpCmdLine,
                   int        CmdShow)

{
    iPediaApplication& app = iPediaApplication::instance();
    // if we're already running, then just bring our window to front
    String cmdLine(lpCmdLine);
    if (app.initApplication(hInstance, hPrevInstance, cmdLine, CmdShow))
    {
        int retVal = app.runEventLoop();

        deinitConnection();
        return retVal;
    }
    return FALSE;

}

void setScrollBar(Definition* definition)
{
    int first=0;
    int total=0;
    int shown=0;
    if ((NULL!=definition) && (!definition->empty()))
    {
        first=definition->firstShownLine();
        total=definition->totalLinesCount();
        shown=definition->shownLinesCount();
    }
    SetScrollPos(g_hwndScroll, SB_CTL, first, TRUE);
    SetScrollRange(g_hwndScroll, SB_CTL, 0, total-shown, TRUE);
}

void ArsLexis::handleBadAlloc()
{
    RaiseException(1,0,0,NULL);    
}

void paint(HWND hwnd, HDC hdc, RECT rcpaint)
{
    RECT rect;
    GetClientRect (hwnd, &rect);
    ArsLexis::Rectangle rin(rcpaint);
    ArsLexis::Rectangle rout(g_progressRect);
    
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager(true);

    bool onlyProgress=false;

    if (lookupManager && lookupManager->lookupInProgress() &&
        (rout && rin.topLeft) && (rout.extent.x>=rin.extent.x)
        && (rout.extent.y>=rin.extent.y))
    {
            onlyProgress = true;
    }

    rect.top    += 22;
    rect.left   += 2;
    rect.right  -= (2+g_scrollBarDx);
    rect.bottom -= (2+g_menuDy);
    Graphics gr(hdc, hwnd);

    if ( !onlyProgress && !g_recalculationInProgress)
    {
        Definition &def = currentDefinition();
        if (g_forceLayoutRecalculation)
        {
            DWORD threadID;
            g_recalculationInProgress = true;
            CreateThread(NULL, 0, formattingThreadProc, &g_recalculationData, 0, &threadID);
        }
        else
            repaintDefiniton();
    }

    if (lookupManager && lookupManager->lookupInProgress())
    {

        if (g_fRegistration)
        {
            g_RegistrationProgressReporter->reportProgress(-1);
        }
        else
        {
            Graphics gr(GetDC(app.getMainWindow()), app.getMainWindow());    
            ArsLexis::Rectangle progressArea(g_progressRect);
            lookupManager->showProgress(gr, progressArea);
        }
    }
}
            
LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    iPediaApplication& app=iPediaApplication::instance();
    switch(msg)
    {
        case WM_KEYDOWN:
        {
            switch (wp) 
            {
                case VK_TACTION:
                    if (g_uiEnabled)
                    {
                        SendMessage(app.getMainWindow(),WM_COMMAND, ID_SEARCH, 0);
                    }
                    return 0; 
                case VK_DOWN:
                    scrollDefinition(1, scrollPage, true);
                    return 0;
                case VK_UP:
                    scrollDefinition(-1, scrollPage, true);
                    return 0;
            }
            break;
        }
        /*
        case WM_SETFOCUS:
        {            
            SHSipPreference(app.getMainWindow(), SIP_UP);
            break;
        }
        case WM_KILLFOCUS:
        {
            SHSipPreference(app.getMainWindow(), SIP_DOWN);
            break;
        }*/
    }
    return CallWindowProc(g_oldEditWndProc, hwnd, msg, wp, lp);
}

static void SetMenuBarButtonState(int buttonID, bool state)
{
    TBBUTTONINFO    buttonInfo;
    
    ZeroMemory( &buttonInfo, sizeof( TBBUTTONINFO ) );
    buttonInfo.cbSize   = sizeof( TBBUTTONINFO );
    buttonInfo.dwMask   = TBIF_STATE;
    if (state)
        buttonInfo.fsState = TBSTATE_ENABLED;
    else
        buttonInfo.fsState = TBSTATE_INDETERMINATE;
    SendMessage( g_hWndMenuBar, TB_SETBUTTONINFO, buttonID, (LPARAM)(LPTBBUTTONINFO)&buttonInfo );
}

void setMenu(HWND hwnd)
{
    Definition &def = currentDefinition();
    HWND hwndMB = SHFindMenuBar (hwnd);
    if (!hwndMB) 
        return;

    bool isNextHistoryElement = false;
    HMENU hMenu;
    hMenu = (HMENU)SendMessage (hwndMB, SHCMBM_GETSUBMENU, 0, ID_MENU_BTN);
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager(true);
    
    unsigned int lastSearchResultEnabled = MF_GRAYED;
    if (!lookupManager->lastSearchResults().empty())
        lastSearchResultEnabled = MF_ENABLED;
    EnableMenuItem(hMenu, IDM_MENU_RESULTS, lastSearchResultEnabled);
    
    unsigned int nextHistoryTermEnabled = MF_GRAYED;
    if (lookupManager->hasNextHistoryTerm())
        nextHistoryTermEnabled = MF_ENABLED;
    EnableMenuItem(hMenu, IDM_MENU_NEXT, nextHistoryTermEnabled);

    unsigned int previousHistoryTermEnabled = MF_GRAYED;
    if (lookupManager->hasPreviousHistoryTerm()||
        ( (displayMode()!=showArticle)&&(!g_definition->empty()) )
       )
        previousHistoryTermEnabled = MF_ENABLED;

    EnableMenuItem(hMenu, IDM_MENU_PREV, previousHistoryTermEnabled);
    
    unsigned int clipboardEnabled = MF_GRAYED;
    if (!def.empty())
        clipboardEnabled = MF_ENABLED;
    EnableMenuItem(hMenu, IDM_MENU_CLIPBOARD, clipboardEnabled);

#ifdef WIN32_PLATFORM_PSPC
    //SetMenuBarButtonState(ID_SEARCH_BTN2, g_uiEnabled);
    if (g_uiEnabled)
    {
        SetMenuBarButtonState(ID_NEXT_BTN, lookupManager->hasNextHistoryTerm());
        SetMenuBarButtonState(ID_PREV_BTN, 
            lookupManager->hasPreviousHistoryTerm()||
            ((displayMode()!=showArticle)&&(!g_definition->empty())));
    }
#endif

    EnableMenuItem(hMenu, IDM_MENU_HYPERS, MF_GRAYED);
    Definition::ElementPosition_t pos;
    for(pos=def.firstElementPosition();
        pos!=def.lastElementPosition();
        pos++)
    {
        DefinitionElement *curr=*pos;
        if (curr->isTextElement())
        {
            GenericTextElement *txtEl=(GenericTextElement*)curr;
            if ((txtEl->isHyperlink())&&(txtEl->hyperlinkProperties()->type==hyperlinkTerm))
            {
                EnableMenuItem(hMenu, IDM_MENU_HYPERS, MF_ENABLED);
                return;
            }
        }
    }
}

void setupAboutWindow()
{
    iPediaApplication &app=iPediaApplication::instance();
    if (app.preferences().articleCount!=-1)
    {
        g_articleCountSet = app.preferences().articleCount;
        updateArticleCountEl(app.preferences().articleCount, app.preferences().databaseTime);
        prepareAbout();
        g_forceAboutRecalculation = true;
    }    
}

void setUIState(bool enabled)
{
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
    //SetMenuBarButtonState(ID_SEARCH_BTN2, enabled);
    EnableWindow(g_hwndSearchButton, enabled);
#else
    SetMenuBarButtonState(ID_MENU_BTN, enabled);
    SetMenuBarButtonState(ID_SEARCH, enabled);
#endif

    g_uiEnabled = enabled;
}

void handleExtendSelection(HWND hwnd, int x, int y, bool finish)
{
    iPediaApplication& app=iPediaApplication::instance();
    const LookupManager* lookupManager=app.getLookupManager();
    if (lookupManager && lookupManager->lookupInProgress())
        return;
    Definition &def = currentDefinition();
    if (def.empty())
        return;
    ArsLexis::Point point(x,y);
    Graphics graphics(GetDC(hwnd), hwnd);
    def.extendSelection(graphics, app.preferences().renderingPreferences, point, finish);         
}

void scrollDefinition(int units, ScrollUnit unit, bool updateScrollbar)
{
    iPediaApplication &app = iPediaApplication::instance();
    const LookupManager* lookupManager = app.getLookupManager();
    Definition &def = currentDefinition();
    if (def.empty())
        return;
    if (lookupManager && lookupManager->lookupInProgress())
        return;
    if (g_recalculationInProgress)
        return;
    switch(unit)
    {
        case scrollPage:
            units*=(def.shownLinesCount());
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
    repaintDefiniton(units);
}

void repaintDefiniton(int scrollDelta)
{
    iPediaApplication& app = iPediaApplication::instance();
    const RenderingPreferences& prefs = app.preferences().renderingPreferences;
    Definition &def=currentDefinition();
    RECT rect;
    GetClientRect (app.getMainWindow(), &rect);
    ArsLexis::Rectangle bounds=rect;
    
    rect.top    +=22;
    rect.left   +=2;
    rect.right  -=2+g_scrollBarDx;
    rect.bottom -=2+g_menuDy;
    ArsLexis::Rectangle defRect=rect;
    
    Graphics gr(GetDC(app.getMainWindow()), app.getMainWindow());
    bool doubleBuffer=true;
    if ( (true == g_forceAboutRecalculation) && (displayMode() == showAbout) )
    {
        g_forceLayoutRecalculation = true;
        g_forceAboutRecalculation = false;
    }   
    HDC offscreenDc=::CreateCompatibleDC(gr.handle());
    if (offscreenDc) 
    {
        HBITMAP bitmap=::CreateCompatibleBitmap(gr.handle(), bounds.width(), bounds.height());
        if (bitmap) 
        {
            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
            {
                Graphics offscreen(offscreenDc, NULL);
                gr.copyArea(defRect, offscreen, defRect.topLeft);
                if (0 != scrollDelta)
                    def.scroll(offscreen, prefs, scrollDelta);
                else
                    def.render(offscreen, defRect, prefs, g_forceLayoutRecalculation);
                offscreen.copyArea(defRect, gr, defRect.topLeft);
            }
            ::SelectObject(offscreenDc, oldBitmap);
            ::DeleteObject(bitmap);
        }
        else
            doubleBuffer=false;
        ::DeleteDC(offscreenDc);
    }
    else
        doubleBuffer=false;
    if (!doubleBuffer)
        if (0 != scrollDelta)
            def.scroll(gr, prefs, scrollDelta);
        else
            def.render(gr, defRect, prefs, g_forceLayoutRecalculation);
    
    setScrollBar(&def);
    if (g_forceLayoutRecalculation)
        PostMessage(app.getMainWindow(),WM_COMMAND, IDM_ENABLE_UI, 0);
    g_forceLayoutRecalculation = false;
}

void CopyToClipboard()
{
    iPediaApplication& app=iPediaApplication::instance();

    if (g_definition->empty())
        return;
    if (!OpenClipboard(app.getMainWindow()))
        return;
    if (!EmptyClipboard())
    {
        CloseClipboard();
        return;
    }
    
    String text;
    g_definition->selectionToText(text);
    
    char_t *hLocal;
    int len = text.length();
    hLocal = (char_t*) LocalAlloc (LPTR, len*2+2);
    
    memcpy(hLocal, text.c_str(), len*2);

    SetClipboardData(CF_UNICODETEXT, hLocal);
    CloseClipboard();
}

static void handleMoveHistory(bool forward)
{
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager(true);
    if (displayMode()!=showArticle)
    {
        if (!g_definition->empty())
        {
            setDisplayMode(showArticle);
            //SendMessage(g_hwndEdit, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)lookupManager->lastInputTerm().c_str());
            //int len = SendMessage(g_hwndEdit, EM_LINELENGTH, 0,0);
            //SendMessage(g_hwndEdit, EM_SETSEL, 0, -1);
            setScrollBar(g_definition);
            InvalidateRect(app.getMainWindow(), NULL, false);
            return;
        }
    }
    if (lookupManager && !lookupManager->lookupInProgress())
    {
        if (!fInitConnection())
            return;
        lookupManager->moveHistory(forward);
        setUIState(false);
    }
}

void moveHistoryForward()
{
    handleMoveHistory(true);
}

void moveHistoryBack()
{
    handleMoveHistory(false);
}


void SimpleOrExtendedSearch(HWND hwnd, bool simple)
{
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager(true);

    if (!fInitConnection())
        return;
    int len = SendMessage(g_hwndEdit, EM_LINELENGTH, 0,0);
    TCHAR *buf = new TCHAR[len+1];
    len = SendMessage(g_hwndEdit, WM_GETTEXT, len+1, (LPARAM)buf);
    //SendMessage(g_hwndEdit, EM_SETSEL, 0, -1);
    String term(buf); 
    delete buf;
    if (term.empty()) 
        return;

    if ((NULL==lookupManager) || lookupManager->lookupInProgress())
        return;

    if (!simple)
    {
        lookupManager->search(term);
        setUIState(false);
        InvalidateRect(hwnd,NULL,FALSE);
    }
    else
    {
        if (lookupManager->lookupIfDifferent(term))
        {
            setUIState(false);
            InvalidateRect(hwnd,NULL,FALSE);
        }
        else
        {
            if (displayMode()!=showArticle)
            {
                setDisplayMode(showArticle);
                setScrollBar(g_definition);
                setUIState(true);
                InvalidateRect(hwnd,NULL,FALSE);                            
            }
        }
    }
}
