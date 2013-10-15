//
//    MODULE:        TextViewFont.cpp
//
//    PURPOSE:    Font support for the TextView control
//
//    NOTES:        www.catch22.net
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include "TextView.h"
#include "TextViewInternal.h"

//
//  TextView::
//
int NeatTextYOffset_TextView(
    TextView * lps,
    USPFONT * font
    )
{
    return lps->nMaxAscent + lps->nHeightAbove - font->tm.tmAscent;
}

int TextWidth_TextView(
    TextView * lps,
    HDC hdc,
    LPCTSTR lpString,
    int c
    )
{
    SIZE stSize;
    if (c == -1)
    {
        c = lstrlen(lpString);
    }
    GetTextExtentPoint32(hdc, lpString, c, &stSize);
    return stSize.cx;
}

//
//    Update the lineheight based on current font settings
//
VOID RecalcLineHeight_TextView(
    TextView * lps
    )
{
    lps->nLineHeight = 0;
    lps->nMaxAscent = 0;

    // find the tallest font in the TextView
    for (int i = 0; i < lps->nNumFonts; i++)
    {
        // always include a font's external-leading
        int fontheight = lps->uspFontList[i].tm.tmHeight +
            lps->uspFontList[i].tm.tmExternalLeading;

        lps->nLineHeight = max(lps->nLineHeight, fontheight);
        lps->nMaxAscent = max(lps->nMaxAscent, lps->uspFontList[i].tm.tmAscent);
    }

    // add on the above+below spacings
    lps->nLineHeight += lps->nHeightAbove + lps->nHeightBelow;

    // force caret resize if we've got the focus
    if (GetFocus() == lps->hWnd)
    {
        OnKillFocus_TextView(lps, 0);
        OnSetFocus_TextView(lps, 0);
    }
}

//
//    Set a font for the TextView
//
LONG SetFont_TextView(
    TextView * lps,
    HFONT hFont,
    int idx
    )
{
    USPFONT *uspFont = &lps->uspFontList[idx];

    // need a DC to query font data
    HDC hdc = GetDC(lps->hWnd);

    // Initialize the font for USPLIB
    UspFreeFont(uspFont);
    UspInitFont(uspFont, hdc, hFont);

    ReleaseDC(lps->hWnd, hdc);

    // calculate new line metrics
    lps->nFontWidth = lps->uspFontList[0].tm.tmAveCharWidth;

    RecalcLineHeight_TextView(lps);
    UpdateMarginWidth_TextView(lps);

    ResetLineCache_TextView(lps);

    return 0;
}

//
//    Add a secondary font to the TextView
//
LONG AddFont_TextView(
    TextView * lps,
    HFONT hFont
    )
{
    int idx = lps->nNumFonts++;

    SetFont_TextView(lps, hFont, idx);
    UpdateMetrics_TextView(lps);

    return 0;
}

//
//    WM_SETFONT handler: set a new default font
//
LONG OnSetFont_TextView(
    TextView * lps,
    HFONT hFont
    )
{
    // default font is always #0
    SetFont_TextView(lps, hFont, 0);
    UpdateMetrics_TextView(lps);

    return 0;
}

//
//    Set spacing (in pixels) above and below each line - 
//  this is in addition to the external-leading of a font
//
LONG SetLineSpacing_TextView(
    TextView * lps,
    int nAbove,
    int nBelow
    )
{
    lps->nHeightAbove = nAbove;
    lps->nHeightBelow = nBelow;
    RecalcLineHeight_TextView(lps);
    return TRUE;
}
