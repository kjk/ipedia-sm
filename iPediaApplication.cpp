#include <iPediaApplication.hpp>
#include <SysUtils.hpp>
#include <DeviceInfo.hpp>
//#include "MainForm.hpp"
//#include "RegistrationForm.hpp"
//#include "SearchResultsForm.hpp"
#include "LookupManager.hpp"
#include "LookupHistory.hpp"
//#include <PrefsStore.hpp>
#include <SocketConnection.hpp>
//IMPLEMENT_APPLICATION_INSTANCE(appFileCreator)

using namespace ArsLexis;

iPediaApplication::iPediaApplication():
    log_( _T("root") ),
    history_(0),
    diaNotifyRegistered_(false),
    //ticksPerSecond_(SysTicksPerSecond()),
    ticksPerSecond_(1000),
    lookupManager_(0),
    server_(serverToUse),
    stressMode_(false),
    hasHighDensityFeatures_(false)
{
#ifdef INTERNAL_BUILD
    log_.addSink(new MemoLogSink(), log_.logError);
#ifndef NDEBUG    
    log_.addSink(new HostFileLogSink(_T("\\var\\log\\iPedia.log")), log_.logEverything);
    log_.addSink(new DebuggerLogSink(), log_.logWarning);
#endif    
#endif
}

inline void iPediaApplication::detectViewer()
{
    //ushort_t  cardNo;
    //LocalID dbID;

    /*if (fDetectViewer(&cardNo,&dbID))
    {
        assert(dbID!=0);
        hyperlinkHandler_.setViewerLocation(cardNo, dbID);
    }*/
}

status_t iPediaApplication::initialize()
{
    status_t error = errNone;
    //err = initialize();
    if (!error)
    {
        /*if (diaSupport_ && notifyManagerPresent()) 
        {
            error=registerNotify(diaSupport_.notifyType());
            if (!error)
                diaNotifyRegistered_=true;
        }*/
    }
    
    //detectViewer();
       
    return error;
    //return errNone;
}

iPediaApplication::~iPediaApplication()
{
/*    if (diaNotifyRegistered_) 
        unregisterNotify(diaSupport_.notifyType());*/
    
    if (lookupManager_)
        delete lookupManager_;

    if (history_)
        delete history_;

    logAllocation_=false;
}


status_t iPediaApplication::normalLaunch()
{
    history_=new LookupHistory();
    loadPreferences();
    //gotoForm(mainForm);
    //runEventLoop();
    savePreferences();
    return errNone;
}

/*status_t iPediaApplication::handleSystemNotify(SysNotifyParamType& notify)
{
    const ArsLexis::DIA_Support& dia=getDIASupport();
    if (dia && dia.notifyType()==notify.notifyType)
        dia.handleNotify();
    return errNone;
}*/

LookupManager* iPediaApplication::getLookupManager(bool create)
{
    if (!lookupManager_ && create)
    {
        //assert(0!=history_);
        lookupManager_=new LookupManager(*history_);
    }
    return lookupManager_;
}

DWORD iPediaApplication::waitForEvent()
{
    MSG msg;
    /*
    ArsLexis::SocketConnectionManager* manager=0;
    if (lookupManager_)
        manager=&lookupManager_->connectionManager();
    if (manager && manager->active())
    {
        //setEventTimeout(0);
        iPediaApplication::waitForEvent(event);
        if (//nilEvent==event.eType)
            0==event.eType)
            manager->manageConnectionEvents(ticksPerSecond_/20);
    }
    else
    {
        //setEventTimeout(evtWaitForever);
        iPediaApplication::waitForEvent(event);
    }
    */
    while(true)
    {
        ArsLexis::SocketConnectionManager* manager=0;
        if (lookupManager_)
            manager=&lookupManager_->connectionManager();
        if (manager && manager->active())
        {
            if(!PeekMessage(&msg, NULL, 0, 0, FALSE))
                manager->manageConnectionEvents(ticksPerSecond_/20);
        }
        if(!PeekMessage(&msg, NULL, 0, 0, TRUE))
            Sleep(ticksPerSecond_/20);
        else
        {
            if (lookupManager_ && 
                (LookupManager::lookupStartedEvent<=msg.message)
                && (LookupManager::lookupFinishedEvent>=msg.message))
            {
                EventType event;
                event.eType = msg.message;
                lookupManager_->handleLookupEvent(event);
            }
    
            if(msg.message!=WM_QUIT)
            {
                TranslateMessage (&msg);
		        DispatchMessage(&msg);
            }
            else
                return (msg.wParam);

        }
    };
}

/*Form* iPediaApplication::createForm(ushort_t formId)
{
    Form* form=0;
    switch (formId)
    {
        case mainForm:
            form=new MainForm(*this);
            break;
            
        case registrationForm:
            form=new RegistrationForm(*this);
            break;
            
        case searchResultsForm:
            form=new SearchResultsForm(*this);
            break;
        
        default:
            assert(false);
    }
    return form;            
}*/

