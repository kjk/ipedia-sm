#ifndef __WIN_IPEDIA_APPLICATION_HPP__
#define __WIN_IPEDIA_APPLICATION_HPP__

//#include "ipedia.h"
//#include <Application.hpp>
//#include <DynamicInputAreas.hpp>
#include <Logging.hpp>
#include <RenderingPreferences.hpp>
//#include "iPediaHyperlinkHandler.hpp"
#include <SysUtils.hpp>
#include <BaseTypes.hpp>

class LookupManager;
class LookupHistory;
struct LookupFinishedEventData;

#define serverLocalhost        _T("arslex.no-ip.info:9000")
//#define serverLocalhost    _T("arslex.no-ip.info:9000")
#define serverIpediaArslexis   _T("ipedia.arslexis.com:9000")

#define serverToUse serverLocalhost

class iPediaApplication //: public ArsLexis::Application 
{
    mutable ArsLexis::RootLogger log_;
    //ArsLexis::DIA_Support diaSupport_;
    ushort_t ticksPerSecond_;
    //iPediaHyperlinkHandler hyperlinkHandler_;
    LookupHistory* history_;
    LookupManager* lookupManager_;
    ArsLexis::String server_;
    
    typedef std::list<ArsLexis::String> CustomAlerts_t;
    CustomAlerts_t customAlerts_;

    void detectViewer();
    
    void loadPreferences();

    void savePreferences();
    
    HWND hwndMain_;    
    
protected:

    //ArsLexis::status_t handleSystemNotify(SysNotifyParamType& notify);
    
    ArsLexis::status_t normalLaunch();

    
    
    //ArsLexis::Form* createForm(ushort_t formId);

    bool handleApplicationEvent(ArsLexis::EventType& event);
    
public:

    //static const tick_t requiredRomVersion=sysMakeROMVersion(3,5,0,sysROMStageDevelopment,0);
    //static const tick_t creatorId=appFileCreator;
    //static const ushort_t notEnoughMemoryAlertId=notEnoughMemoryAlert;
    //static const ushort_t romIncompatibleAlertId=romIncompatibleAlert;
    
    //const ArsLexis::DIA_Support& getDIASupport() const
    //{return diaSupport_;}

    iPediaApplication();

    DWORD waitForEvent();
    
    HWND getMainWindow()
    {return hwndMain_ ;}
    
    void setMainWindow(HWND hwndMain)
    {hwndMain_=hwndMain;}

    ~iPediaApplication();
    
    ArsLexis::status_t initialize();
    
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

        //enum {articleCountNotChecked=-1L};
        
        long articleCount;
        
        Preferences():
            //serialNumberRegistered(false),
            //checkArticleCountAtStartup(true),
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
    
    /*const ArsLexis::DIA_Support& diaSupport() const
    {return diaSupport_;}*/
    
    enum {
        reservedLookupEventsCount=3
    };

    enum Event
    {
        appDisplayAlertEvent=WM_USER,
        appDisplayCustomAlertEvent,
        appLookupEventFirst,
        appLookupEventLast=appLookupEventFirst+reservedLookupEventsCount,
        appGetArticlesCountEvent,
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
    
    bool inStressMode() const
    {return stressMode_;}
    
    void toggleStressMode(bool enable)
    {stressMode_=enable;}
    
    const LookupHistory& history() const
    {
        assert(0!=history_);
        return *history_;
    }
    
    bool hasHighDensityFeatures() const
    {return hasHighDensityFeatures_;}

    /*iPediaHyperlinkHandler& hyperlinkHandler()
    {return hyperlinkHandler_;}    */
    bool fArticleCountChecked;
private:
    
    Preferences preferences_;
    static iPediaApplication instance_;
    bool diaNotifyRegistered_:1;
    bool stressMode_:1;
    bool hasHighDensityFeatures_:1;
    bool logAllocation_;  
};

#endif
