#ifndef __WIN_IPEDIA_APPLICATION_HPP__
#define __WIN_IPEDIA_APPLICATION_HPP__

#include <windows.h>

#include "iPediaHyperlinkHandler.hpp"
#include <Logging.hpp>
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
    ushort_t                ticksPerSecond_;
    iPediaHyperlinkHandler  hyperlinkHandler_;
    LookupHistory*          history_;
    HINSTANCE               hInst_;
    
    typedef std::list<ArsLexis::String> CustomAlerts_t;
    CustomAlerts_t customAlerts_;

    HWND hwndMain_;    

protected:

    bool handleApplicationEvent(ArsLexis::EventType& event);

public:

    LookupManager*          lookupManager;

    const char_t* serverAddress;
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
    
    bool fLookupInProgress() const;

    ushort_t ticksPerSecond() const
    {return ticksPerSecond_;}
    
    struct Preferences
    {
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
    {sendEvent(appDisplayAlertEvent, DisplayAlertEventData(alertId));}
    
    ArsLexis::String popCustomAlert();
    
    void sendDisplayCustomAlertEvent(ushort_t alertId, const String& text1);
    
    static iPediaApplication& instance()
    {return instance_;}
            
    const LookupHistory& history() const
    {
        assert(0!=history_);
        return *history_;
    }
    
    iPediaHyperlinkHandler& hyperlinkHandler()
    {return hyperlinkHandler_;}

    void getErrorMessage(int alertId, bool customAlert, String& out);

    bool InitInstance (HINSTANCE hInstance, int CmdShow );
    
	BOOL InitApplication ( HINSTANCE hInstance );    
    
	bool initApplication(HINSTANCE hInstance, HINSTANCE hPrevInstance, const String& cmdLine, int cmdShow);

    HINSTANCE getApplicationHandle() {return hInst_;}

private:
    
    Preferences preferences_;
    static iPediaApplication instance_;
    bool logAllocation_;  

};

iPediaApplication& GetApp();

iPediaApplication::Preferences& GetPrefs();
#endif
