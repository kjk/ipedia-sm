#include "resource.h"
#include <windows.h>

BOOL CALLBACK HyperlinksDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
BOOL InitHyperlinks(HWND hDlg);
LRESULT CALLBACK HyperlinksListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

