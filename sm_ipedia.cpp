#include "sm_ipedia.h"
#include "ProgressReporters.h"
#include "LastResults.h"
#include "Registration.h"
#include "Hyperlinks.h"
#include "DefaultArticles.h"

#include <SysUtils.hpp>
#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include <LookupManagerBase.hpp>
#include <LookupHistory.hpp>
#include <ipedia_rsc.h>
#include <Definition.hpp>
#include <DefinitionElement.hpp>
#include <GenericTextElement.hpp>
#include <Geometry.hpp>
#include <Debug.hpp>

#include <objbase.h>
#include <initguid.h>
#include <connmgr.h>

#include <windows.h>
#include <aygshell.h>
#ifdef WIN32_PLATFORM_WFSP
#include <tpcshell.h>
#endif
#include <wingdi.h>
#include <fonteffects.hpp>
#include <sms.h>
#include <uniqueid.h>
#include <Text.hpp>



using ArsLexis::String;
using ArsLexis::Graphics;
using ArsLexis::char_t;

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

const int ErrorsTableEntries = 25;

ErrorsTableEntry ErrorsTable[ErrorsTableEntries] =
{
    ErrorsTableEntry(romIncompatibleAlert,
    _T("System incompatible"),
    _T("System Version 4.0 or greater is required to run iPedia.")),

    ErrorsTableEntry(networkUnavailableAlert,
    _T("Network unavailable"),
    _T("Unable to initialize network subsystem.")),

    ErrorsTableEntry(cantConnectToServerAlert,
    _T("Server unavailable"),
    _T("Can't connect to the server.")),

    ErrorsTableEntry(articleNotFoundAlert,
    _T("Article not found"),
    _T("Encyclopedia article for '^1' was not found.")),

    ErrorsTableEntry(articleTooLongAlert,
    _T("Article too long"),
    _T("Article is too long for iPedia to process.")),

    ErrorsTableEntry(notEnoughMemoryAlert,
    _T("Error"),
    _T("Not enough memory to complete current operation.")),

    ErrorsTableEntry(serverFailureAlert,
    _T("Server error"),
    _T("Unable to complete request due to server error.")),

    ErrorsTableEntry(invalidRegCodeAlert,
    _T("Invalid registration code"),
    _T("Invalid registration code sent. Please check registration code (using menu 'Options/Register'). Please contact support@arslexis.com if the problem persists.")),

    ErrorsTableEntry(invalidCookieAlert,
    _T("Invalid cookie"),
    _T("Invalid cookie sent. Please contact support@arslexis.com if the problem persists.")),

    ErrorsTableEntry(alertRegistrationFailed,
    _T("Wrong registration code"),
    _T("Incorrect registration code. Contact support@arslexis.com in case of problems.")),

    ErrorsTableEntry(alertRegistrationOk,
    _T("Registration successful"),
    _T("Thank you for registering iPedia.")),

    ErrorsTableEntry(lookupLimitReachedAlert,
    _T("Trial expired"),
    _T("Your unregistered version expired. Please register by purchasing registration code and entering it using menu 'Options/Register'.")),

    ErrorsTableEntry(forceUpgradeAlert,
    _T("Upgrade required"),
    _T("You need to upgrade iPedia to a newer version. Use menu 'Options/Check for updates' or press Update button to download newer version.")),

    ErrorsTableEntry(unsupportedDeviceAlert,
    _T("Unsupported device"),
    _T("Your hardware configuration is unsupported. Please contact support@arslexis.com if the problem persists.")),

    // this shouldn't really happen, means bug on the client
    ErrorsTableEntry(unexpectedRequestArgumentAlert,
    _T("Unexpected request argument"),
    _T("Unexpected request argument. Please contact support@arslexis.com if the problem persists.")),

    ErrorsTableEntry(requestArgumentMissingAlert,
    _T("Request argument missing"),
    _T("Request argument missing. Please contact support@arslexis.com if the problem persists.")),

    ErrorsTableEntry(invalidProtocolVersionAlert,
    _T("Invalid protocol version"),
    _T("Invalid protocol version. Please contact support@arslexis.com if the problem persists.")),

    ErrorsTableEntry(userDisabledAlert,
    _T("User disabled"),
    _T("This user has been disabled. Please contact support@arslexis.com if the problem persists.")),

    ErrorsTableEntry(malformedRequestAlert,
    _T("Malformed request"),
    _T("Server rejected your query. Please contact support@arslexis.com if the problem persists.")),

    ErrorsTableEntry(invalidRequestAlert,
    _T("Invalid request"),
    _T("Client sent invalid request. Please contact support@arslexis.com if the problem persists.")),

    ErrorsTableEntry(noWebBrowserAlert,
    _T("No web browser"),
    _T("Web browser is not installed on this device.")),

    ErrorsTableEntry(malformedResponseAlert,
    _T("Malformed response"),
    _T("Server returned malformed response. Please contact support@arslexis.com if the problem persists.")),

    ErrorsTableEntry(connectionTimedOutAlert,
    _T("Connection timed out"),
    _T("Connection timed out.")),

    ErrorsTableEntry(connectionErrorAlert,
    _T("Error"),
    _T("Connection terminated.")),

    ErrorsTableEntry(connectionServerNotRunning,
    _T("Error"),
    _T("The iPedia server is not available. Please contact support@arslexis.com if the problem persists."))
};

