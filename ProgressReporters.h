#ifndef __PROGRESS_REPORTERS_H__
#define __PROGRESS_REPORTERS_H__

#include <LookupManagerBase.hpp>
#include <Definition.hpp>

class CommonProgressReporter : public DefaultLookupProgressReporter
{
protected:    
    CommonProgressReporter();
    void refreshProgress(const LookupProgressReportingSupport& support, Graphics &gr, const ArsRectangle& bounds);

protected:
    DWORD ticksAtUpdate_;
    DWORD ticksAtStart_;
    uint_t lastPercent_;
    bool showProgress_;
    bool afterTrigger_;
    static const uint_t refreshDelay; 

    void update(uint_t percent);
    void setTicksAtUpdate(DWORD ticks)
    {ticksAtUpdate_ = ticks;}
    bool shallShow()
    {return showProgress_;}
    void DrawProgressInfo(const LookupProgressReportingSupport &support, Graphics &offscreen, const ArsRectangle &bounds, bool percentProgEnabled);
    void DrawProgressBar(Graphics& gr, uint_t percent, const ArsRectangle& bounds);
    void DrawProgressRect(HDC hdc, const ArsRectangle& bounds);
};

class RenderingProgressReporter: public Definition::RenderingProgressReporter, CommonProgressReporter
{
    HWND hwndMain_;
    const char_t* waitText_;
    RECT progressArea_;
public:
    RenderingProgressReporter(HWND hwnd, RECT progressArea, const char_t* text);
    void setProgressArea(const RECT &progressArea)
    {progressArea_=progressArea;}
    virtual void reportProgress(uint_t percent);
};

class DownloadingProgressReporter: public CommonProgressReporter
{
    void showProgress(const LookupProgressReportingSupport& support, Graphics& gr, const ArsRectangle& bounds, bool clearBkg=true);
};

#endif