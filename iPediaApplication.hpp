#ifndef __WIN_IPEDIA_APPLICATION_HPP__
#define __WIN_IPEDIA_APPLICATION_HPP__

#include <Windows.h>

#include "iPediaHyperlinkHandler.hpp"
#include <Logging.hpp>
#include <RenderingPreferences.hpp>
#include <SysUtils.hpp>
#include <BaseTypes.hpp>

using ArsLexis::char_t;

class LookupManager;
class LookupHistory;
struct LookupFinishedEventData;

struct DisplayAlertEventData
{
    ushort_t alertId;

    DisplayAlertEventData(ushort_t aid):
        alertId(aid) {}

    DisplayAlertEventData():
        alertId(0) {}
};

struct ErrorInfo
{
    int       errorCode;
    const char_t *  title;
    const char_t *  message;

    ErrorInfo(int code, const char_t *tit, const char_t *msg) : errorCode(code), title(tit), message(msg)
    {}
};

const char_t *getErrorTitle(int alertId);
const char_t *getErrorMessage(int alertId);

class iPediaApplication
{
    mutable ArsLexis::RootLogger log_;
    ushort_t                ticksPerSecond_;
    iPediaHyperlinkHandler  hyperlinkHandler_;
    LookupHistory*          history_;
    LookupManager*          lookupManager_;
    ArsLexis::String        server_;
    HINSTANCE               hInst_;
    
    typedef std::list<ArsLexis::String> CustomAlerts_t;
    CustomAlerts_t customAlerts_;

    HWND hwndMain_;    

protected:

    bool handleApplicationEvent(ArsLexis::EventType& event);

public:
    bool fArticleCountChecked;

#ifndef _PALM_OS
    // wince only
    SYSTEMTIME lastArticleCountCheckTime;
#endif

    void savePreferences();
    void loadPreferences();

    iPediaApplication();

    DWORD runEventLoop();
    
    HWND getMainWindow()
    {return hwndMain_ ;}
    
    void setMainWindow(HWND hwndMain)
    {hwndMain_=hwndMain;}

    ~iPediaApplication();
    
    LookupManager* getLookupManager(bool create=false);
    const LookupManager* getLookupManager() const
    {return lookupManager_;}
    bool fLookupInProgress() const;

    ushort_t ticksPerSecond() const
    {return ticksPerSecond_;}
    
    struct Preferences
    {
        RenderingPreferences renderingPreferences;
        
        enum {cookieLength=32};
        ArsLexis::String cookie;
                        
        enum {regCodeLength=32};
        ArsLexis::String regCode;
        
        long articleCount;

        ArsLexis::String currentLang;

        ArsLexis::String availableLangs;

        Preferences():
            articleCount(-1)
        {}
            ArsLexis::String databaseTime;
    };
    
    Preferences& preferences() 
    {return preferences_;}
    
    const Preferences& preferences() const
    {return preferences_;}
    
    const RenderingPreferences& renderingPreferences() const
    {return preferences().renderingPreferences;}
        
    enum {
        reservedLookupEventsCount=3
    };
    
    enum Event
    {
        appDisplayAlertEvent=WM_APP,
        appDisplayCustomAlertEvent,
        appLookupEventFirst,
        appLookupEventLast=appLookupEventFirst+reservedLookupEventsCount,
        appForceUpgrade,
        appLangNotAvailable,
        appFirstAvailableEvent
    };
    
    static void sendDisplayAlertEvent(ushort_t alertId)
    {ArsLexis::sendEvent(appDisplayAlertEvent, DisplayAlertEventData(alertId));}
    
    ArsLexis::String popCustomAlert();
    
    void sendDisplayCustomAlertEvent(ushort_t alertId, const ArsLexis::String& text1);
    
    ArsLexis::Logger& log() const
    {return log_;}
    
    void setServer(const ArsLexis::String& server)
    {server_=server;}
    
    const ArsLexis::String& server() const
    {return server_;}
    
    static iPediaApplication& instance()
    {return instance_;}
            
    const LookupHistory& history() const
    {
        assert(0!=history_);
        return *history_;
    }
    
    iPediaHyperlinkHandler& hyperlinkHandler()
    {return hyperlinkHandler_;}

    void getErrorMessage(int alertId, bool customAlert, ArsLexis::String &out);

    bool InitInstance (HINSTANCE hInstance, int CmdShow );
    BOOL InitApplication ( HINSTANCE hInstance );    
    bool initApplication(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                         ArsLexis::String cmdLine, int cmdShow);
    HINSTANCE getApplicationHandle()
    {return hInst_;}
private:
    
    Preferences preferences_;
    static iPediaApplication instance_;
    bool logAllocation_;  

};

iPediaApplication& GetApp();

iPediaApplication::Preferences& GetPrefs();
#endif