static RECT g_progressRect = { 0, 0, 0, 0 };
static RenderingProgressReporter* g_RenderingProgressReporter = NULL;
static RenderingProgressReporter* g_RegistrationProgressReporter = NULL;

TCHAR szAppName[] = TEXT("iPedia");
TCHAR szTitle[]   = TEXT("iPedia");

iPediaApplication iPediaApplication::instance_;

Definition *g_definition = new Definition();
Definition *g_about = new Definition();
Definition *g_register = new Definition();
Definition *g_tutorial = new Definition();
Definition *g_wikipedia = new Definition();
DisplayMode g_displayMode = showAbout;

GenericTextElement* g_articleCountElement = NULL;
long g_articleCountSet = -1;

bool g_forceLayoutRecalculation = false;
bool g_forceAboutRecalculation = false;

RenderingProgressReporter* rep;
RenderingPreferences* prefs= new RenderingPreferences();
LRESULT handleMenuCommand(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void CopyToClipboard();

static int  g_stressModeCnt = 0;

//bool g_isAboutVisible = false;

bool g_uiEnabled = true;

String articleCountText;
String databaseDateText;
String dateText;
String recentWord;
String searchWord;

HINSTANCE g_hInst            = NULL;
HWND      g_hwndMain         = NULL;
HWND      g_hwndEdit         = NULL;
HWND      g_hwndScroll       = NULL;
HWND      g_hWndMenuBar      = NULL;   // Current active menubar
HWND      g_hwndSearchButton = NULL;
HBITMAP   g_searchBtnUpBmp   = NULL;
HBITMAP   g_searchBtnDownBmp = NULL;
HBITMAP   g_searchBtnDiasabledBmp = NULL;

WNDPROC   g_oldEditWndProc   = NULL;

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void paint(HWND hwnd, HDC hdc, RECT rcpaint);
void handleExtendSelection(HWND hwnd, int x, int y, bool finish);

void repaintDefiniton(HWND hwnd, int scrollDelta = 0);
void moveHistoryForward();
void moveHistoryBack();
void setScrollBar(Definition* definition);
void scrollDefinition(int units, ScrollUnit unit, bool updateScrollbar);
void SimpleOrExtendedSearch(HWND hwnd, bool simple);
void DrawSearchButton(LPDRAWITEMSTRUCT lpdis);
static void DoRegister();

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
        MessageBox(
            g_hwndMain,
            errorMsg.c_str(),
            TEXT("Error"),
            MB_OK|MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND
            );
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
    g_scrollBarDx = GetSystemMetrics(SM_CXVSCROLL);
    g_menuDy = 0;
#ifdef WIN32_PLATFORM_PSPC
    g_menuDy = GetSystemMetrics(SM_CYMENU);
    g_searchBtnUpBmp = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SEARCH_BTN_UP)); 
    g_searchBtnDownBmp = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SEARCH_BTN_DOWN)); 
    g_searchBtnDiasabledBmp = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SEARCH_BTN_DISABLED));  

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

    mbi.hInstRes = g_hInst;

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
        g_hInst,
        NULL);
