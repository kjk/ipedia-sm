#include "Registration.h"
#include "BaseTypes.hpp"
#include "sm_ipedia.h"
#include <iPediaApplication.hpp>

ArsLexis::String g_newRegCode;
static HWND    g_hRegistrationDlg  = NULL;

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
#ifdef WIN32_PLATFORM_PSPC
            HWND ctrlRegButton =  GetDlgItem(hDlg, IDM_REGISTER);
            HWND ctrlLaterButton =  GetDlgItem(hDlg, ID_CANCEL);
            MoveWindow(ctrlRegButton, width/2-59, (height-fntHeight*2)/2 + 2*fntHeight+5, 59, fntHeight, TRUE);
            MoveWindow(ctrlLaterButton, width/2+3, (height-fntHeight*2)/2 + 2*fntHeight+5, 59, fntHeight, TRUE);
#endif            
            break;
        }
        
        case WM_SETTINGCHANGE:
        {
            SHACTIVATEINFO sai;
            if (SPI_SETSIPINFO == wp){
                memset(&sai, 0, sizeof(SHACTIVATEINFO));
                SHHandleWMSettingChange(g_hRegistrationDlg, -1 , 0, &sai);
            }
            break;
        }
        case WM_ACTIVATE:
        {
            SHACTIVATEINFO sai;
            if (SPI_SETSIPINFO == wp){
                memset(&sai, 0, sizeof(SHACTIVATEINFO));
                SHHandleWMActivate(g_hRegistrationDlg, wp, lp, &sai, 0);
            }
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
                    g_newRegCode.assign(text);
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
#ifdef WIN32_PLATFORM_PSPC
    shidi.dwFlags  = SHIDIF_SIZEDLG | SHIDIF_EMPTYMENU;
#else
    shidi.dwFlags  = SHIDIF_SIZEDLGFULLSCREEN;
#endif
    shidi.hDlg     = hDlg;


    if (!SHInitDialog(&shidi))
        return false;

    // Set up the menu bar
#ifdef WIN32_PLATFORM_WFSP
    SHMENUBARINFO shmbi;
    ZeroMemory(&shmbi, sizeof(shmbi));
    shmbi.cbSize      = sizeof(shmbi);
    shmbi.hwndParent  = hDlg;
    shmbi.nToolBarId  = IDR_REGISTER_MENUBAR;
    shmbi.hInstRes    = GetModuleHandle(NULL);
    
    if (!SHCreateMenuBar(&shmbi))
        return false;

    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
#endif

    HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT_REGCODE);
    SendMessage(hwndEdit, EM_SETINPUTMODE, 0, EIM_NUMBERS);

    iPediaApplication::Preferences& prefs=iPediaApplication::instance().preferences();
    SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)g_newRegCode.c_str());
    //if(!prefs.regCode.empty())
        //SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)prefs.regCode.c_str());
    //else
    g_hRegistrationDlg = hDlg;
    SHSipPreference(hwndEdit, SIP_UP);
    SetFocus(hwndEdit);
    
    return true;
}
