#include "LastResults.h"
#include "sm_ipedia.h"
#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include <list>
#include <windows.h>

using namespace ArsLexis;

WNDPROC oldResultsListWndProc;
HWND hLastResultsDlg=NULL;
HWND RefineEditBox = NULL;

std::list<char_t*> listPositions_;
ArsLexis::String listPositionsString_;

const int hotKeyCode=0x32;
bool refine = true;
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
                shmbi.hwndParent = hLastResultsDlg;
                shmbi.nToolBarId = IDR_LAST_RESULTS_SEARCH_MENUBAR;
                shmbi.hInstRes = g_hInst;
                
                if (!SHCreateMenuBar(&shmbi))
                    return FALSE;
    
                (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
                    MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
                    SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
                refine = false;
            break;
        }
        case 0x87: 
        {
            switch(wp)
            {
                case hotKeyCode:
                    SendMessage(hLastResultsDlg, WM_COMMAND, ID_REFINE, 0);
                    break;
                case hotKeyCode +1:
                    int pos = SendMessage (hwnd, LB_GETCURSEL, 0, 0);
                    if(pos==0)
                    {
                        HWND ctrl=GetDlgItem(hLastResultsDlg, IDC_REFINE_EDIT);
                        SetFocus(ctrl);

                        SHMENUBARINFO shmbi;
                        ZeroMemory(&shmbi, sizeof(shmbi));
                        shmbi.cbSize = sizeof(shmbi);
                        shmbi.hwndParent = hLastResultsDlg;
                        shmbi.nToolBarId = IDR_LAST_RESULTS_REFINE_MENUBAR;
                        shmbi.hInstRes = g_hInst;
        
                        if (!SHCreateMenuBar(&shmbi))
                            return FALSE;
    
                        (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
                            MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
                            SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
                        refine = true;
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
            return InitLastResults(hDlg);
        case WM_COMMAND:
        {
            switch (wp)
            {
                case ID_CANCEL:
                    EndDialog(hDlg, 0);
                    break;

                case ID_REFINE:
                {
                    if(!refine)
                    {
                        HWND ctrl = GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
                        int idx = SendMessage(ctrl, LB_GETCURSEL, 0, 0);
                        int len = SendMessage(ctrl, LB_GETTEXTLEN, idx, 0);
                        TCHAR *buf = new TCHAR[len+1];
                        SendMessage(ctrl, LB_GETTEXT, idx, (LPARAM) buf);
                        searchWord.assign(buf);
                        delete buf;
                        EndDialog(hDlg, 1);
                        break;
                    }
                    else                
                    {
                        HWND ctrl = GetDlgItem(hDlg, IDC_REFINE_EDIT);
                        int len = SendMessage(ctrl, EM_LINELENGTH, 0,0);
                        if(len)
                        {
                            TCHAR *buf=new TCHAR[len+1];
                            len = SendMessage(ctrl, WM_GETTEXT, len+1, (LPARAM)buf);
                            SendMessage(ctrl, EM_SETSEL, 0,len);
                            recentWord+=_T(" ");
                            recentWord+=buf;
                            searchWord.assign(recentWord);
                            delete buf;
                            EndDialog(hDlg, 2);
                            break;
                        }
                    }
                }
            }
        }
    }
    return FALSE;
}

BOOL InitLastResults(HWND hDlg)
{
    // Specify that the dialog box should stretch full screen
    SHINITDLGINFO shidi;
    ZeroMemory(&shidi, sizeof(shidi));
    shidi.dwMask = SHIDIM_FLAGS;
    shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
    shidi.hDlg = hDlg;
    
    // Set up the menu bar
    SHMENUBARINFO shmbi;
    ZeroMemory(&shmbi, sizeof(shmbi));
    shmbi.cbSize = sizeof(shmbi);
    shmbi.hwndParent = hDlg;
    shmbi.nToolBarId = IDR_LAST_RESULTS_REFINE_MENUBAR;
    shmbi.hInstRes = g_hInst;
    
    // If we could not initialize the dialog box, return an error
    if (!SHInitDialog(&shidi))
        return FALSE;
    
    if (!SHCreateMenuBar(&shmbi))
        return FALSE;
    
    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
    
    hLastResultsDlg=hDlg;
    refine = true;

    HWND ctrl=GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
    oldResultsListWndProc=(WNDPROC)SetWindowLong(ctrl, GWL_WNDPROC, (LONG)ResultsListWndProc);
    RegisterHotKey( ctrl, hotKeyCode, 0, VK_TACTION);
    RegisterHotKey( ctrl, hotKeyCode +1, 0 , VK_TUP);
        
    listPositions_.clear();
    
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager();
    if (lookupManager)
    {
        listPositionsString_=lookupManager->lastSearchResults();
        listPositions_.clear();
        String::iterator end(listPositionsString_.end());
        String::iterator lastStart=listPositionsString_.begin();
        for (String::iterator it=listPositionsString_.begin(); it!=end; ++it)
        {
            if ('\n'==*it)
            {
                char_t* start=&(*lastStart);
                lastStart=it;
                ++lastStart;
                *it=chrNull;
                listPositions_.push_back(start);
                SendMessage(
                    ctrl,
                    LB_ADDSTRING,
                    0,
                    (LPARAM)start);            
            }
        }
        //if (!lookupManager->lastSearchExpression().empty())
        //    setTitle(lookupManager->lastSearchExpression());
    }
    SendMessage (ctrl, LB_SETCURSEL, 0, 0);
    UpdateWindow(ctrl);
    return TRUE;
}