#ifdef WIN32_PLATFORM_PSPC
    g_hwndSearchButton = CreateWindow(
        _T("button"),  
        _T("Search"),
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_OWNERDRAW,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        CW_USEDEFAULT, CW_USEDEFAULT,
        hwnd, (HMENU)ID_SEARCH_BTN, g_hInst, NULL);
#endif

    g_oldEditWndProc=(WNDPROC)SetWindowLong(g_hwndEdit, GWL_WNDPROC, (LONG)EditWndProc);
    
    SendMessage(g_hwndEdit, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)_T(""));

    g_hwndScroll = CreateWindow(
        TEXT("scrollbar"),
        NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP| SBS_VERT,
        0,0, CW_USEDEFAULT, CW_USEDEFAULT, hwnd,
        (HMENU) ID_SCROLL,
        g_hInst,
        NULL);

    setScrollBar(g_about);
    (void)SendMessage(
        mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY)
        );

    iPediaApplication& app=iPediaApplication::instance();
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

#ifdef WIN32_PLATFORM_PSPC
        case WM_DRAWITEM:
            DrawSearchButton((LPDRAWITEMSTRUCT) lp);
            return TRUE; 
#endif
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
            int searchButtonDX = 20;
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
            Graphics gr(GetDC(g_hwndMain), g_hwndMain);
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
                MB_YESNO);
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
            for(int j=0; j<ErrorsTableEntries; j++)
            {
                if (ErrorsTable[j].errorCode == data.alertId)
                {
                    String msg = ErrorsTable[j].message;
                    if (customAlert)
                    {
                        int pos=msg.find(_T("^1"));
                        msg.replace(pos,2,app.popCustomAlert().c_str());
                    }
                    MessageBox(g_hwndMain, 
                        msg.c_str(),
                        ErrorsTable[j].title.c_str(),
                        MB_OK|MB_ICONEXCLAMATION);
                    break;
                }
            }
            setUIState(true);
            break;
        }

        case LookupManager::lookupStartedEvent:
        case LookupManager::lookupProgressEvent:
        {
            InvalidateRect(g_hwndMain, &g_progressRect, FALSE);
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
                        int len = SendMessage(g_hwndEdit, EM_LINELENGTH, 0,0);
                        SendMessage(g_hwndEdit, EM_SETSEL, 0,len);
                    }
                    //g_isAboutVisible=false;
                    setScrollBar(g_definition);
                    setDisplayMode(showArticle);
                    InvalidateRect(g_hwndMain, NULL, FALSE);
                    break;
                }

                case LookupFinishedEventData::outcomeList:
                    recentWord.assign(lookupManager->lastInputTerm());
                    SendMessage(hwnd, WM_COMMAND, IDM_MENU_RESULTS, 0);
                    setUIState(true);
                    break;

                case LookupFinishedEventData::outcomeRegCodeValid:
                {
                    setUIState(true);
                    g_fRegistration = false;
                    iPediaApplication::Preferences& prefs=iPediaApplication::instance().preferences();
                    assert(!g_newRegCode.empty());
                    // TODO: assert that it consists of numbers only
                    if (g_newRegCode!=prefs.regCode)
                    {
                        assert(g_newRegCode.length()<=prefs.regCodeLength);
                        prefs.regCode=g_newRegCode;
                        app.savePreferences();
                    }   
                    MessageBox(hwnd, 
                        _T("Thank you for registering iPedia."), 
                        _T("Registration successful"), 
                        MB_OK|MB_ICONINFORMATION);
                    break;
                }

                case LookupFinishedEventData::outcomeRegCodeInvalid:
                {
                    g_fRegistration = false;
                    setUIState(true);
                    iPediaApplication::Preferences& prefs=iPediaApplication::instance().preferences();
                    if (MessageBox(hwnd, 
                        _T("Incorrect registration code. Contact support@arslexis.com in case of problems. Do you want to re-enter the code ?"), 
                        _T("Wrong registration code"), 
                        MB_YESNO|MB_ICONERROR)==IDNO)
                    {
                        // this is "Ok" button. Clear-out registration code (since it was invalid)
                        prefs.regCode = _T("");
                        app.savePreferences();
                        g_newRegCode = _T("");
                    }
                    else
                    {
                        DoRegister();
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
            DeleteObject(g_searchBtnUpBmp);
            DeleteObject(g_searchBtnDownBmp);
            DeleteObject(g_searchBtnDiasabledBmp);
            PostQuitMessage(0);
        break;

        default:
            lResult = DefWindowProc(hwnd, msg, wp, lp);
            break;
    }
    return lResult;
}

