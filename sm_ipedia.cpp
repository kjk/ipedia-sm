// sm_ipedia.cpp : Defines the entry point for the application.
//

#include "sm_ipedia.h"
#include "LastResults.h"
#include "Registration.h"

#include <SysUtils.hpp>
#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include <LookupManagerBase.hpp>
#include <LookupHistory.hpp>
#include <ipedia_rsc.h>

#include <Definition.hpp>
#include <Debug.hpp>

#include <objbase.h>
#include <initguid.h>
#include <connmgr.h>

#include <windows.h>
#include <aygshell.h>
#include <tpcshell.h>
#include <wingdi.h>
#include <fonteffects.hpp>
#include <sms.h>
#include <uniqueid.h>

const int ErrorsTableEntries = 16;

ErrorsTableEntry ErrorsTable[ErrorsTableEntries] =
{   
    ErrorsTableEntry(
        romIncompatibleAlert,
        _T("System Incompatible"),
        _T("System Version 4.0 or greater is required to run iPedia.")
    ),

    ErrorsTableEntry(
        networkUnavailableAlert,
        _T("Network Unavailable"),
        _T("Unable to initialize network subsystem.")
    ),
    
    ErrorsTableEntry(
        cantConnectToServerAlert,
        _T("Server unavailable"),
        _T("Can't connect to the server.")
    ),
    
    ErrorsTableEntry(
        articleNotFoundAlert,
        _T("Article not found"),
        _T("Encyclopedia article for '^1' was not found.")
    ),

    ErrorsTableEntry(
        articleTooLongAlert,
        _T("Article too long"),
        _T("Article is too long for iPedia to process.")
    ),

    ErrorsTableEntry(
        notEnoughMemoryAlert,
        _T("Error"),
        _T("Not enough memory to complete current operation.")
    ),

    ErrorsTableEntry(
        serverFailureAlert,
        _T("Server Error"),
        _T("Unable to complete request due to server error.")
    ),

    ErrorsTableEntry(
        invalidAuthorizationAlert,
        _T("Invalid Authorization"),
        _T("Unable to complete request due to invalid authorization data. Please check if you entered serial number correctly.")
    ),

    ErrorsTableEntry(
        trialExpiredAlert,
        _T("Expired"),
        _T("Your unregistered trial version expired. Please register to remove daily requests limit.")
    ),

    ErrorsTableEntry(
        unsupportedDeviceAlert,
        _T("Unsupported Device"),
        _T("Your hardware configuration is unsupported.")
    ),

    ErrorsTableEntry(
        malformedRequestAlert,
        _T("Malformed Request"),
        _T("Server rejected your query.")
    ),

    ErrorsTableEntry(
        noWebBrowserAlert,
        _T("No web browser"),
        _T("Web browser is not installed on this device.")
    ),

    ErrorsTableEntry(
        malformedResponseAlert,
        _T("Malformed Response"),
        _T("Server returned malformed response.")
    ),

    ErrorsTableEntry(
        connectionTimedOutAlert,
        _T("Timed Out"),
        _T("Connection timed out.")
    ),

    ErrorsTableEntry(
        connectionErrorAlert,
        _T("Error"),
        _T("Connection terminated.")
    ),

    ErrorsTableEntry(
        connectionServerNotRunning,
        _T("Error"),
        _T("The iPedia server is not available. Please contact support@arslexis.com.")
    )
};

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
WNDPROC oldEditWndProc;

iPediaApplication iPediaApplication::instance_;

Definition *definition_ = new Definition();
RenderingProgressReporter* rep; 
RenderingPreferences* prefs= new RenderingPreferences();
static bool g_forceLayoutRecalculation=false;
bool rec=false;
void setScrollBar(Definition* definition_);

using ArsLexis::String;
using ArsLexis::Graphics;
using ArsLexis::char_t;


String articleCountText;
String recentWord;
String searchWord;


HINSTANCE g_hInst = NULL;  // Local copy of hInstance
HWND hwndMain = NULL;    // Handle to Main window returned from CreateWindow


void paint(HWND hwnd, HDC hdc, RECT rcpaint);
void DrawProgressBar(Graphics& gr, uint_t percent, const ArsLexis::Rectangle& bounds);
RECT progressRect = { 7, 80, 162, 125 };

