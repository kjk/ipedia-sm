#include "ProgressReporters.h"
#include "iPediaApplication.hpp"
#include <Graphics.hpp>

using ArsLexis::Graphics;

const uint_t CommonProgressReporter::refreshDelay = 250; 

CommonProgressReporter::CommonProgressReporter():
    ticksAtStart_(0),
    ticksAtUpdate_(0),
    lastPercent_(-1),
    showProgress_(true),
    afterTrigger_(false)
{
}

void CommonProgressReporter::update(u_int percent)
{
    lastPercent_ = percent;
    
    if (0==lastPercent_)
    {
        ticksAtStart_ = GetTickCount();
        ticksAtUpdate_ = ticksAtStart_;
        showProgress_=false;
        afterTrigger_=false;
        showProgress_ = false;
        return;
    }
    
    iPediaApplication& app=iPediaApplication::instance();
    if (!afterTrigger_)
    {
        // Delay before we start displaying progress meter in milliseconds. Timespans < 300ms are typically perceived "instant"
        // so we shouldn't distract user if the time is short enough.
        static const uint_t delay=100; 
        int ticksDiff=GetTickCount()-ticksAtStart_;
        ticksDiff*=1000;
        ticksDiff/=app.ticksPerSecond();
        if (ticksDiff>=delay)
            afterTrigger_=true;
        if (afterTrigger_ && percent<=20)
            showProgress_=true;
        return;
    }
    
    int ticksDiff=GetTickCount()-ticksAtUpdate_;
    ticksDiff*=1000;
    ticksDiff/=app.ticksPerSecond();
    if(ticksDiff<refreshDelay)
        showProgress_ = false;
    else
        showProgress_ = true;
}

void CommonProgressReporter::DrawProgressRect(HDC hdc, const ArsLexis::Rectangle& bounds)
{
    HBRUSH hbr = CreateSolidBrush(RGB(255,255,255));
    HBRUSH oldbr = (HBRUSH)SelectObject(hdc, hbr);
    int x = bounds.x();
    int y = bounds.y();
    int width = bounds.width();
    int height = bounds.height();
    Rectangle(hdc, x, y, x + width, y+height);
    
    POINT points[4];
    points[0].x = points[3].x = x +2;
    points[0].y = points[1].y = y +2;
    points[2].y = points[3].y = y + height - 3;
    points[1].x = points[2].x = x + width - 3;   
    Polygon(hdc, points,4);
    
    DeleteObject(SelectObject(hdc, oldbr));
}

void CommonProgressReporter::DrawProgressBar(Graphics& gr, uint_t percent, const ArsLexis::Rectangle& bounds)
{
    RECT nativeRec;
    bounds.toNative(nativeRec);
    nativeRec.top +=6;
    nativeRec.bottom = nativeRec.top + 17;
    nativeRec.left+=6;
    nativeRec.right-=6;

    HBRUSH hbr=CreateSolidBrush(RGB(180,180,180));
    FillRect(gr.handle(), &nativeRec, hbr);
    DeleteObject(hbr);

    nativeRec.right=nativeRec.left + ((nativeRec.right-nativeRec.left)*percent)/100;
    
    hbr=CreateSolidBrush(RGB(0,0,255));
    FillRect(gr.handle(), &nativeRec, hbr);
    DeleteObject(hbr);
}
void CommonProgressReporter::DrawProgressInfo(const ArsLexis::LookupProgressReportingSupport &support, ArsLexis::Graphics &offscreen, const ArsLexis::Rectangle &bounds, bool percentProgEnabled)
{
    DrawProgressRect(offscreen.handle(),bounds);
    
    if (support.percentProgress()!=support.percentProgressDisabled)
        DrawProgressBar(offscreen, support.percentProgress(), bounds);
    else
        DrawProgressBar(offscreen, 0, bounds);    
    
    offscreen.setTextColor(RGB(0,0,0));
    offscreen.setBackgroundColor(RGB(255,255,255));

    ArsLexis::Rectangle progressArea(bounds);    
    int h = progressArea.height();
    progressArea.explode(6, h - 20, -12, -h + 15);
    
    DefaultLookupProgressReporter::showProgress(support, offscreen, progressArea, false);
    
}

void CommonProgressReporter::refreshProgress(const ArsLexis::LookupProgressReportingSupport &support, ArsLexis::Graphics &gr, const ArsLexis::Rectangle& bounds)
{
    update(support.percentProgress());
    if(!shallShow())
        return;
    setTicksAtUpdate(GetTickCount());

    ArsLexis::Rectangle zeroBasedBounds = bounds;
    zeroBasedBounds.x() = 0;
    zeroBasedBounds.y() = 0;
    bool doubleBuffer = true;
    HDC offscreenDc=::CreateCompatibleDC(gr.handle());
    if (offscreenDc)
    {
        HBITMAP bitmap=::CreateCompatibleBitmap(gr.handle(), bounds.width(), bounds.height());
        if (bitmap) 
        {
            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
            {
                Graphics offscreen(offscreenDc, NULL);
                DrawProgressInfo(support, offscreen, zeroBasedBounds, true);
                offscreen.copyArea(zeroBasedBounds, gr, bounds.topLeft );
            }
            ::SelectObject(offscreenDc, oldBitmap);
            ::DeleteObject(bitmap);
        }
        else
            doubleBuffer=false;
        ::DeleteDC(offscreenDc);
    }
    else
        doubleBuffer=false;
    if (!doubleBuffer)
        DrawProgressInfo(support, gr, bounds, true);
}

RenderingProgressReporter::RenderingProgressReporter(HWND hwnd, RECT progressArea, ArsLexis::String& text):
    hwndMain_(hwnd),
    progressArea_(progressArea),
    waitText_(text)
{
    waitText_.c_str(); // We don't want reallocation to occur while rendering...
}


void RenderingProgressReporter::reportProgress(uint_t percent) 
{      

    ArsLexis::LookupProgressReportingSupport support;
    support.setPercentProgress(percent);
    support.setStatusText(waitText_);

    assert( hwndMain_ == g_hwndMain);
    Graphics gr(GetDC(hwndMain_), hwndMain_);
    
    ArsLexis::Rectangle bounds(progressArea_);
    refreshProgress(support, gr, bounds);
}



void DownloadingProgressReporter::showProgress(const ArsLexis::LookupProgressReportingSupport& support, Graphics& gr, const ArsLexis::Rectangle& bounds, bool clearBkg)
{
    refreshProgress(support, gr, bounds);
}

