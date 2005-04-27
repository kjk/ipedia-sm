#include "ProgressReporters.h"
#include "iPediaApplication.hpp"
#include <Graphics.hpp>
#include <UIHelper.h>

const uint_t CommonProgressReporter::refreshDelay = 100; 

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
        showProgress_ = false;
        afterTrigger_ = false;
        showProgress_ = false;
        return;
    }
    
    iPediaApplication& app=iPediaApplication::instance();
    if (!afterTrigger_)
    {
        // Delay before we start displaying progress meter in milliseconds. 
        // Timespans < 300ms are typically perceived "instant"
        // so we shouldn't distract user if the time is short enough.
        static const uint_t delay=100; 
        int ticksDiff=GetTickCount() - ticksAtStart_;
        ticksDiff = ticksDiff * 1000;
        ticksDiff = ticksDiff / app.ticksPerSecond();
        if (ticksDiff>=delay)
            afterTrigger_ = true;
        if (afterTrigger_ && percent<=20)
            showProgress_ = true;
        return;
    }
    
    int ticksDiff=GetTickCount()-ticksAtUpdate_;
    ticksDiff = ticksDiff * 1000;
    ticksDiff = ticksDiff / app.ticksPerSecond();
    if (ticksDiff<refreshDelay)
        showProgress_ = false;
    else
        showProgress_ = true;
}

void CommonProgressReporter::DrawProgressRect(HDC hdc, const ArsRectangle& bounds)
{
    HBRUSH hbr = CreateSolidBrush(RGB(255,255,255));
    HBRUSH oldbr = (HBRUSH)SelectObject(hdc, hbr);
    int x = bounds.x();
    int y = bounds.y();
    int width = bounds.width();
    int height = bounds.height();
    Rectangle(hdc, x, y, x + width, y+height);
    
    POINT points[4];
    points[0].x = points[3].x = x + SCALEX(2);
    points[0].y = points[1].y = y + SCALEY(2);
    points[2].y = points[3].y = y + height - SCALEY(3);
    points[1].x = points[2].x = x + width - SCALEX(3);   
    Polygon(hdc, points, 4);

    SelectObject(hdc, oldbr);
    DeleteObject(hbr);
}

void CommonProgressReporter::DrawProgressBar(Graphics& gr, uint_t percent, const ArsRectangle& bounds)
{
    RECT nativeRec;
    bounds.toNative(nativeRec);
    nativeRec.top += SCALEY(6);
    nativeRec.bottom = nativeRec.top + SCALEY(17);
    nativeRec.left += SCALEX(6);
    nativeRec.right -= SCALEX(6);

    HBRUSH hbr = CreateSolidBrush(RGB(180,180,180));
    FillRect(gr.handle(), &nativeRec, hbr);
    DeleteObject(hbr);

    nativeRec.right=nativeRec.left + ((nativeRec.right-nativeRec.left)*percent)/100;
    
    hbr = CreateSolidBrush(RGB(0,0,255));
    FillRect(gr.handle(), &nativeRec, hbr);
    DeleteObject(hbr);
}

void CommonProgressReporter::DrawProgressInfo(const LookupProgressReportingSupport &support, Graphics &offscreen, const ArsRectangle &bounds, bool percentProgEnabled)
{
    DrawProgressRect(offscreen.handle(), bounds);
    
    if (support.percentProgress()!=support.percentProgressDisabled)
        DrawProgressBar(offscreen, support.percentProgress(), bounds);
    else
        DrawProgressBar(offscreen, 0, bounds);    
    
    offscreen.setTextColor(RGB(0,0,0));
    offscreen.setBackgroundColor(RGB(255,255,255));

    ArsRectangle progressArea(bounds);    
    int h = progressArea.height();
    progressArea.explode(SCALEX(6), h - SCALEY(20), SCALEX(-12), - h + SCALEY(15));
    
    DefaultLookupProgressReporter::showProgress(support, offscreen, progressArea, false);    
}

void CommonProgressReporter::refreshProgress(const LookupProgressReportingSupport &support, Graphics &gr, const ArsRectangle& bounds)
{
    update(support.percentProgress());

    if(!shallShow())
        return;

    setTicksAtUpdate(GetTickCount());

    ArsRectangle zeroBasedBounds = bounds;
    zeroBasedBounds.x() = 0;
    zeroBasedBounds.y() = 0;

    bool fDidDoubleBuffer = false;
    HDC offscreenDc=::CreateCompatibleDC(gr.handle());
    if (offscreenDc)
    {
        Graphics offscreen(offscreenDc);
		offscreen.setFont(gr.font());
        HBITMAP bitmap=::CreateCompatibleBitmap(gr.handle(), bounds.width(), bounds.height());
        if (bitmap) 
        {
            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
            DrawProgressInfo(support, offscreen, zeroBasedBounds, true);
            offscreen.copyArea(zeroBasedBounds, gr, bounds.topLeft );
            ::SelectObject(offscreenDc, oldBitmap);
            ::DeleteObject(bitmap);
            fDidDoubleBuffer = true;
        }
    }
    if (!fDidDoubleBuffer)
        DrawProgressInfo(support, gr, bounds, true);
}

RenderingProgressReporter::RenderingProgressReporter(HWND hwnd, RECT progressArea, const char_t* text):
    hwndMain_(hwnd),
    progressArea_(progressArea),
    waitText_(text)
{
}


void RenderingProgressReporter::reportProgress(uint_t percent) 
{
    LookupProgressReportingSupport support;
    support.setPercentProgress(percent);
    support.setStatusText(waitText_);

    Graphics gr(hwndMain_);
    
	WinFont font;
	font.getSystemFont();
	gr.setFont(font);

    ArsRectangle bounds(progressArea_);
    refreshProgress(support, gr, bounds);
}

void DownloadingProgressReporter::showProgress(const LookupProgressReportingSupport& support, Graphics& gr, const ArsRectangle& bounds, bool clearBkg)
{
    refreshProgress(support, gr, bounds);
}