static void DoRegister()
{
    if (!fInitConnection())
        return;

    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager(true);

    g_newRegCode = app.preferences().regCode;
    if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_REGISTER), g_hwndMain,RegistrationDlgProc))
    {
        if (lookupManager && !lookupManager->lookupInProgress())
        {
            lookupManager->verifyRegistrationCode(g_newRegCode);
            setUIState(false);
            g_fRegistration = true;
        }
    }
    else
        g_newRegCode = _T("");
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
            //g_isAboutVisible = true;
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
            if (!fInitConnection())
                break;
            Definition &def = currentDefinition();
            if (!def.empty())
            {
                if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_HYPERLINKS), hwnd,HyperlinksDlgProc))
                {
                /*if (lookupManager && !lookupManager->lookupInProgress())
                {
                    lookupManager->lookupTerm(searchWord);*/
                    //setUIState(false);
                    InvalidateRect(hwnd,NULL,FALSE);
                    //}
                }
            }
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
            DoRegister();
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
            int res=DialogBox(g_hInst, MAKEINTRESOURCE(IDD_LAST_RESULTS), hwnd,LastResultsDlgProc);
            if (res)
            {
                if (lookupManager && !lookupManager->lookupInProgress())
                {
                    if (1==res)
                        lookupManager->lookupIfDifferent(searchWord);
                    else
                        lookupManager->search(searchWord);
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
        

bool InitInstance (HINSTANCE hInstance, int CmdShow )
{

    g_hInst = hInstance;
    g_hwndMain = CreateWindow(szAppName,
            szTitle,
            WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL, NULL, hInstance, NULL );

    if ( !g_hwndMain )
        return false;

    ShowWindow(g_hwndMain, CmdShow );
    UpdateWindow(g_hwndMain);
    return true;
}

BOOL InitApplication ( HINSTANCE hInstance )
{
    WNDCLASS wc;
    BOOL f;

    wc.style = CS_HREDRAW | CS_VREDRAW ;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = NULL;
    wc.hInstance = hInstance;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH) GetStockObject( WHITE_BRUSH );
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szAppName;

    f = RegisterClass(&wc);

    return f;
}


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPWSTR     lpCmdLine,
                   int        CmdShow)

