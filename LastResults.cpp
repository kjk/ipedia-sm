#include <windows.h>
#include <iPediaApplication.hpp>
#include <LookupManager.hpp>
#include "LastResults.h"
#include "sm_ipedia.h"

using namespace ArsLexis;

static WNDPROC oldResultsListWndProc = NULL;
static WNDPROC oldResultsEditWndProc = NULL;
static HWND    g_hLastResultsDlg  = NULL;

static StrList_t g_strList;

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

    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));

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
        return FALSE;

    if (!SHCreateMenuBar(&shmbi))
        return false;

    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));

    g_fRefineMenu = false;
    return true;
}

LRESULT CALLBACK ResultsEditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (WM_SETFOCUS==msg)
    {               
        if (!SetRefineMenu(g_hLastResultsDlg))
            return FALSE;
        // return TRUE;
    }

    return CallWindowProc(oldResultsEditWndProc, hwnd, msg, wp, lp);
}

LRESULT CALLBACK ResultsListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    int currSelection;

    if (WM_SETFOCUS==msg)
    {
        if (!SetSearchMenu(g_hLastResultsDlg))
            return FALSE;
        SendMessage (hwnd, LB_SETCURSEL, 0, 0);
        return TRUE;
    }

    //What the hell is constatnt - any idea VK_F24 ??
    if (0x87==msg)
    {
        switch (wp)
        {
            case HOT_KEY_ACTION:
                currSelection = SendMessage (hwnd, LB_GETCURSEL, 0, 0);
                SendMessage(g_hLastResultsDlg, WM_COMMAND, ID_REFINE, 0);
                break;
            case HOT_KEY_UP:
                currSelection = SendMessage (hwnd, LB_GETCURSEL, 0, 0);
                if (0==currSelection)
                {
                    HWND ctrl=GetDlgItem(g_hLastResultsDlg, IDC_REFINE_EDIT);                    
                    SetFocus(ctrl);
                    SendMessage (hwnd, LB_SETCURSEL, -1, 0);
                }
                else
                {
                    SendMessage (hwnd, LB_SETCURSEL, currSelection-1, 0);
                }
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
    int fntHeight = GetSystemMetrics(SM_CYCAPTION);
    MoveWindow(ctrlRefineEdit, 2, 1, width-4, fntHeight, TRUE);
    MoveWindow(ctrlResultsList,0, fntHeight + 2, width, height - fntHeight, TRUE);
}

static void DoRefine(HWND hDlg)
{
    HWND hwndEdit = GetDlgItem(hDlg, IDC_REFINE_EDIT);
    String txt;
    GetEditWinText(hwndEdit, txt);
    if (txt.empty())
        return;

    g_recentWord += _T(" ");
    g_recentWord += txt;
    g_searchWord.assign(g_recentWord);
    EndDialog(hDlg, LR_DO_REFINE);
}
    
static void DoSelect(HWND hDlg)
{
    HWND ctrl = GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
    bool fSelected = FGetListSelectedItemText(ctrl, g_searchWord);
    if (fSelected)
        EndDialog(hDlg, LR_DO_LOOKUP_IF_DIFFERENT);
}

static void DoSelectOrRefine(HWND hDlg)
{
    if (g_fRefineMenu)
        DoRefine(hDlg);
    else
        DoSelect(hDlg);
}

BOOL CALLBACK LastResultsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            return FInitLastResults(hDlg) ? TRUE : FALSE;

        case WM_SIZE:
            OnSize(hDlg,wp,lp);
            break;

        case WM_COMMAND:
        {
            switch (wp)
            {
                case ID_CANCEL:
                    EndDialog(hDlg, LR_CANCEL_PRESSED);
                    break;

                case ID_REFINE:  // no break is intentional
                case ID_SELECT:
                    DoSelectOrRefine(hDlg);
                    break;

                case IDC_LAST_RESULTS_LIST:
                    int code = HIWORD(lp);
                    if (LBN_DBLCLK==code)
                    {
                        DoSelect(hDlg);
                    }
                    break;
            }
        }
    }
    return FALSE;
}

bool FInitLastResults(HWND hDlg)
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

    g_hLastResultsDlg = hDlg;

    HWND ctrlList = GetDlgItem(hDlg, IDC_LAST_RESULTS_LIST);
    oldResultsListWndProc = (WNDPROC)SetWindowLong(ctrlList, GWL_WNDPROC, (LONG)ResultsListWndProc);
    RegisterHotKey(ctrlList, HOT_KEY_ACTION, 0, VK_TACTION);
    RegisterHotKey(ctrlList, HOT_KEY_UP,     0, VK_TUP);

    HWND EditCrl = GetDlgItem(hDlg, IDC_REFINE_EDIT);
    oldResultsEditWndProc = (WNDPROC)SetWindowLong(EditCrl, GWL_WNDPROC, (LONG)ResultsEditWndProc);

    iPediaApplication& app = GetApp();
    LookupManager* lookupManager=app.getLookupManager();
    
    ArsLexis::String   listPositionsString;
    if (lookupManager)
    {
        listPositionsString=lookupManager->lastSearchResults();
        String::iterator end(listPositionsString.end());
        String::iterator lastStart=listPositionsString.begin();
        for (String::iterator it=listPositionsString.begin(); it!=end; ++it)
        {
            if ('\n'==*it)
            {
                char_t* start = &(*lastStart);
                lastStart = it;
                ++lastStart;
                *it = chrNull;
                SendMessage(ctrlList, LB_ADDSTRING, 0, (LPARAM)start);
            }
        }
    }
    SendMessage (ctrlList, LB_SETCURSEL, -1, 0);
    UpdateWindow(ctrlList);
    return true;
}

enum RefineDlgResult {
    REFINE_DLG_REFINE,
    REFINE_DLG_SEARCH,
    REFINE_DLG_CANCEL
};

RefineDlgResult DoRefineDlg(HWND hwnd, StrList_t strList, String& strOut)
{
    g_strList = strList;

    int res = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LAST_RESULTS), hwnd,LastResultsDlgProc);

    if (LR_DO_LOOKUP_IF_DIFFERENT==res)
    {
        // TODO: put string
        return REFINE_DLG_SEARCH;
    }

    if (LR_DO_REFINE==res)
    {
        // TODO: put string
        return REFINE_DLG_REFINE;
    }

    return REFINE_DLG_CANCEL;
}

