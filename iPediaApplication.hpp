#ifndef __WIN_IPEDIA_APPLICATION_HPP__
#define __WIN_IPEDIA_APPLICATION_HPP__

#include "iPediaHyperlinkHandler.hpp"
#include <Logging.hpp>
#include <RenderingPreferences.hpp>
#include <SysUtils.hpp>
#include <BaseTypes.hpp>

class LookupManager;
class LookupHistory;
struct LookupFinishedEventData;

#define serverMarek          _T("arslex.no-ip.info:9000")
#define serverKjk            _T("dict-pc.arslexis.com:9000")
#define serverKjkLaptop      _T("192.168.123.150:9000")
#define serverLocalhost      _T("192.168.0.1:9000")
#define serverLocalhost2     _T("127.0.0.1:9000")
#define serverIpediaArslexis _T("ipedia.arslexis.com:9000")

#define serverToUse serverIpediaArslexis

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
    void loadPreferences();

    HWND hwndMain_;    

protected:

    bool handleApplicationEvent(ArsLexis::EventType& event);

public:
    bool fArticleCountChecked;

    void savePreferences();

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
        appFirstAvailableEvent
    };

    struct DisplayAlertEventData
    {
        ushort_t alertId;
        
        DisplayAlertEventData(ushort_t aid):
            alertId(aid) {}
        
        DisplayAlertEventData():
            alertId(0) {}

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
    
    bool hasHighDensityFeatures() const
    {return hasHighDensityFeatures_;}

    iPediaHyperlinkHandler& hyperlinkHandler()
    {return hyperlinkHandler_;}


    struct ErrorInfo
    {
        int errorCode;
        ArsLexis::String title;
        ArsLexis::String message;
    
        ErrorInfo(int eCode, ArsLexis::String tit, ArsLexis::String msg):
            errorCode(eCode),
            title(tit),
            message(msg)
        {}

    };
    void getErrorMessage(int alertId, bool customAlert, ArsLexis::String &out);
    void getErrorTitle(int alertId, ArsLexis::String &out);

    bool InitInstance (HINSTANCE hInstance, int CmdShow );
    BOOL InitApplication ( HINSTANCE hInstance );    
    bool initApplication(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                         ArsLexis::String cmdLine, int cmdShow);
    HINSTANCE getApplicationHandle()
    {return hInst_;}
private:
    
    Preferences preferences_;
    static iPediaApplication instance_;
    bool diaNotifyRegistered_:1;
    bool hasHighDensityFeatures_:1;
    bool logAllocation_;  
    static const ErrorInfo ErrorsTable[];
    const ErrorInfo& getErrorInfo(int alertID);
    static const ArsLexis::char_t szAppName[];
    static const ArsLexis::char_t szTitle[];

};

#endif
