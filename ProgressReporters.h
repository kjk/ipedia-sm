#ifndef __PROGRESS_REPORTERS_H__
#define __PROGRESS_REPORTERS_H__

#include <LookupManagerBase.hpp>
#include <Definition.hpp>

class CommonProgressReporter : public ArsLexis::DefaultLookupProgressReporter
{
protected:    
    CommonProgressReporter();
    void refreshProgress(const ArsLexis::LookupProgressReportingSupport &support, ArsLexis::Graphics &gr, const ArsLexis::Rectangle& bounds);

protected:
    DWORD ticksAtUpdate_;
    DWORD ticksAtStart_;
    uint_t lastPercent_;
    bool showProgress_:1;
    bool afterTrigger_:1;
    static const uint_t refreshDelay; 

    void update(uint_t percent);
    void setTicksAtUpdate(DWORD ticks)
    {ticksAtUpdate_ = ticks;}
    bool shallShow()
    {return showProgress_;}
    void DrawProgressInfo(const ArsLexis::LookupProgressReportingSupport &support, ArsLexis::Graphics &offscreen, const ArsLexis::Rectangle &bounds, bool percentProgEnabled);
    void DrawProgressBar(ArsLexis::Graphics& gr, uint_t percent, const ArsLexis::Rectangle& bounds);
    void DrawProgressRect(HDC hdc, const ArsLexis::Rectangle& bounds);


};

class RenderingProgressReporter: public Definition::RenderingProgressReporter, CommonProgressReporter
{
    HWND hwndMain_;
    ArsLexis::String waitText_;
    RECT progressArea_;
public:
    RenderingProgressReporter(HWND hwnd, RECT progressArea, ArsLexis::String& text);
    void setProgressArea(const RECT &progressArea)
    {progressArea_=progressArea;}
    virtual void reportProgress(uint_t percent);
};

class DownloadingProgressReporter: public CommonProgressReporter
{
    void showProgress(const ArsLexis::LookupProgressReportingSupport& support, ArsLexis::Graphics& gr, const ArsLexis::Rectangle& bounds, bool clearBkg=true);
};

#endif