#include "resource.h"
#include <BaseTypes.hpp>
#include <windows.h>

BOOL CALLBACK RegistrationDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
BOOL InitRegistrationDlg(HWND hDlg);

extern ArsLexis::String newRegCode_;