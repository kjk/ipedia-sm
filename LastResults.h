#include "resource.h"
#include <windows.h>

#define LR_CANCEL_PRESSED 1
#define LR_DO_LOOKUP_IF_DIFFERENT 2
#define LR_DO_SEARCH 3

BOOL CALLBACK LastResultsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
bool FInitLastResults(HWND hDlg);
LRESULT CALLBACK LastResultsWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