TCHAR szAppName[] = TEXT("iPedia");
TCHAR szTitle[]   = TEXT("iPedia");
//TCHAR szMessage[] = TEXT("Enter article name and press Search.");
HWND hwndEdit;
HWND hwndScroll;

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT		lResult = TRUE;
	HDC			hdc;
	bool customAlert = false;

    switch(msg)
    {
        case WM_CREATE:
        {
            // create the menu bar
            SHMENUBARINFO mbi;
            ZeroMemory(&mbi, sizeof(SHMENUBARINFO));
            mbi.cbSize = sizeof(SHMENUBARINFO);
            mbi.hwndParent = hwnd;
            mbi.nToolBarId = IDR_MAIN_MENUBAR;
            mbi.hInstRes = g_hInst;

            if (!SHCreateMenuBar(&mbi)) {
                PostQuitMessage(0);
            }

            hwndEdit = CreateWindow(
                TEXT("edit"),
                NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | 
                WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
                0,0,0,0,hwnd,
                (HMENU) ID_EDIT,
                ((LPCREATESTRUCT)lp)->hInstance,
                NULL);
            oldEditWndProc=(WNDPROC)SetWindowLong(hwndEdit, GWL_WNDPROC, (LONG)EditWndProc);

            hwndScroll = CreateWindow(
                TEXT("scrollbar"),
                NULL,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP| SBS_VERT,
                0,0,0,0,hwnd,
                (HMENU) ID_SCROLL,
                ((LPCREATESTRUCT)lp)->hInstance,
                NULL);
            
            setScrollBar(definition_);
            (void)SendMessage(
                mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
                MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
                SHMBOF_NODEFAULT | SHMBOF_NOTIFY)
                );
            iPediaApplication& app=iPediaApplication::instance();
            LookupManager* lookupManager=app.getLookupManager(true);
            app.setMainWindow(hwnd);
            //if (lookupManager && !lookupManager->lookupInProgress())
            //    lookupManager->checkArticleCount();
            lookupManager->setProgressReporter(new SmartPhoneProgressReported());
            rep = new RenderingProgressReporter(hwnd);
            definition_->setRenderingProgressReporter(rep);
            break;
        
        }
        case WM_SIZE:
            MoveWindow(hwndEdit,2,2,LOWORD(lp)-4,20,TRUE);
            MoveWindow(hwndScroll,LOWORD(lp)-5, 28 , 5, HIWORD(lp)-28, false);
            break;

        case WM_SETFOCUS:
            SetFocus(hwndEdit);
            break;

        case WM_COMMAND:
        {
            switch (wp)
            {
                case IDOK:
                {
                    SendMessage(hwnd,WM_CLOSE,0,0);
                    break;
                }
                
                case ID_SEARCH:
                {
                    int len = SendMessage(hwndEdit, EM_LINELENGTH, 0,0);
                    TCHAR *buf=new TCHAR[len+1];
                    len = SendMessage(hwndEdit, WM_GETTEXT, len+1, (LPARAM)buf);
                    SendMessage(hwndEdit, EM_SETSEL, 0,len);
                    String word(buf); 
                    iPediaApplication& app=iPediaApplication::instance();
                    LookupManager* lookupManager=app.getLookupManager(true);
                    if (lookupManager && !lookupManager->lookupInProgress())
                        lookupManager->lookupTerm(word);
                    delete buf;
                    InvalidateRect(hwnd,NULL,TRUE);
                    break;            
                }
                
                case IDM_MENU_RANDOM:
                {
                    iPediaApplication& app=iPediaApplication::instance();
                    LookupManager* lookupManager=app.getLookupManager(true);
                    if (lookupManager && !lookupManager->lookupInProgress())
                        lookupManager->lookupRandomTerm();
                    break;
                }
                case IDM_MENU_REGISTER:
                {
                    if(DialogBox(g_hInst, MAKEINTRESOURCE(IDD_REGISTER), hwnd,RegistrationDlgProc))
                    {
                        iPediaApplication& app=iPediaApplication::instance();
                        LookupManager* lookupManager=app.getLookupManager(true);
                        if (lookupManager && !lookupManager->lookupInProgress())
                            lookupManager->verifyRegistrationCode(newRegCode_);
                    }

                    break;
                }

                case IDM_MENU_PREV:
                case IDM_MENU_NEXT:
                {
                    iPediaApplication& app=iPediaApplication::instance();
                    LookupManager* lookupManager=app.getLookupManager(true);
                    if (lookupManager && !lookupManager->lookupInProgress())
                        lookupManager->moveHistory(!(wp-IDM_MENU_NEXT));
                    break;
                }
                case IDM_MENU_RESULTS:
                {
                    iPediaApplication& app=iPediaApplication::instance();
                    LookupManager* lookupManager=app.getLookupManager();    

                    if(DialogBox(g_hInst, MAKEINTRESOURCE(IDD_LAST_RESULTS), hwnd,LastResultsDlgProc))
                    {
                        iPediaApplication& app=iPediaApplication::instance();
                        LookupManager* lookupManager=app.getLookupManager(true);
                        if (lookupManager && !lookupManager->lookupInProgress())
                            lookupManager->lookupTerm(searchWord);
                        SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)recentWord.c_str());
                        int len = SendMessage(hwndEdit, EM_LINELENGTH, 0,0);
                        SendMessage(hwndEdit, EM_SETSEL, 0,len);

                        InvalidateRect(hwnd,NULL,TRUE);
                    }
                    break;
                }
        		default:
		        	lResult = DefWindowProc(hwnd, msg, wp, lp);
            }
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT	ps;
            hdc = BeginPaint (hwnd, &ps);
            /*GetClientRect (hwnd, &rect);
            DrawText (hdc, TEXT("Enter article name "),-1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
            rect.top+=38;
            DrawText (hdc, TEXT("and press \"Search\" button."),-1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);*/
            paint(hwnd, hdc, ps.rcPaint);
            EndPaint (hwnd, &ps);
            break;
        }
        
        case WM_HOTKEY:
        {
            Graphics gr(GetDC(hwndMain), hwndMain);
            int page=0;
            if (definition_)
                page=definition_->shownLinesCount();
            switch(HIWORD(lp))
            {
                case VK_TBACK:
                    if ( 0 != (MOD_KEYUP & LOWORD(lp)))
                        SHSendBackToFocusWindow( msg, wp, lp );
                    break;
                case VK_TDOWN:
                    if(definition_)
                        definition_->scroll(gr,*prefs,page);
                    setScrollBar(definition_);
                    break;
            }
            break;
        }    
        case iPediaApplication::appDisplayCustomAlertEvent:
            customAlert = true;
        case iPediaApplication::appDisplayAlertEvent:
        {
            iPediaApplication& app=iPediaApplication::instance();
            iPediaApplication::DisplayAlertEventData data;
            ArsLexis::EventData i;
            i.wParam=wp; i.lParam=lp;
            memcpy(&data, &i, sizeof(data));
            for(int j=0; j<ErrorsTableEntries; j++)
            {
                if(ErrorsTable[j].errorCode == data.alertId)
                {
                    String msg = ErrorsTable[j].message;
                    if(customAlert)
                    {
                        int pos=msg.find(_T("^1"));
                        msg.replace(pos,2,app.popCustomAlert().c_str());
                    }
                    MessageBox(hwndMain, 
                        msg.c_str(),
                        ErrorsTable[j].title.c_str(),
                        MB_OK|MB_ICONEXCLAMATION);
                    break;
                }
            }
            break;
        }
        
        case LookupManager::lookupStartedEvent:
        case LookupManager::lookupProgressEvent:
        {
            InvalidateRect(hwndMain, &progressRect, FALSE);
            break;
        }

        case LookupManager::lookupFinishedEvent:
        {
            iPediaApplication& app=iPediaApplication::instance();
            LookupManager* lookupManager=app.getLookupManager(true);
            LookupFinishedEventData data;
            ArsLexis::EventData i;
            i.wParam=wp; i.lParam=lp;
            memcpy(&data, &i, sizeof(data));
            switch (data.outcome)
            {
                case LookupFinishedEventData::outcomeArticleBody:
                {   
                    assert(lookupManager!=0);
                    if (lookupManager)
                    {
                        definition_->replaceElements(lookupManager->lastDefinitionElements());
                        g_forceLayoutRecalculation=true;
                        //const LookupHistory& history=app.history();
                    }
                    InvalidateRect(hwndMain, NULL, TRUE);
                    break;
                }

                case LookupFinishedEventData::outcomeList:
                    //DialogBox(g_hInst, MAKEINTRESOURCE(IDD_LAST_RESULTS), hwnd,LastResultsDlgProc);                        
                    recentWord.assign(lookupManager->lastInputTerm());
                    SendMessage(hwnd, WM_COMMAND, IDM_MENU_RESULTS, 0);
                    break;
                
                case LookupFinishedEventData::outcomeRegCodeValid:
                {
                    iPediaApplication::Preferences& prefs=iPediaApplication::instance().preferences();
                    assert(!newRegCode_.empty());
                    // TODO: assert that it consists of numbers only
                    if (newRegCode_!=prefs.regCode)
                    {
                        assert(newRegCode_.length()<=prefs.regCodeLength);
                        prefs.regCode=newRegCode_;
                        //app.savePreferences();
                    }   

                    //FrmAlert(alertRegistrationOk);
                    //closePopup();
                }
                
                case LookupFinishedEventData::outcomeRegCodeInvalid:
                {
                    iPediaApplication::Preferences& prefs=iPediaApplication::instance().preferences();
                    // TODO: should it be done as a message to ourselves?
                    //UInt16 buttonId;
                    //buttonId = FrmAlert(alertRegistrationFailed);
            
                    /*if (0==buttonId)
                    {
                        // this is "Ok" button. Clear-out registration code (since it was invalid)
                        prefs.regCode = "";
                        app.savePreferences();
                        closePopup();
                        return;
                    } */  
                    // this must be "Re-enter registration code" button
                    //assert(1==buttonId);
                }
            }   
            
            assert(0!=app.preferences().articleCount);
            if(app.preferences().articleCount!=-1)
            {
                articleCountText.assign(_T("Number of articles: "));
                char_t buffer[16];
                int len= tprintf(buffer, _T("%ld"), app.preferences().articleCount);
                articleCountText.append(buffer, len);
            }
            lookupManager->handleLookupFinishedInForm(data);
            InvalidateRect(hwnd,NULL,TRUE);
        }
        break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;

        case WM_DESTROY:
            PostQuitMessage(0);
        break;

		default:
			lResult = DefWindowProc(hwnd, msg, wp, lp);
  		break;
        
	}
	return (lResult);
}


