#include "Registration.h"
#include "BaseTypes.hpp"
#include "sm_ipedia.h"
#include <iPediaApplication.hpp>
BOOL CALLBACK RegistrationDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            return InitRegistrationDlg(hDlg);
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
                    ArsLexis::String newSn(text);
                    if (newSn!=prefs.serialNumber)
                    {
                        assert(newSn.length()<=prefs.serialNumberLength);
                        prefs.serialNumber=newSn;
                        prefs.serialNumberRegistered=false;
                    }
                    delete text;
                    EndDialog(hDlg, 0);                    
                    break;
                }
            }
        }
    }
    return FALSE;
}

BOOL InitRegistrationDlg(HWND hDlg)
{
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
    shmbi.nToolBarId = IDR_REGISTER_MENUBAR;
    shmbi.hInstRes = g_hInst;

    if (!SHInitDialog(&shidi))
        return FALSE;
    
    if (!SHCreateMenuBar(&shmbi))
        return FALSE;

    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));

    iPediaApplication::Preferences& prefs=iPediaApplication::instance().preferences();
    HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT_REGCODE);
    SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)prefs.serialNumber.c_str());

    SetFocus(GetDlgItem(hDlg,IDC_EDIT_REGCODE));
    return TRUE;
}
