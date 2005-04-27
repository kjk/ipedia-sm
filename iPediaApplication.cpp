#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include <LookupHistory.hpp>
#include <WinPrefsStore.hpp>
#include <SocketConnection.hpp>
#include <SysUtils.hpp>
#include <DeviceInfo.hpp>
#include <ipedia.h>
#include <sm_ipedia.h>

HWND g_hwndForEvents = NULL;

using namespace ArsLexis;

#define serverMarek          _T("arslex.no-ip.info:9000")
#define serverKjk            _T("dict-pc.arslexis.com:9000")
#define serverKjkLaptop      _T("192.168.123.150:9000")
#define serverKjkLaptop2     _T("169.254.191.23:9000")
#define serverLocalhost      _T("192.168.0.1:9000")
#define serverLocalhost2     _T("127.0.0.1:9000")

#define serverKjkLaptop3     _T("192.168.57.162:9000")

#define serverIpediaArslexis _T("ipedia.arslexis.com:9000")

#define serverAndrzejLaptop _T("gizmo:9000")
#define serverAndrzejDVD		_T("rabban:9000")

#define SERVER_TO_USE serverIpediaArslexis
//#define SERVER_TO_USE serverKjkLaptop3

const ErrorInfo ErrorsTable[] =
{
    ErrorInfo(romIncompatibleAlert,
    _T("System incompatible"),
    _T("System Version 4.0 or greater is required to run iPedia.")),

    ErrorInfo(networkUnavailableAlert,
    _T("Network unavailable"),
    _T("Unable to initialize network subsystem.")),

    ErrorInfo(cantConnectToServerAlert,
    _T("Server unavailable"),
    _T("Can't connect to the server.")),

    ErrorInfo(articleNotFoundAlert,
    _T("Article not found"),
    _T("Encyclopedia article for '^1' was not found.")),

    ErrorInfo(articleTooLongAlert,
    _T("Article too long"),
    _T("Article is too long for iPedia to process.")),

    ErrorInfo(notEnoughMemoryAlert,
    _T("Error"),
    _T("Not enough memory to complete current operation.")),

    ErrorInfo(serverFailureAlert,
    _T("Server error"),
    _T("Unable to complete request due to server error.")),

    ErrorInfo(invalidRegCodeAlert,
    _T("Invalid registration code"),
    _T("Invalid registration code sent. Please check registration code (using menu '") REGISTER_MENU_ITEM _T("'). Please contact support@arslexis.com if the problem persists.")),

    ErrorInfo(invalidCookieAlert,
    _T("Invalid cookie"),
    _T("Invalid cookie sent. Please contact support@arslexis.com if the problem persists.")),

    ErrorInfo(alertRegistrationFailed,
    _T("Wrong registration code"),
    _T("Wrong registration code. Please contact support@arslexis.com if problem persists.\n\nRe-enter the code?.")),

    ErrorInfo(alertRegistrationOk,
    _T("Registration successful"),
    _T("Thank you for registering iPedia.")),

    ErrorInfo(lookupLimitReachedAlert,
    _T("Trial expired"),
    _T("Unregistered version expired. Please register by purchasing registration code.\nEnter registration code?")),

    ErrorInfo(forceUpgradeAlert,
    _T("Upgrade required"),
    _T("You need to upgrade iPedia to a newer version. Use menu '") UPDATES_MENU_ITEM  _T("' or press Update button to download newer version.")),

    ErrorInfo(unsupportedDeviceAlert,
    _T("Unsupported device"),
    _T("Your hardware configuration is unsupported. Please contact support@arslexis.com if the problem persists.")),

    // this shouldn't really happen, means bug on the client
    ErrorInfo(unexpectedRequestArgumentAlert,
    _T("Unexpected request argument"),
    _T("Unexpected request argument. Please contact support@arslexis.com if the problem persists.")),

    ErrorInfo(requestArgumentMissingAlert,
    _T("Request argument missing"),
    _T("Request argument missing. Please contact support@arslexis.com if the problem persists.")),

    ErrorInfo(invalidProtocolVersionAlert,
    _T("Invalid protocol version"),
    _T("Invalid protocol version. Please contact support@arslexis.com if the problem persists.")),

    ErrorInfo(userDisabledAlert,
    _T("User disabled"),
    _T("This user has been disabled. Please contact support@arslexis.com if the problem persists.")),

    ErrorInfo(malformedRequestAlert,
    _T("Malformed request"),
    _T("Server rejected your query. Please contact support@arslexis.com if the problem persists.")),

    ErrorInfo(invalidRequestAlert,
    _T("Invalid request"),
    _T("Client sent invalid request. Please contact support@arslexis.com if the problem persists.")),

    ErrorInfo(noWebBrowserAlert,
    _T("No web browser"),
    _T("Web browser is not installed on this device.")),

    ErrorInfo(malformedResponseAlert,
    _T("Malformed response"),
    _T("Server returned malformed response. Please contact support@arslexis.com if the problem persists.")),

    ErrorInfo(connectionTimedOutAlert,
    _T("Connection timed out"),
    _T("Connection timed out.")),

    ErrorInfo(connectionErrorAlert,
    _T("Error"),
    _T("Connection terminated.")),

    ErrorInfo(connectionServerNotRunning,
    _T("Error"),
    _T("The iPedia server is not available. Please contact support@arslexis.com if the problem persists."))
};

