#include "LastResults.h"
#include "sm_ipedia.h"
#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include <list>
#include <windows.h>

using namespace ArsLexis;

static WNDPROC oldResultsListWndProc = NULL;
static WNDPROC oldResultsEditWndProc = NULL;
static HWND    g_hLastResultsDlg  = NULL;
static bool    g_fRefine = true;

std::list<char_t*> g_listPositions;
ArsLexis::String   g_listPositionsString;

const int hotKeyCode=0x32;

LRESULT CALLBACK ResultsEditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_SETFOCUS:
        {               
                SHMENUBARINFO shmbi;
                ZeroMemory(&shmbi, sizeof(shmbi));
                shmbi.cbSize = sizeof(shmbi);
                shmbi.hwndParent = g_hLastResultsDlg;
                shmbi.nToolBarId = IDR_LAST_RESULTS_REFINE_MENUBAR;
                shmbi.hInstRes = iPediaApplication::instance().getApplicationHandle();
                
                if (!SHCreateMenuBar(&shmbi))
                    return FALSE;
                
                (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
                    MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
                    SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
                g_fRefine = true;
                break;
        }
    }
    return CallWindowProc(oldResultsEditWndProc, hwnd, msg, wp, lp);
}

LRESULT CALLBACK ResultsListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        //What the hell is constatnt - any idea VK_F24 ??
        case WM_SETFOCUS:
        {               
            SHMENUBARINFO shmbi;
            ZeroMemory(&shmbi, sizeof(shmbi));
            shmbi.cbSize = sizeof(shmbi);
            shmbi.hwndParent = g_hLastResultsDlg;
            shmbi.nToolBarId = IDR_LAST_RESULTS_SEARCH_MENUBAR ;
            shmbi.hInstRes = iPediaApplication::instance().getApplicationHandle();
            
            if (!SHCreateMenuBar(&shmbi))
                return FALSE;
            
            (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
                MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
                SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
            g_fRefine = false;
            break;
        }
        
        case 0x87: 
        {
            switch (wp)
            {
                case hotKeyCode:
                    SendMessage(g_hLastResultsDlg, WM_COMMAND, ID_REFINE, 0);
                    break;
                case hotKeyCode+1:
                    int pos = SendMessage (hwnd, LB_GETCURSEL, 0, 0);
                    if (0==pos)
                    {
                        HWND ctrl=GetDlgItem(g_hLastResultsDlg, IDC_REFINE_EDIT);
                        SetFocus(ctrl);

                        /*SHMENUBARINFO shmbi;
                        ZeroMemory(&shmbi, sizeof(shmbi));
                        shmbi.cbSize = sizeof(shmbi);
                        shmbi.hwndParent = g_hLastResultsDlg;
                        shmbi.nToolBarId = IDR_LAST_RESULTS_REFINE_MENUBAR;
                        shmbi.hInstRes = g_hInst;
        
                        if (!SHCreateMenuBar(&shmbi))
                            return FALSE;
    
                        (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
                            MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
                            SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
                        g_fRefine = true;*/
                    }
                    else
                        SendMessage (hwnd, LB_SETCURSEL, pos-1, 0);

                    UpdateWindow(hwnd);
                    break;
            }
        }
    }
    return CallWindowProc(oldResultsListWndProc, hwnd, msg, wp, lp);
}

