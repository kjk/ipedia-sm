#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include <LookupHistory.hpp>
#include <WinPrefsStore.hpp>
#include <SocketConnection.hpp>
#include <SysUtils.hpp>
#include <DeviceInfo.hpp>
#include <ipedia.h>
#include <sm_ipedia.h>

using namespace ArsLexis;

const char_t iPediaApplication::szAppName[] = _T("iPedia");
const char_t iPediaApplication::szTitle[]   = _T("iPedia");

const iPediaApplication::ErrorInfo iPediaApplication::ErrorsTable[] =
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
    _T("Your unregistered version expired. Please register by purchasing registration code and entering it using menu '") REGISTER_MENU_ITEM _T("'. Do you want to enter it now ?")),

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


iPediaApplication::iPediaApplication():
    log_( _T("root") ),
    history_(new LookupHistory()),
    ticksPerSecond_(1000),
    lookupManager_(0),
    server_(serverToUse),
    fArticleCountChecked(false)
{
#ifdef INTERNAL_BUILD
    log_.addSink(new MemoLogSink(), log_.logError);
#ifndef NDEBUG    
    log_.addSink(new HostFileLogSink(_T("\\var\\log\\iPedia.log")), log_.logEverything);
    log_.addSink(new DebuggerLogSink(), log_.logWarning);
#endif    
#endif
}

iPediaApplication::~iPediaApplication()
{   
    if (lookupManager_)
        delete lookupManager_;

    if (history_)
        delete history_;

    logAllocation_ = false;
}

bool iPediaApplication::fLookupInProgress() const
{
    if (NULL==lookupManager_)
        return false;
    return lookupManager_->lookupInProgress();
}

LookupManager* iPediaApplication::getLookupManager(bool create)
{
    if (!lookupManager_ && create)
    {
        assert(0!=history_);
        lookupManager_=new LookupManager(*history_);
    }
    return lookupManager_;
}

DWORD iPediaApplication::runEventLoop()
{
    loadPreferences();
    SetupAboutWindow();
    setMenu(hwndMain_);
    InvalidateRect(hwndMain_,NULL,FALSE);

    MSG msg;
    while (true)
    {
        ArsLexis::SocketConnectionManager* manager=0;

        if (lookupManager_)
            manager = &lookupManager_->connectionManager();

        if (manager && manager->active())
        {
            if (!PeekMessage(&msg, NULL, 0, 0, FALSE))
                manager->manageConnectionEvents(ticksPerSecond_/20);
        }

        if (!PeekMessage(&msg, NULL, 0, 0, TRUE))
            Sleep(ticksPerSecond_/20);
        else
        {
            if (lookupManager_ && 
                (LookupManager::lookupStartedEvent<=msg.message)
                && (LookupManager::lookupFinishedEvent>=msg.message)&&
                this->hwndMain_==msg.hwnd)
            {
                EventType event;
                event.eType = msg.message;
                LookupFinishedEventData data;
                ArsLexis::EventData i;
                i.wParam=msg.wParam; i.lParam=msg.lParam;
                //memcpy(&data, &i, sizeof(data));
                memcpy(event.data, &i,sizeof(data));
                lookupManager_->handleLookupEvent(event);
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
        renderingPrefsFirstPrefId=lookupHistoryFirstPrefId+LookupHistory::reservedPrefIdCount,
        
        next=renderingPrefsFirstPrefId+RenderingPreferences::reservedPrefIdCount
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

    if (errNone!=(error=prefs.renderingPreferences.serializeIn(*reader, renderingPrefsFirstPrefId)))
        goto OnError;

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
    if (errNone!=(error=preferences_.renderingPreferences.serializeOut(*writer, renderingPrefsFirstPrefId)))
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

ArsLexis::String iPediaApplication::popCustomAlert()
{
    ArsLexis::String tmp = customAlerts_.front();
    customAlerts_.pop_front();
    return tmp;
}

void iPediaApplication::sendDisplayCustomAlertEvent(ushort_t alertId, const ArsLexis::String& text1)
{
    customAlerts_.push_back(text1);
    sendEvent(appDisplayCustomAlertEvent, DisplayAlertEventData(alertId));
}

void* ArsLexis::allocate(size_t size)
{
    void* ptr=0;
    if (size) 
        ptr=malloc(size);
    else
        ptr=malloc(1);
    if (!ptr)
        handleBadAlloc();
    return ptr;
}

const iPediaApplication::ErrorInfo& iPediaApplication::getErrorInfo(int alertId)
{
    for(int j=0; j<sizeof(ErrorsTable)/sizeof(ErrorsTable[0]); j++)
    {
        if (ErrorsTable[j].errorCode == alertId)
            return ErrorsTable[j];
    }
    assert(0);
    return ErrorsTable[0];
}

void iPediaApplication::getErrorMessage(int alertId, bool customAlert, String &out)
{
    out.assign(getErrorInfo(alertId).message);
    if (customAlert)
    {
        int pos = out.find(_T("^1"));
        out.replace(pos,2,popCustomAlert().c_str());
    }
}

void iPediaApplication::getErrorTitle(int alertId, String &out)
{
    out.assign(getErrorInfo(alertId).title);
}

bool iPediaApplication::InitInstance(HINSTANCE hInstance, int CmdShow )
{
    
    hInst_ = hInstance;
    hwndMain_ = CreateWindow(szAppName,
        szTitle,
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
    wc.lpszClassName = szAppName;
    
    f = RegisterClass(&wc);
    
    return f;
}

bool iPediaApplication::initApplication(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     ArsLexis::String cmdLine,
                     int cmdShow)
{
    HWND hwndPrev = FindWindow(szAppName, szTitle);
    if (hwndPrev) 
    {
        SetForegroundWindow(hwndPrev);    
        return false;
    }
    
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