//
//  FUNCTION: InitInstance(HANDLE, int)
//
//  PURPOSE: Saves instance handle and creates main window
//
//  COMMENTS:
//
//    In this function, we save the instance handle in a global variable and
//    create and display the main program window.
//
BOOL InitInstance (HINSTANCE hInstance, int CmdShow )
{

	g_hInst = hInstance;
	hwndMain = CreateWindow(szAppName,						
                	szTitle,
					WS_VISIBLE,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					NULL, NULL, hInstance, NULL );

	if ( !hwndMain )		
	{
		return FALSE;
	}
	ShowWindow(hwndMain, CmdShow );
	UpdateWindow(hwndMain);
	return TRUE;
}

//
//  FUNCTION: InitApplication(HANDLE)
//
//  PURPOSE: Sets the properties for our window.
//
BOOL InitApplication ( HINSTANCE hInstance )
{
	WNDCLASS wc;
	BOOL f;

	wc.style = CS_HREDRAW | CS_VREDRAW ;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = NULL;
	wc.hInstance = hInstance;
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH) GetStockObject( WHITE_BRUSH );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = szAppName;
	
	f = (RegisterClass(&wc));

	return f;
}


//
//  FUNCTION: WinMain(HANDLE, HANDLE, LPWSTR, int)
//
//  PURPOSE: Entry point for the application
//
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPWSTR     lpCmdLine,
                   int        CmdShow)