#define ERROR_INFO_SIZE sizeof(ErrorsTable)/sizeof(ErrorsTable[0])

static const ErrorInfo* getErrorInfo(int alertId)
{
    for (int j=0; j<ERROR_INFO_SIZE; j++)
    {
        if (ErrorsTable[j].errorCode == alertId)
            return &(ErrorsTable[j]);
    }
    assert(0);
    return &(ErrorsTable[0]);
}

const char_t *getErrorTitle(int alertId)
{
    const ErrorInfo *ei = getErrorInfo(alertId);
    return ei->title;
}

const char_t *getErrorMessage(int alertId)
{
    const ErrorInfo *ei = getErrorInfo(alertId);
    return ei->message;
}

iPediaApplication::iPediaApplication():
    history_(new LookupHistory()),
    ticksPerSecond_(1000),
    lookupManager(NULL),
    serverAddress(SERVER_TO_USE),
    fArticleCountChecked(false)
{
	preferences_.currentLang = _T("en");
/*
#ifdef INTERNAL_BUILD
    log_.addSink(new MemoLogSink(), log_.logError);
#ifndef NDEBUG    
    log_.addSink(new HostFileLogSink(_T("\\var\\log\\iPedia.log")), log_.logEverything);
    log_.addSink(new DebuggerLogSink(), log_.logWarning);
#endif    
#endif
 */
#ifndef _PALM_OS
    // wince only
    GetSystemTime(&lastArticleCountCheckTime);
#endif
}

iPediaApplication::~iPediaApplication()
{   
    delete lookupManager;

    delete history_;

    logAllocation_ = false;
}

bool iPediaApplication::fLookupInProgress() const
{
    if (NULL==lookupManager)
        return false;
    return lookupManager->lookupInProgress();
}

DWORD iPediaApplication::runEventLoop()
{
    g_hwndForEvents = getMainWindow();

    MSG msg;
    while (true)
    {
        SocketConnectionManager* manager=0;

        if (lookupManager)
            manager = &lookupManager->connectionManager();

        if (manager && manager->active())
        {
            if (!PeekMessage(&msg, NULL, 0, 0, FALSE))
                manager->manageConnectionEvents(ticksPerSecond_/20);
        }

        if (!PeekMessage(&msg, NULL, 0, 0, TRUE))
            Sleep(ticksPerSecond_/20);
        else
        {
            if (lookupManager && 
                (LookupManager::lookupStartedEvent<=msg.message)
                && (LookupManager::lookupFinishedEvent>=msg.message) &&
                g_hwndForEvents==msg.hwnd)
            {
                EventType event;
                event.eType = msg.message;
                LookupFinishedEventData data;
                EventData i;
                i.wParam=msg.wParam; i.lParam=msg.lParam;
                memcpy(event.data, &i,sizeof(data));
                lookupManager->handleLookupEvent(event);
            }
    
            if (msg.message!=WM_QUIT)
            {
                TranslateMessage (&msg);
                DispatchMessage(&msg);
            }
            else
            {
                savePreferences();
                return (msg.wParam);
            }

        }
    };
}

namespace {