{
    // if we're already running, then just bring our window to front
    HWND hwndPrev = FindWindow(szAppName, szTitle);
    if (hwndPrev) 
    {
        SetForegroundWindow(hwndPrev);    
        return 0;
    }

    if (!hPrevInstance)
    {
        if (!InitApplication(hInstance))
            return FALSE;
    }

    if (!InitInstance(hInstance, CmdShow))
        return FALSE;
    
    int retVal = iPediaApplication::instance().waitForEvent();

    deinitConnection();
    return retVal;

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

    if (!onlyProgress)
    {
        Definition &def = currentDefinition();
        repaintDefiniton(hwnd);
        if (g_forceLayoutRecalculation) 
            setScrollBar(&def);
        if (g_forceLayoutRecalculation)
            PostMessage(g_hwndMain,WM_COMMAND, IDM_ENABLE_UI, 0);
        g_forceLayoutRecalculation = false;
    }

    if (lookupManager && lookupManager->lookupInProgress() && !g_fRegistration)
    {
        Graphics gr(GetDC(g_hwndMain), g_hwndMain);    
        ArsLexis::Rectangle progressArea(g_progressRect);
        lookupManager->showProgress(gr, progressArea);
    }

    if (lookupManager && lookupManager->lookupInProgress() && g_fRegistration)
    {
        g_RegistrationProgressReporter->reportProgress(-1);
        /*Graphics gr(GetDC(g_hwndMain), g_hwndMain);    
        ArsLexis::Rectangle progressArea(g_progressRect);
        
        DrawProgressRect(gr.handle(), progressArea);
        DrawProgressBar(gr, 0, progressArea);
        int h = progressArea.height();
        progressArea.explode(6, h - 20, -12, -h + 15);
        gr.setFont(WinFont());
        String str(_T("Registering application..."));

        gr.drawText(str.c_str(), str.length(), progressArea.topLeft);*/
    }
}
            
LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_KEYDOWN:
        {
            switch (wp) 
            {
                case VK_TACTION:
                    if (g_uiEnabled)
                        SendMessage(g_hwndMain,WM_COMMAND, ID_SEARCH, 0);
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
        I thought it would be more useful, but it's only 
        irritating me
        case WM_SETFOCUS:
        {            
            SHSipPreference(g_hwndMain, SIP_UP);
            break;
        }
        case WM_KILLFOCUS:
        {
            SHSipPreference(g_hwndMain, SIP_DOWN);
		    break;
        }
        */
    }
    return CallWindowProc(g_oldEditWndProc, hwnd, msg, wp, lp);
}

