#include <windows.h>
#ifdef WIN32_PLATFORM_WFSP
#include <tpcshell.h>
#endif
#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include "ExtSearchResultsDlg.hpp"
#include "sm_ipedia.h"

#include <UIHelper.h>

using namespace ArsLexis;

static WNDPROC oldResultsListWndProc = NULL;
static WNDPROC oldResultsEditWndProc = NULL;

// TODO: should be able to eliminate g_hDlg. Just need to know the API
// for finding parent dialog based on hwnd of a control
static HWND    g_hDlg  = NULL;
static HWND    g_hListWnd = NULL;

static CharPtrList_t*  g_strList=NULL;
static String          g_editBoxTxt;
static String          g_listCtrlSelectedTxt;

#define HOT_KEY_ACTION 0x32
#define HOT_KEY_UP     0x33

// switching menus using SHCreateMenuBar() doesn't seem to correctly
// change the IDs that are sent by menu presses so we can't tell
// from messages if ID_REFINE or ID_SELECT really means those were pressed.
// So we have to track which menu is active and make a decision based on that
static bool g_fRefineMenu;

static bool SetRefineMenu(HWND hDlg)
{
    SHMENUBARINFO shmbi = {0};
    shmbi.cbSize     = sizeof(shmbi);
    shmbi.hwndParent = hDlg;
    shmbi.nToolBarId = IDR_LAST_RESULTS_REFINE_MENUBAR;
    shmbi.hInstRes   = GetModuleHandle(NULL);

    if (!SHCreateMenuBar(&shmbi))
        return false;

    // TODO: do I have to repeat that?
    OverrideBackButton(shmbi.hwndMB);

    g_fRefineMenu = true;
    return true;
}

static bool SetSearchMenu(HWND hDlg)
{
    SHMENUBARINFO shmbi = {0};
    shmbi.cbSize = sizeof(shmbi);
    shmbi.hwndParent = hDlg;
    shmbi.nToolBarId = IDR_LAST_RESULTS_SEARCH_MENUBAR ;
    shmbi.hInstRes   = GetModuleHandle(NULL);

    if (!SHCreateMenuBar(&shmbi))
        return false;

    if (!SHCreateMenuBar(&shmbi))
        return false;

    // TODO: do I have to repeat that?
    OverrideBackButton(shmbi.hwndMB);

    g_fRefineMenu = false;
    return true;
}

LRESULT CALLBACK ResultsEditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (WM_SETFOCUS==msg)
    {               
        if (!SetRefineMenu(g_hDlg))
            return FALSE;
    }
    else if (WM_KEYUP==msg)
    {
        int key = wp;
        if (VK_DOWN==key)
        {
            SendMessage(g_hListWnd, LB_SETCURSEL, 0, 0);
            SetFocus(g_hListWnd);
        }
    }
    return CallWindowProc(oldResultsEditWndProc, hwnd, msg, wp, lp);
}

LRESULT CALLBACK ResultsListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    int currSelection;

    if (WM_SETFOCUS==msg)
    {
        if (!SetSearchMenu(g_hDlg))
            return FALSE;
        SendMessage(hwnd, LB_SETCURSEL, 0, 0);
    }
    // What the hell is constant - any idea VK_F24 ??
    else if (0x87==msg)
    {
        switch (wp)
        {
            case HOT_KEY_ACTION:
                currSelection = SendMessage (hwnd, LB_GETCURSEL, 0, 0);
                SendMessage(g_hDlg, WM_COMMAND, ID_REFINE, 0);
                break;
            case HOT_KEY_UP:
                currSelection = SendMessage (hwnd, LB_GETCURSEL, 0, 0);
                if (0==currSelection)
                {
                    HWND ctrl=GetDlgItem(g_hDlg, IDC_REFINE_EDIT);                    
                    SetFocus(ctrl);
                    SendMessage (hwnd, LB_SETCURSEL, -1, 0);
                }
                else
                {
                    SendMessage (hwnd, LB_SETCURSEL, currSelection-1, 0);
                }
                break;
        }
    }
    return CallWindowProc(oldResultsListWndProc, hwnd, msg, wp, lp);
}

