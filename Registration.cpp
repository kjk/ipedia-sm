#include "Registration.h"
#include "BaseTypes.hpp"
#include "sm_ipedia.h"
#include <iPediaApplication.hpp>

ArsLexis::String newRegCode_;

BOOL CALLBACK RegistrationDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            return InitRegistrationDlg(hDlg);

        case WM_SIZE:
        {
            int width = LOWORD(lp);
            int height = HIWORD(lp);
            HWND ctrlRegCodeText = GetDlgItem(hDlg, IDC_STATIC_REG_CODE);
            HWND ctrlRegCodeEdit = GetDlgItem(hDlg, IDC_EDIT_REGCODE);
            int fntHeight = GetSystemMetrics(SM_CYCAPTION);
            MoveWindow(ctrlRegCodeText, 0, (height-fntHeight*2)/2, width, fntHeight, TRUE);
            MoveWindow(ctrlRegCodeEdit, 4, (height-fntHeight*2)/2 + fntHeight, width-8, fntHeight, TRUE);
            break;
        }
        case WM_COMMAND:
        {
            switch (wp)
            {
                case ID_CANCEL:
                    EndDialog(hDlg, 0);
                    break;
                case IDM_REGISTER:
                {
                    HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT_REGCODE);
                    int len = SendMessage(hwndEdit, EM_LINELENGTH, 0,0);
                    TCHAR *text=new TCHAR[len+1];
                    len = SendMessage(hwndEdit, WM_GETTEXT, len+1, (LPARAM)text);
                    iPediaApplication::Preferences& prefs=iPediaApplication::instance().preferences();
                    newRegCode_.assign(text);
                    delete text;
                    EndDialog(hDlg, 1);                    
                    break;
                }
            }
        }
    }
    return FALSE;
}

bool InitRegistrationDlg(HWND hDlg)
{
    SHINITDLGINFO shidi;
    ZeroMemory(&shidi, sizeof(shidi));
    shidi.dwMask   = SHIDIM_FLAGS;
    shidi.dwFlags  = SHIDIF_SIZEDLGFULLSCREEN;
    shidi.hDlg     = hDlg;

    // Set up the menu bar
    SHMENUBARINFO shmbi;
    ZeroMemory(&shmbi, sizeof(shmbi));
    shmbi.cbSize      = sizeof(shmbi);
    shmbi.hwndParent  = hDlg;
    shmbi.nToolBarId  = IDR_REGISTER_MENUBAR;
    shmbi.hInstRes    = g_hInst;

    if (!SHInitDialog(&shidi))
        return false;
    
    if (!SHCreateMenuBar(&shmbi))
        return false;

    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));

    HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT_REGCODE);
    SendMessage(hwndEdit, EM_SETINPUTMODE, 0, EIM_NUMBERS);

    iPediaApplication::Preferences& prefs=iPediaApplication::instance().preferences();
    SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)newRegCode_.c_str());
    //if(!prefs.regCode.empty())
        //SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)prefs.regCode.c_str());
    //else
    SetFocus(hwndEdit);

    return true;
}