    enum PreferenceId 
    {
        cookiePrefId,
        regCodePrefId,
        serialNumberRegFlagPrefId, // unused
        lastArticleCountPrefId,
        databaseTimePrefId,
        lookupHistoryFirstPrefId,
        next=lookupHistoryFirstPrefId + LookupHistory::reservedPrefIdCount,
    };
}

void iPediaApplication::loadPreferences()
{
    std::auto_ptr<PrefsStoreReader> reader(new PrefsStoreReader(appPrefDatabase));

    Preferences     prefs;
    status_t        error;
    const char_t*   text;

    if (errNone!=(error=reader->ErrGetStr(cookiePrefId, &text))) 
        goto OnError;
    prefs.cookie = text;

    if (errNone!=(error=reader->ErrGetStr(regCodePrefId, &text))) 
        goto OnError;
    prefs.regCode = text;

    if (errNone!=(error=reader->ErrGetLong(lastArticleCountPrefId, &prefs.articleCount))) 
        goto OnError;

    if (errNone!=(error=reader->ErrGetStr(databaseTimePrefId, &text))) 
        goto OnError;
    prefs.databaseTime = text;

    preferences_ = prefs;    

    assert(0!=history_);
    if (errNone!=(error=history_->serializeIn(*reader, lookupHistoryFirstPrefId)))
        goto OnError;
    return;
            
OnError:
    return;         
}

void iPediaApplication::savePreferences()
{
    status_t error;
    std::auto_ptr<PrefsStoreWriter> writer(new PrefsStoreWriter(appPrefDatabase));

    if (errNone!=(error=writer->ErrSetStr(cookiePrefId, preferences_.cookie.c_str())))
        goto OnError;
    if (errNone!=(error=writer->ErrSetStr(regCodePrefId, preferences_.regCode.c_str())))
        goto OnError;
    if (errNone!=(error=writer->ErrSetLong(lastArticleCountPrefId, preferences_.articleCount))) 
        goto OnError;
    if (errNone!=(error=writer->ErrSetStr(databaseTimePrefId, preferences_.databaseTime.c_str())))
        goto OnError;
    assert(0!=history_);
    if (errNone!=(error=history_->serializeOut(*writer, lookupHistoryFirstPrefId)))
        goto OnError;
    if (errNone!=(error=writer->ErrSavePreferences()))
        goto OnError;
    return;        
OnError:
    //! @todo Diplay alert that saving failed?
    return; 
}

String iPediaApplication::popCustomAlert()
{
    String tmp = customAlerts_.front();
    customAlerts_.pop_front();
    return tmp;
}

void iPediaApplication::sendDisplayCustomAlertEvent(ushort_t alertId, const String& text1)
{
    customAlerts_.push_back(text1);
    sendEvent(appDisplayCustomAlertEvent, DisplayAlertEventData(alertId));
}

void iPediaApplication::getErrorMessage(int alertId, bool customAlert, String &out)
{
    out.assign(::getErrorMessage(alertId));
    if (customAlert)
    {
        int pos = out.find(_T("^1"));
        out.replace(pos,2,popCustomAlert().c_str());
    }
}

bool iPediaApplication::InitInstance(HINSTANCE hInstance, int CmdShow )
{
	assert(NULL == lookupManager);
	lookupManager = new LookupManager(*history_);
    
    hInst_ = hInstance;
    hwndMain_ = CreateWindow(APP_NAME,
        APP_WIN_TITLE,
        WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL );
    
    if ( !hwndMain_ )
        return false;
    
    ShowWindow(hwndMain_, CmdShow );
    UpdateWindow(hwndMain_);
    return true;
}

BOOL iPediaApplication::InitApplication ( HINSTANCE hInstance )
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
    wc.lpszClassName = APP_NAME;
    
    f = RegisterClass(&wc);
    
    return f;
}

bool iPediaApplication::initApplication(HINSTANCE hInstance, HINSTANCE hPrevInstance, const String& cmdLine, int cmdShow)
{
    if (!hPrevInstance)
    {
        if (!InitApplication(hInstance))
            return false;
    }
    
    if (!InitInstance(hInstance, cmdShow))
        return false;
    //Initialization of appilcation successful
    return true;
}

iPediaApplication& GetApp()
{
    return iPediaApplication::instance();
}

iPediaApplication::Preferences& GetPrefs()
{
    return iPediaApplication::instance().preferences();
}