static void OnSize(HWND hDlg, WPARAM wp, LPARAM lp)
{
    int width = LOWORD(lp);
    int height = HIWORD(lp);
    HWND ctrlRefineEdit = GetDlgItem(hDlg, IDC_REFINE_EDIT);
    HWND ctrlResultsList = GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
	int fntHeight = GetSystemMetrics(SM_CYCAPTION) - SCALEY(2);
    MoveWindow(ctrlRefineEdit, SCALEX(2), SCALEY(2), width - SCALEX(4), fntHeight, TRUE);
    MoveWindow(ctrlResultsList, SCALEX(2), fntHeight + SCALEY(4), width - SCALEY(4), height - fntHeight - SCALEY(4), TRUE);
}

static void DoRefine(HWND hDlg)
{
    HWND hwndEdit = GetDlgItem(hDlg, IDC_REFINE_EDIT);
    GetEditWinText(hwndEdit, g_editBoxTxt);
    if (g_editBoxTxt.empty())
        return;
    
    EndDialog(hDlg, EXT_RESULTS_REFINE);
}
    
static void DoSelect(HWND hDlg)
{
    HWND ctrl = GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
    bool fSelected = ListCtrlGetSelectedItemText(ctrl, g_listCtrlSelectedTxt);
    if (fSelected)
        EndDialog(hDlg, EXT_RESULTS_SELECT);
}

static void DoSelectOrRefine(HWND hDlg)
{
    if (g_fRefineMenu)
        DoRefine(hDlg);
    else
        DoSelect(hDlg);
}

static bool FInitDlg(HWND hDlg)
{
    // Specify that the dialog box should stretch full screen
    SHINITDLGINFO shidi = {0};
    shidi.dwMask  = SHIDIM_FLAGS;
    shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
    shidi.hDlg    = hDlg;

    // If we could not initialize the dialog box, return an error
    if (!SHInitDialog(&shidi))
        return false;

    if (!SetRefineMenu(hDlg))
        return false;

    g_hDlg = hDlg;

    HWND EditCrl = GetDlgItem(hDlg, IDC_REFINE_EDIT);
    oldResultsEditWndProc = (WNDPROC)SetWindowLong(EditCrl, GWL_WNDPROC, (LONG)ResultsEditWndProc);

    g_hListWnd = GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
    oldResultsListWndProc = (WNDPROC)SetWindowLong(g_hListWnd, GWL_WNDPROC, (LONG)ResultsListWndProc);
    RegisterHotKey(g_hListWnd, HOT_KEY_ACTION, 0, VK_TACTION);
    RegisterHotKey(g_hListWnd, HOT_KEY_UP,     0, VK_TUP);
    ListCtrlFillFromList(g_hListWnd, *g_strList, false);
    SendMessage (g_hListWnd, LB_SETCURSEL, -1, 0);
    UpdateWindow(g_hListWnd);

    return true;
}

static BOOL CALLBACK ExtSearchResultsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            return FInitDlg(hDlg) ? TRUE : FALSE;

        case WM_SIZE:
            OnSize(hDlg,wp,lp);
            break;

 #ifdef WIN32_PLATFORM_WFSP
        case WM_HOTKEY:
            SHSendBackToFocusWindow(msg, wp, lp);
            return TRUE;
#endif
        case WM_COMMAND:
        {
            uint_t control = LOWORD(wp);
            uint_t code = HIWORD(wp);

            switch (control)
            {
                case ID_CANCEL:
                    EndDialog(hDlg, EXT_RESULTS_CANCEL);
                    break;

                case ID_REFINE:  // no break is intentional
                case ID_SELECT:
                    DoSelectOrRefine(hDlg);
                    break;

                case IDC_LAST_RESULTS_LIST:
                    if (LBN_DBLCLK==code)
                        DoSelect(hDlg);
                    break;
            }
        }
    }
    return FALSE;
}

int ExtSearchResultsDlg(HWND hwnd, CharPtrList_t& strList, String& strOut)
{
    g_strList = &strList;

    int res = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LAST_RESULTS), hwnd,ExtSearchResultsDlgProc);

    if (EXT_RESULTS_SELECT==res)
    {
        strOut.assign(g_listCtrlSelectedTxt);
        return res;
    }

    if (EXT_RESULTS_REFINE==res)
    {
        strOut.assign(g_editBoxTxt);
        return res;
    }

    return EXT_RESULTS_CANCEL;
}

