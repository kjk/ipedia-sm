#include "resource.h"
#include <windows.h>
#include <Text.hpp>

// those are possible values returned by ExtSearchResultsDlg
// cancel was pressed, strOut is not set
#define EXT_RESULTS_CANCEL 1

// select was pressed i.e. an existing string from a list was selected
// strOut contains that string
#define EXT_RESULTS_SELECT 2

// refine was pressed i.e. user entered some text into a text edit dialog box
// and pressed "refine". strOut contains text entered by user
#define EXT_RESULTS_REFINE 3

int ExtSearchResultsDlg(HWND hwnd, CharPtrList_t& strList, String& strOut);