BOOL CALLBACK LastResultsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            if (FInitLastResults(hDlg))
                return TRUE;
            else
                return FALSE;
            assert(0);
            break;

        case WM_SIZE:
        {
            int width = LOWORD(lp);
            int height = HIWORD(lp);
            HWND ctrlRefineEdit = GetDlgItem(hDlg, IDC_REFINE_EDIT);
            HWND ctrlResultsList = GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
            int fntHeight = GetSystemMetrics(SM_CYCAPTION);
            MoveWindow(ctrlRefineEdit, 2, 1, width-4, fntHeight, TRUE);
            MoveWindow(ctrlResultsList,0, fntHeight + 2, width, height - fntHeight, TRUE);
            break;
        }

        case WM_COMMAND:
        {
            switch (wp)
            {
                case ID_CANCEL:
                    EndDialog(hDlg, 0);
                    break;

                case ID_REFINE:
                case ID_SELECT:
                {
                    if (!g_fRefine)
                    {
                        HWND ctrl = GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
                        int idx = SendMessage(ctrl, LB_GETCURSEL, 0, 0);
                        int len = SendMessage(ctrl, LB_GETTEXTLEN, idx, 0);
                        TCHAR *buf = new TCHAR[len+1];
                        SendMessage(ctrl, LB_GETTEXT, idx, (LPARAM) buf);
                        g_searchWord.assign(buf);
                        delete buf;
                        EndDialog(hDlg, 1);
                        break;
                    }
                    else                
                    {
                        HWND ctrl = GetDlgItem(hDlg, IDC_REFINE_EDIT);
                        int len = SendMessage(ctrl, EM_LINELENGTH, 0,0);
                        if (len>0)
                        {
                            TCHAR *buf=new TCHAR[len+1];
                            len = SendMessage(ctrl, WM_GETTEXT, len+1, (LPARAM)buf);
                            SendMessage(ctrl, EM_SETSEL, 0,len);
                            g_recentWord += _T(" ");
                            g_recentWord += buf;
                            g_searchWord.assign(g_recentWord);
                            delete buf;
                            EndDialog(hDlg, 2);
                            break;
                        }
                    }
                }
            }
        }
        HWND ctrlList = GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
        int senderID = LOWORD(wp);
        int code = HIWORD(wp);
        HWND senderHWND = (HWND)lp;
        if((IDC_LAST_RESULTS_LIST == senderID) && (senderHWND == ctrlList))
            if (LBN_DBLCLK == code)
                SendMessage(g_hLastResultsDlg , WM_COMMAND, ID_SELECT, 0); 
    }
    return FALSE;
}

bool FInitLastResults(HWND hDlg)
{
    // Specify that the dialog box should stretch full screen
    SHINITDLGINFO shidi;
    ZeroMemory(&shidi, sizeof(shidi));
    shidi.dwMask  = SHIDIM_FLAGS;
    shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
    shidi.hDlg    = hDlg;

    // Set up the menu bar
    SHMENUBARINFO shmbi;
    ZeroMemory(&shmbi, sizeof(shmbi));
    shmbi.cbSize     = sizeof(shmbi);
    shmbi.hwndParent = hDlg;
    shmbi.nToolBarId = IDR_LAST_RESULTS_REFINE_MENUBAR;
    shmbi.hInstRes   = iPediaApplication::instance().getApplicationHandle();

    // If we could not initialize the dialog box, return an error
    if (!SHInitDialog(&shidi))
        return false;

    if (!SHCreateMenuBar(&shmbi))
        return false;

    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));

    g_hLastResultsDlg = hDlg;
    g_fRefine = true;

    HWND ctrl=GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
    oldResultsListWndProc = (WNDPROC)SetWindowLong(ctrl, GWL_WNDPROC, (LONG)ResultsListWndProc);
    RegisterHotKey( ctrl, hotKeyCode,   0, VK_TACTION);
    RegisterHotKey( ctrl, hotKeyCode+1, 0, VK_TUP);

    HWND EditCrl=GetDlgItem(hDlg, IDC_REFINE_EDIT);
    oldResultsEditWndProc = (WNDPROC)SetWindowLong(EditCrl, GWL_WNDPROC, (LONG)ResultsEditWndProc);

    g_listPositions.clear();
    
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager();
    if (lookupManager)
    {
        g_listPositionsString=lookupManager->lastSearchResults();
        g_listPositions.clear();
        String::iterator end(g_listPositionsString.end());
        String::iterator lastStart=g_listPositionsString.begin();
        for (String::iterator it=g_listPositionsString.begin(); it!=end; ++it)
        {
            if ('\n'==*it)
            {
                char_t* start = &(*lastStart);
                lastStart = it;
                ++lastStart;
                *it = chrNull;
                g_listPositions.push_back(start);
                SendMessage(ctrl, LB_ADDSTRING, 0, (LPARAM)start);
            }
        }
        //if (!lookupManager->lastSearchExpression().empty())
        //    setTitle(lookupManager->lastSearchExpression());
    }
    SendMessage (ctrl, LB_SETCURSEL, 0, 0);
    UpdateWindow(ctrl);
    return true;
}
