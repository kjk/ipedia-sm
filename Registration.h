#include "resource.h"
#include <BaseTypes.hpp>
#include <windows.h>

BOOL CALLBACK RegistrationDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
bool InitRegistrationDlg(HWND hDlg);

extern ArsLexis::String g_newRegCode;