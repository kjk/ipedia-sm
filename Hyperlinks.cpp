#include "Hyperlinks.h"
#include "sm_ipedia.h"
#include <Definition.hpp>
#include <DefinitionElement.hpp>
#include <GenericTextElement.hpp>
#include <windows.h>

using namespace ArsLexis;

WNDPROC oldHyperlinksListWndProc;
HWND hHyperlinksDlg=NULL;

const int hotKeyCode=0x32;

LRESULT CALLBACK HyperlinksListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        //What the hell is constatnt - any idea VK_F24 ??
        case 0x87: 
        {
            switch(wp)
            {
                case hotKeyCode:
                    SendMessage(hHyperlinksDlg, WM_COMMAND, ID_SELECT, 0);
                    break;
            }
        }
    }
    return CallWindowProc(oldHyperlinksListWndProc, hwnd, msg, wp, lp);
}

BOOL CALLBACK HyperlinksDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            return InitHyperlinks(hDlg);
        case WM_COMMAND:
        {
            switch (wp)
            {
                case ID_CANCEL:
                    EndDialog(hDlg, 0);
                    break;
                case ID_SELECT:
                {
                    HWND ctrl = GetDlgItem(hDlg, IDC_LIST_HYPERLINKS);
                    int idx = SendMessage(ctrl, LB_GETCURSEL, 0, 0);
                    int len = SendMessage(ctrl, LB_GETTEXTLEN, idx, 0);
                    TCHAR *buf = new TCHAR[len+1];
                    SendMessage(ctrl, LB_GETTEXT, idx, (LPARAM) buf);
                    searchWord.assign(buf);
                    recentWord.assign(buf);
                    delete buf;
                    EndDialog(hDlg, 1);
                    break;
                }
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
    shmbi.hInstRes = g_hInst;
    
    // If we could not initialize the dialog box, return an error
    if (!SHInitDialog(&shidi))
        return FALSE;
    
    if (!SHCreateMenuBar(&shmbi))
        return FALSE;
    
    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
    
    hHyperlinksDlg=hDlg;

    HWND ctrl=GetDlgItem(hDlg, IDC_LIST_HYPERLINKS);
    oldHyperlinksListWndProc=(WNDPROC)SetWindowLong(ctrl, GWL_WNDPROC, (LONG)HyperlinksListWndProc);
    RegisterHotKey( ctrl, hotKeyCode, 0, VK_TACTION);

    
    Definition::ElementPosition_t pos;
    for(pos=definition_->firstElementPosition();
        pos!=definition_->lastElementPosition();
        pos++)
    {
        DefinitionElement *curr=*pos;
        if(curr->isTextElement())
        {
            GenericTextElement *txtEl=(GenericTextElement*)curr;
            if((txtEl->isHyperlink())&&
                (txtEl->hyperlinkProperties()->type==hyperlinkTerm))
                SendMessage(
                ctrl,
                LB_ADDSTRING,
                0,
                (LPARAM)txtEl->text().c_str());

        }
        
    }
    SendMessage (ctrl, LB_SETCURSEL, 0, 0);
    UpdateWindow(ctrl);
    return TRUE;
}