/*bool iPediaApplication::handleApplicationEvent(EventType& event)
{
    bool handled=false;
    if (appDisplayAlertEvent==event.eType)
    {
        DisplayAlertEventData& data=reinterpret_cast<DisplayAlertEventData&>(event.data);
        if (!inStressMode())
            FrmAlert(data.alertId);
        else
            log().debug()<<_T("Alert: ")<<data.alertId;
    }
    else if (appDisplayCustomAlertEvent==event.eType)
    {
        assert(!customAlerts_.empty());            
        DisplayAlertEventData& data=reinterpret_cast<DisplayAlertEventData&>(event.data);
        if (!inStressMode())
            FrmCustomAlert(data.alertId, customAlerts_.front().c_str(), _T(""), _T(""));
        else
            log().debug()<<_T("Custom alert: ")<<data.alertId<<char_t('[')<<customAlerts_.front()<<char_t(']');
        customAlerts_.pop_front();
    }
    
    if (lookupManager_ && appLookupEventFirst<=event.eType && appLookupEventLast>=event.eType)
        lookupManager_->handleLookupEvent(event);
    else
        handled=Application::handleApplicationEvent(event);
    return handled;
}

namespace {

    enum PreferenceId 
    {
        cookiePrefId,
        serialNumberPrefId,
        serialNumberRegFlagPrefId,
        lastArticleCountPrefId,
        lookupHistoryFirstPrefId,
        renderingPrefsFirstPrefId=lookupHistoryFirstPrefId+LookupHistory::reservedPrefIdCount,
        
        next=renderingPrefsFirstPrefId+RenderingPreferences::reservedPrefIdCount
    };

    // These globals will be removed by dead code elimination.
    ArsLexis::StaticAssert<(sizeof(uint_t) == sizeof(ushort_t))> uint_t_the_same_size_as_UInt16;
    ArsLexis::StaticAssert<(sizeof(bool) == sizeof(Boolean))> bool_the_same_size_as_Boolean;
    
}*/

void iPediaApplication::loadPreferences()
{
   /* 
    Preferences prefs;
    // PrefsStoreXXXX seem to be rather heavyweight objects (writer is >480kB), so it might be a good idea not to allocate them on stack.
    std::auto_ptr<PrefsStoreReader> reader(new PrefsStoreReader(appPrefDatabase, appFileCreator, sysFileTPreferences));

    status_t         error;
    const char* text;

    if (errNone!=(error=reader->ErrGetStr(cookiePrefId, &text))) 
        goto OnError;
    prefs.cookie=text;
    if (errNone!=(error=reader->ErrGetStr(serialNumberPrefId, &text))) 
        goto OnError;
    prefs.serialNumber=text;
    if (errNone!=(error=reader->ErrGetBool(serialNumberRegFlagPrefId, safe_reinterpret_cast<Boolean*>(&prefs.serialNumberRegistered))))
        goto OnError;
    if (errNone!=(error=reader->ErrGetLong(lastArticleCountPrefId, &prefs.articleCount))) 
        goto OnError;
    if (errNone!=(error=prefs.renderingPreferences.serializeIn(*reader, renderingPrefsFirstPrefId)))
        goto OnError;
    preferences_=prefs;    
    assert(0!=history_);
    if (errNone!=(error=history_->serializeIn(*reader, lookupHistoryFirstPrefId)))
        goto OnError;
    return;
           
OnError:*/
    return;        
}

void iPediaApplication::savePreferences()
{
    /*
    status_t   error;
    std::auto_ptr<PrefsStoreWriter> writer(new PrefsStoreWriter(appPrefDatabase, appFileCreator, sysFileTPreferences));

    if (errNone!=(error=writer->ErrSetStr(cookiePrefId, preferences_.cookie.c_str())))
        goto OnError;
    if (errNone!=(error=writer->ErrSetStr(serialNumberPrefId, preferences_.serialNumber.c_str())))
        goto OnError;
    if (errNone!=(error=writer->ErrSetBool(serialNumberRegFlagPrefId, preferences_.serialNumberRegistered)))
        goto OnError;
    if (errNone!=(error=writer->ErrSetLong(lastArticleCountPrefId, preferences_.articleCount))) 
        goto OnError;
    if (errNone!=(error=preferences_.renderingPreferences.serializeOut(*writer, renderingPrefsFirstPrefId)))
        goto OnError;
    assert(0!=history_);
    if (errNone!=(error=history_->serializeOut(*writer, lookupHistoryFirstPrefId)))
        goto OnError;
    if (errNone!=(error=writer->ErrSavePreferences()))
        goto OnError;
    return;        
OnError:*/
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
