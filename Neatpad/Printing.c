//
//    Neatpad
//
//    Printing.c
//
//    Not currently used but might be in the future
//
//    www.catch22.net
//

#define STRICT

#include <windows.h>
#include <commdlg.h>
#include "Neatpad.h"

static HANDLE g_hDevMode = NULL;
static HANDLE g_hDevNames = NULL;
extern HWND      g_hwndTextView;
static int      g_nPrinterWidth = 0;

int GetPrinterWidth(HDC hdcPrn);

HDC ShowPrintDlg(HWND hwndParent)
{
    PRINTDLGEX    pdlgx = { sizeof(pdlgx), hwndParent, g_hDevMode, g_hDevNames };
    pdlgx.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOCURRENTPAGE | PD_NOWARNING | PD_HIDEPRINTTOFILE;
    pdlgx.nStartPage = START_PAGE_GENERAL;

    if (S_OK == PrintDlgEx(&pdlgx) && pdlgx.dwResultAction != PD_RESULT_CANCEL)
    {
        g_hDevMode = pdlgx.hDevMode;
        g_hDevNames = pdlgx.hDevNames;
        g_nPrinterWidth = GetPrinterWidth(pdlgx.hDC);

        return pdlgx.hDC;
    }

    return NULL;
}

HDC GetDefaultPrinterDC()
{
    PRINTDLG pdlg = { sizeof(pdlg), 0, 0, 0 };
    pdlg.Flags = PD_RETURNDC | PD_RETURNDEFAULT;

    if (PrintDlg(&pdlg))
    {
        g_hDevMode = pdlg.hDevMode;
        g_hDevNames = pdlg.hDevNames;

        return pdlg.hDC;
    }
    else
    {
        return NULL;
    }
}

int GetPrinterWidth(HDC hdcPrn)
{
    HDC hdcScr = GetDC(0);

    int nPrinterDPI = GetDeviceCaps(hdcPrn, LOGPIXELSX);
    int nPrinterWidth = GetDeviceCaps(hdcPrn, HORZRES);
    int nScreenDPI = GetDeviceCaps(hdcScr, LOGPIXELSX);

    ReleaseDC(0, hdcScr);

    return nPrinterWidth * nScreenDPI / nPrinterDPI;
}


int GetCurrentPrinterWidth()
{
    if (g_nPrinterWidth == 0)
    {
        HDC hdcPrn = GetDefaultPrinterDC();
        g_nPrinterWidth = GetPrinterWidth(hdcPrn);

        DeleteDC(hdcPrn);
    }

    return g_nPrinterWidth;
}