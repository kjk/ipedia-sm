#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include <LookupHistory.hpp>
#include <PrefsStore.hpp>
#include <SocketConnection.hpp>
#include <SysUtils.hpp>
#include <DeviceInfo.hpp>
#include <ipedia.h>
#include <sm_ipedia.h>

using namespace ArsLexis;

iPediaApplication::iPediaApplication():
    log_( _T("root") ),
    history_(new LookupHistory()),
    diaNotifyRegistered_(false),
    ticksPerSecond_(1000),
    lookupManager_(0),
    server_(serverToUse),
    stressMode_(false),
    hasHighDensityFeatures_(false),
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

    logAllocation_=false;
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

DWORD iPediaApplication::waitForEvent()
{
    loadPreferences();
    setupAboutWindow();    
    setMenu(hwndMain_);
    InvalidateRect(hwndMain_,NULL,FALSE);

    MSG msg;
    while (true)
    {
        ArsLexis::SocketConnectionManager* manager=0;
        if (lookupManager_)
            manager=&lookupManager_->connectionManager();
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

    // These globals will be removed by dead code elimination.
    //ArsLexis::StaticAssert<(sizeof(uint_t) == sizeof(ushort_t))> uint_t_the_same_size_as_UInt16;
    //ArsLexis::StaticAssert<(sizeof(bool) == sizeof(Boolean))> bool_the_same_size_as_Boolean;
    
}

void iPediaApplication::loadPreferences()
{
    std::auto_ptr<PrefsStoreReader> reader(new PrefsStoreReader(appPrefDatabase, appFileCreator, 0));

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
    std::auto_ptr<PrefsStoreWriter> writer(new PrefsStoreWriter(appPrefDatabase, appFileCreator, 0));

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
