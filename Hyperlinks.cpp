#include "Hyperlinks.h"
#include "iPediaApplication.hpp"
#include "sm_ipedia.h"
#include <Definition.hpp>
#include <DefinitionElement.hpp>
#include <GenericTextElement.hpp>
#include <windows.h>

using namespace ArsLexis;

WNDPROC g_oldHyperlinksListWndProc = NULL;
HWND    g_hHyperlinksDlg = NULL;

const int hotKeyCode = 0x32;

LRESULT CALLBACK HyperlinksListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        //What the hell is constatnt - any idea VK_F24 ??
        case 0x87: 
        {
            switch (wp)
            {
                case hotKeyCode:
                    SendMessage(g_hHyperlinksDlg, WM_COMMAND, ID_SELECT, 0);
                    break;
            }
        }
    }
    return CallWindowProc(g_oldHyperlinksListWndProc, hwnd, msg, wp, lp);
}

static void OnSelect(HWND hDlg)
{
    HWND ctrl = GetDlgItem(hDlg, IDC_LIST_HYPERLINKS);
    int  idx = SendMessage(ctrl, LB_GETCURSEL, 0, 0);

    GenericTextElement *txtEl=(GenericTextElement*)SendMessage(ctrl, LB_GETITEMDATA, idx, 0);
    if((txtEl->hyperlinkProperties()->type==hyperlinkTerm)||
       (txtEl->hyperlinkProperties()->type==hyperlinkBookmark))
    {
        int len = SendMessage(ctrl, LB_GETTEXTLEN, idx, 0);
        TCHAR *buf = new TCHAR[len+1];
        SendMessage(ctrl, LB_GETTEXT, idx, (LPARAM) buf);
        g_searchWord.assign(buf);
        g_recentWord.assign(buf);
        delete [] buf;
    }
    txtEl->performAction(currentDefinition());
    EndDialog(hDlg, 1);
}

BOOL CALLBACK HyperlinksDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            return InitHyperlinks(hDlg);
        case WM_SIZE:
        {
            HWND ctrlList = GetDlgItem(hDlg, IDC_LIST_HYPERLINKS);
            MoveWindow(ctrlList, 0, 2, LOWORD(lp), HIWORD(lp), TRUE);
            break;
        }
        case WM_COMMAND:
        {
            switch (wp)
            {
                case ID_CANCEL:
                    EndDialog(hDlg, 0);
                    break;
                case ID_SELECT:
                    OnSelect(hDlg);
                    break;
            }
            // TODO: when is this code executed?
            HWND ctrlList = GetDlgItem(hDlg, IDC_LIST_HYPERLINKS);
            int  senderID = LOWORD(wp);
            int  code = HIWORD(wp);
            HWND senderHWND = (HWND)lp;
            if ((IDC_LIST_HYPERLINKS == senderID) && (senderHWND == ctrlList))
            {
                if (LBN_DBLCLK == code)
                    SendMessage(g_hHyperlinksDlg, WM_COMMAND, ID_SELECT, 0);
            }
        }
    }
    return FALSE;
}

BOOL InitHyperlinks(HWND hDlg)
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
    shmbi.nToolBarId = IDR_HYPERLINKS_MENUBAR ;
    shmbi.hInstRes = iPediaApplication::instance().getApplicationHandle();
    
    // If we could not initialize the dialog box, return an error
    if (!SHInitDialog(&shidi))
        return FALSE;
    
    if (!SHCreateMenuBar(&shmbi))
        return FALSE;
    
    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
    
    g_hHyperlinksDlg = hDlg;

    HWND ctrl = GetDlgItem(hDlg, IDC_LIST_HYPERLINKS);
    g_oldHyperlinksListWndProc = (WNDPROC)SetWindowLong(ctrl, GWL_WNDPROC, (LONG)HyperlinksListWndProc);
    RegisterHotKey(ctrl, hotKeyCode, 0, VK_TACTION);

    
    Definition::ElementPosition_t pos;
    Definition &def = currentDefinition();
    for(pos=def.firstElementPosition();
        pos!=def.lastElementPosition();
        pos++)
    {
        DefinitionElement *curr=*pos;
        if(curr->isTextElement())
        {
            GenericTextElement *txtEl=(GenericTextElement*)curr;
            if((txtEl->isHyperlink())&&
                ((txtEl->hyperlinkProperties()->type==hyperlinkTerm)||
                 (txtEl->hyperlinkProperties()->type==hyperlinkExternal)))
            {
                int idx = SendMessage(ctrl,LB_FINDSTRING,0,(LPARAM)txtEl->text().c_str());
                if (LB_ERR == idx)
                {
                    idx = SendMessage(ctrl,LB_ADDSTRING,0,(LPARAM)txtEl->text().c_str());
                    SendMessage(ctrl, LB_SETITEMDATA, idx, (LPARAM) txtEl);
                }
            }
        }
    }
    SendMessage (ctrl, LB_SETCURSEL, 0, 0);
    UpdateWindow(ctrl);
    return TRUE;
}
