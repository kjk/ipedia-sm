// sm_ipedia.cpp : Defines the entry point for the application.
//

#include "sm_ipedia.h"
#include <SysUtils.hpp>
#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include <LookupManagerBase.hpp>

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

iPediaApplication iPediaApplication::instance_;
LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
WNDPROC oldEditWndProc;

Definition *definition_ = new Definition();
RenderingPreferences* prefs= new RenderingPreferences();
static bool g_forceLayoutRecalculation=false;
bool rec=false;
void setScrollBar(Definition* definition_);
ArsLexis::String articleCountText;

HINSTANCE g_hInst = NULL;  // Local copy of hInstance
HWND hwndMain = NULL;    // Handle to Main window returned from CreateWindow


void paint(HWND hwnd, HDC hdc);


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
	PAINTSTRUCT	ps;

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
            if (lookupManager && !lookupManager->lookupInProgress())
                lookupManager->checkArticleCount();
            
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
                    ArsLexis::String word(buf); 
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
            default:
                return DefWindowProc(hwnd, msg, wp, lp);
            }
            break;
		case WM_PAINT:
		{
			hdc = BeginPaint (hwnd, &ps);
			/*GetClientRect (hwnd, &rect);
            DrawText (hdc, TEXT("Enter article name "),-1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
            rect.top+=38;
            DrawText (hdc, TEXT("and press \"Search\" button."),-1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);*/
            paint(hwnd, hdc);
			EndPaint (hwnd, &ps);
    		break;
		}		
        
        case WM_HOTKEY:
        {
            ArsLexis::Graphics gr(GetDC(hwndMain), hwndMain);
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
        
        case LookupManager::lookupProgressEvent:
        {
            //Still progress displaying doesn't work
            //InvalidateRect(hwnd,NULL,TRUE);
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
                case LookupFinishedEventData::outcomeDefinition:
                {   
                    assert(lookupManager!=0);
                    if (lookupManager)
                    {
                        definition_->replaceElements(lookupManager->lastDefinitionElements());
                        g_forceLayoutRecalculation=true;
                    }
                    InvalidateRect(hwndMain, NULL, TRUE);
                    break;
                }
            }            
            assert(0!=articleCountElement_);
            articleCountText.assign(_T("Number of articles: "));
            ArsLexis::char_t buffer[16];
            int len= tprintf(buffer, _T("%ld"), app.preferences().articleCount);
            articleCountText.append(buffer, len);
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
    
    iPediaApplication::instance().waitForEvent();
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

void paint(HWND hwnd, HDC hdc)
{
    RECT rect;
    GetClientRect (hwnd, &rect);
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
    rect.top    +=22;
    rect.left   +=2;
    rect.right  -=7;
    rect.bottom -=2;
    ArsLexis::Graphics gr(hdc, hwnd);
    //iPediaApplication& app=iPediaApplication::instance();
    //LookupManager* lookupManager=app.getLookupManager(true);
    //Definition::Elements_t defels_=lookupManager->lastDefinitionElements();
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
                    ArsLexis::Graphics offscreen(offscreenDc, NULL);
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
    RECT a;
    GetClientRect(hwnd, &a);

    ArsLexis::Rectangle progressArea(a);
    const iPediaApplication& app=iPediaApplication::instance();
    const LookupManager* lookupManager=app.getLookupManager();
    if (lookupManager && lookupManager->lookupInProgress())
    {
        gr.setColor(ArsLexis::Graphics::ColorChoice::colorText,RGB(0,0,0));
        lookupManager->showProgress(gr, progressArea);
    }
    if(rec)
    {
        setScrollBar(definition_);
        rec=false;
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
                    ArsLexis::Graphics gr(GetDC(hwndMain), hwndMain);
                    bool doubleBuffer=true;
                    
                    HDC offscreenDc=::CreateCompatibleDC(gr.handle());
                    if (offscreenDc) {
                        HBITMAP bitmap=::CreateCompatibleBitmap(gr.handle(), bounds.width(), bounds.height());
                        if (bitmap) {
                            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
                            {
                                ArsLexis::Graphics offscreen(offscreenDc, NULL);
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


// end sm_ipedia.cpp