void setMenuBarButtonState(int buttonID, bool state)
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
    if (hwndMB) 
    {
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
        setMenuBarButtonState(ID_SEARCH_BTN2, g_uiEnabled);
        if (g_uiEnabled)
        {
            setMenuBarButtonState(ID_NEXT_BTN, lookupManager->hasNextHistoryTerm());
            setMenuBarButtonState(ID_PREV_BTN, 
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

// Try to launch IE with a given url
bool GotoURL(LPCTSTR lpszUrl)
{
    SHELLEXECUTEINFO sei;
    memset(&sei, 0, sizeof(SHELLEXECUTEINFO));
    sei.cbSize  = sizeof(SHELLEXECUTEINFO);
    sei.fMask   = SEE_MASK_FLAG_NO_UI;
    sei.lpVerb  = _T("open");
    sei.lpFile  = lpszUrl;
    sei.nShow   = SW_SHOWMAXIMIZED;

    return ShellExecuteEx(&sei);
}

void setUIState(bool enabled)
{
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager(true);

#ifdef WIN32_PLATFORM_PSPC
    setMenuBarButtonState(ID_MENU_BTN, enabled);
    setMenuBarButtonState(ID_OPTIONS_MENU_BTN, enabled);
    if (enabled)
    {
        setMenuBarButtonState(ID_NEXT_BTN, lookupManager->hasNextHistoryTerm());
        setMenuBarButtonState(ID_PREV_BTN, lookupManager->hasPreviousHistoryTerm());
    }
    else
    {
        setMenuBarButtonState(ID_NEXT_BTN, false);
        setMenuBarButtonState(ID_PREV_BTN, false);
    }
    setMenuBarButtonState(ID_SEARCH_BTN2, enabled);
    EnableWindow(g_hwndSearchButton, enabled);
#else
    setMenuBarButtonState(ID_MENU_BTN, enabled);
    setMenuBarButtonState(ID_SEARCH, enabled);
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
    Definition &def = currentDefinition();
    if (def.empty())
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
    repaintDefiniton(g_hwndMain, units);
}

void repaintDefiniton(HWND hwnd, int scrollDelta)
{
    Definition &def=currentDefinition();
    RECT rect;
    GetClientRect (g_hwndMain, &rect);
    rect.top    +=22;
    rect.left   +=2;
    rect.right  -=2+g_scrollBarDx;
    rect.bottom -=2+g_menuDy;

    RECT b;
    GetClientRect(g_hwndMain, &b);
    ArsLexis::Rectangle bounds=b;
    ArsLexis::Rectangle defRect=rect;

    Graphics gr(GetDC(g_hwndMain), g_hwndMain);
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
                    def.scroll(offscreen,*prefs, scrollDelta);
                else
                    def.render(offscreen, defRect, *prefs, g_forceLayoutRecalculation);
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
            def.scroll(gr,*prefs, scrollDelta);
        else
            def.render(gr, defRect, *prefs, g_forceLayoutRecalculation);
    
    setScrollBar(&def);
}

void CopyToClipboard()
{
    if (g_definition->empty())
        return;
    if (!OpenClipboard(g_hwndMain))
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
    if (displayMode()!=showArticle)//g_isAboutVisible) 
    {
        setDisplayMode(showArticle);
        //g_isAboutVisible = false;
        if (!g_definition->empty())
        {
            SendMessage(g_hwndEdit, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)lookupManager->lastInputTerm().c_str());
            int len = SendMessage(g_hwndEdit, EM_LINELENGTH, 0,0);
            SendMessage(g_hwndEdit, EM_SETSEL, 0,len);
        }
        setScrollBar(g_definition);
        InvalidateRect(g_hwndMain, NULL, false);
        return;
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
    SendMessage(g_hwndEdit, EM_SETSEL, 0,len);
    String term(buf); 
    delete buf;
    if (term.empty()) 
        return;
    if (lookupManager && !lookupManager->lookupInProgress())
    {
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
                if (displayMode()!=showArticle)//g_isAboutVisible)
                {
                    //g_isAboutVisible = false;
                    setDisplayMode(showArticle);
                    setScrollBar(g_definition);
                    setUIState(true);
                    InvalidateRect(hwnd,NULL,FALSE);                            
                }
            }
        }
    }
}

void DrawSearchButton(LPDRAWITEMSTRUCT lpdis)
{
    HDC hdcMem; 
    hdcMem = CreateCompatibleDC(lpdis->hDC); 
    if (lpdis->itemState & ODS_DISABLED)
        SelectObject(hdcMem, g_searchBtnDiasabledBmp);
    else
    {
        if (lpdis->itemState & ODS_SELECTED)  // if selected 
            SelectObject(hdcMem, g_searchBtnDownBmp); 
        else 
            SelectObject(hdcMem, g_searchBtnUpBmp); 
    };
    
    // Destination 
    StretchBlt( 
        lpdis->hDC,         // destination DC 
        lpdis->rcItem.left, // x upper left 
        lpdis->rcItem.top,  // y upper left 
        
        // The next two lines specify the width and 
        // height. 
        lpdis->rcItem.right - lpdis->rcItem.left, 
        lpdis->rcItem.bottom - lpdis->rcItem.top, 
        hdcMem,    // source device context 
        0, 0,      // x and y upper left 
        20,        // source bitmap width 
        20,        // source bitmap height 
        SRCCOPY);  // raster operation 
    
    DeleteDC(hdcMem); 
}