{
	HWND hHelloWnd = NULL;	
    
	//Check if Hello.exe is running. If it's running then focus on the window
	hHelloWnd = FindWindow(szAppName, szTitle);	
	if (hHelloWnd) 
	{
		SetForegroundWindow (hHelloWnd);    
		return 0;
	}

	if ( !hPrevInstance )
	{
		if ( !InitApplication ( hInstance ) )
		{ 
			return (FALSE); 
		}

	}
	if ( !InitInstance( hInstance, CmdShow )  )
	{
		return (FALSE);
	}
    
    return iPediaApplication::instance().waitForEvent();
}

void setScrollBar(Definition* definition_)
{
    int frst=0;
    int total=0;
    int shown=0;
    if(definition_)
    {
        frst=definition_->firstShownLine();
        total=definition_->totalLinesCount();
        shown=definition_->shownLinesCount();
    }
    
    SetScrollPos(
        hwndScroll, 
        SB_CTL,
        frst,
        TRUE);
    
    SetScrollRange(
        hwndScroll,
        SB_CTL,
        0,
        total-shown,
        TRUE);
}

void ArsLexis::handleBadAlloc()
{
    RaiseException(1,0,0,NULL);    
}

void paint(HWND hwnd, HDC hdc, RECT rcpaint)
{
    RECT rect;
    GetClientRect (hwnd, &rect);
    ArsLexis::Rectangle rin(rcpaint);
    ArsLexis::Rectangle rout(progressRect);
    
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager(true);
    //Definition::Elements_t defels_=lookupManager->lastDefinitionElements();

    bool onlyProgress=false;
    //FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
    if (lookupManager && lookupManager->lookupInProgress() &&
        (rout&&rin.topLeft) && (rout.extent.x>=rin.extent.x)
        && (rout.extent.y>=rin.extent.y))
            onlyProgress = true;

    rect.top    +=22;
    rect.left   +=2;
    rect.right  -=7;
    rect.bottom -=2;
    Graphics gr(hdc, hwnd);
    


    if(!onlyProgress)
    {
        if (definition_->empty())
        {
            LOGFONT logfnt;
            HFONT   fnt=(HFONT)GetStockObject(SYSTEM_FONT);
            GetObject(fnt, sizeof(logfnt), &logfnt);
            logfnt.lfHeight+=1;
            int fontDy = logfnt.lfHeight;
            HFONT fnt2=(HFONT)CreateFontIndirect(&logfnt);
            SelectObject(hdc, fnt2);
            
            RECT tmpRect=rect;
            DrawText(hdc, TEXT("(enter article name and press"), -1, &tmpRect, DT_SINGLELINE|DT_CENTER);
            tmpRect.top += 16;
            
            DrawText(hdc, TEXT("\"Lookup\")"), -1, &tmpRect, DT_SINGLELINE|DT_CENTER);
            
            //tmpRect.top += (fontDy*3);
            tmpRect.top += 40;
            DrawText(hdc, TEXT("ArsLexis iPedia 1.0"), -1, &tmpRect, DT_SINGLELINE|DT_CENTER);
            // tmpRect.top += fontDy+6;
            tmpRect.top += 18;
            DrawText(hdc, TEXT("http://www.arslexis.com"), -1, &tmpRect, DT_SINGLELINE|DT_CENTER);
            tmpRect.top += 18;
            DrawText(hdc, articleCountText.c_str(), -1, &tmpRect, DT_SINGLELINE|DT_CENTER);
            SelectObject(hdc,fnt);
            DeleteObject(fnt2);
        }
        else
        {
            RECT b;
            GetClientRect(hwnd, &b);
            ArsLexis::Rectangle bounds=b;
            ArsLexis::Rectangle defRect=rect;
            bool doubleBuffer=true;
            HDC offscreenDc=::CreateCompatibleDC(hdc);
            if (offscreenDc) {
                HBITMAP bitmap=::CreateCompatibleBitmap(hdc, bounds.width(), bounds.height());
                if (bitmap) {
                    HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
                    {
                        Graphics offscreen(offscreenDc, NULL);
                        definition_->render(offscreen, defRect, *prefs, g_forceLayoutRecalculation);
                        offscreen.copyArea(defRect, gr, defRect.topLeft);
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
                definition_->render(gr, defRect, *prefs, g_forceLayoutRecalculation);
            if(g_forceLayoutRecalculation) 
                setScrollBar(definition_);
            g_forceLayoutRecalculation=false;
        }
    }
    if(rec)
    {
        setScrollBar(definition_);
        rec=false;
    }

    if (lookupManager && lookupManager->lookupInProgress())
    {
        Graphics gr(GetDC(hwndMain), hwndMain);    
        ArsLexis::Rectangle progressArea(progressRect);
        lookupManager->showProgress(gr, progressArea);
    }
}

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_KEYDOWN:
        {
            if(definition_)
            {
                int page=0;
                switch (wp) 
                {
                    case VK_DOWN:
                        page=definition_->shownLinesCount();
                        break;
                    case VK_UP:
                        page=-static_cast<int>(definition_->shownLinesCount());
                        break;
                }
                if (page!=0)
                {
                    RECT b;
                    GetClientRect (hwndMain, &b);
                    ArsLexis::Rectangle bounds=b;
                    ArsLexis::Rectangle defRect=bounds;
                    defRect.explode(2, 22, -9, -24);
                    Graphics gr(GetDC(hwndMain), hwndMain);
                    bool doubleBuffer=true;
                    
                    HDC offscreenDc=::CreateCompatibleDC(gr.handle());
                    if (offscreenDc) {
                        HBITMAP bitmap=::CreateCompatibleBitmap(gr.handle(), bounds.width(), bounds.height());
                        if (bitmap) {
                            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
                            {
                                Graphics offscreen(offscreenDc, NULL);
                                definition_->scroll(offscreen,*prefs, page);
                                offscreen.copyArea(defRect, gr, defRect.topLeft);
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
                        definition_->scroll(gr,*prefs, page);
                    
                    setScrollBar(definition_);
                    return 0;
                }
            }
            break;
       }
    }
    return CallWindowProc(oldEditWndProc, hwnd, msg, wp, lp);
}

void DrawProgressRect(HDC hdc, const ArsLexis::Rectangle& bounds)
{

    HBRUSH hbr = CreateSolidBrush(RGB(255,255,255));
    SelectObject(hdc, hbr);
    Rectangle(hdc, 
        bounds.x(), 
        bounds.y(), 
        bounds.x()+bounds.width(), 
        bounds.y()+bounds.height() 
        );
    
    POINT points[4];
    points[0].x = points[3].x = bounds.x() +2;
    points[0].y = points[1].y = bounds.y() +2;
    points[2].y = points[3].y = bounds.y() + bounds.height() - 3;
    points[1].x = points[2].x = bounds.x() + bounds.width() - 3;
    
    Polygon(hdc, points,4);
    /*points[0].x = points[3].x = bounds.x - 2;
    points[0].y = points[1].y = bounds.y -2;
    points[2].y = points[3].y = bounds.y - 2 + bounds.height() - 4;
    points[1].x = points[2].x = bounds.x -2 + bounds.width() - 4;
    Polygon(hdc, points,4);*/

}

RenderingProgressReporter::RenderingProgressReporter(HWND hwnd):
    hwndMain_(hwnd),
    ticksAtStart_(0),
    lastPercent_(-1),
    showProgress_(true),
    afterTrigger_(false)
{
    waitText_.assign(_T("Wait... %d%%"));
    waitText_.c_str(); // We don't want reallocation to occur while rendering...
}

void RenderingProgressReporter::reportProgress(uint_t percent) 
{      
    if (percent==lastPercent_)
        return;
    lastPercent_=percent;

    Graphics gr(GetDC(hwndMain), hwndMain_);
    
    ArsLexis::Rectangle bounds(progressRect);
    DrawProgressRect(gr.handle(),bounds);

    DrawProgressBar(gr, percent, bounds);

    ArsLexis::Rectangle progressArea(bounds);    
    int h = progressArea.height();
    progressArea.explode(6, h - 20, -12, -h + 15);

    gr.setFont(WinFont());
    String str(_T("Formatting article..."));
    uint_t length=str.length();
    uint_t width=bounds.width();
    gr.charsInWidth(str.c_str(), length, width);
    uint_t height=gr.fontHeight();
    ArsLexis::Point p(progressArea.topLeft);
    gr.drawText(str.c_str(), length, p);
    //gr.drawText(str.c_str(), length,progressArea.topLeft);

    char_t buffer[16];
    //assert(support.percentProgress()<=100);
    length=tprintf(buffer, _T(" %hu%%"), percent);

    
    p.x+=width+2;
    width=bounds.width()-width;
    gr.charsInWidth(buffer, length, width);
    gr.drawText(buffer, length, p);
}

void DrawProgressBar(Graphics& gr, uint_t percent, const ArsLexis::Rectangle& bounds)
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

void SmartPhoneProgressReported::showProgress(const ArsLexis::LookupProgressReportingSupport& support, Graphics& gr, const ArsLexis::Rectangle& bounds, bool clearBkg)
{
    
    DrawProgressRect(gr.handle(),bounds);

    if(support.percentProgress()!=support.percentProgressDisabled)
        DrawProgressBar(gr, support.percentProgress(), bounds);
    else
        DrawProgressBar(gr, 0, bounds);
    gr.setTextColor(RGB(0,0,0));
    gr.setBackgroundColor(RGB(255,255,255));
    
    ArsLexis::Rectangle progressArea(bounds);    
    int h = progressArea.height();
    progressArea.explode(6, h - 20, -12, -h + 15);

    DefaultLookupProgressReporter::showProgress(support, gr, progressArea, false);
}
// end sm_ipedia.cpp
