#include "resource.h"
#include <windows.h>

BOOL CALLBACK LastResultsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
BOOL InitLastResults(HWND hDlg);
LRESULT CALLBACK LastResultsWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

