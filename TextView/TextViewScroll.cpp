//
//    MODULE:        TextView.cpp
//
//    PURPOSE:    Implementation of the TextView control
//
//    NOTES:        www.catch22.net
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include "TextView.h"
#include "TextViewInternal.h"

bool IsKeyPressed(UINT nVirtKey);

//
//    Set scrollbar positions and range
//
VOID SetupScrollbars_TextView(
    TextView * lps
    )
{
    SCROLLINFO si = { sizeof(si) };

    si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;

    //
    //    Vertical scrollbar
    //
    si.nPos = lps->nVScrollPos;        // scrollbar thumb position
    si.nPage = lps->nWindowLines;        // number of lines in a page
    si.nMin = 0;
    si.nMax = lps->nLineCount - 1;    // total number of lines in file

    SetScrollInfo(lps->hWnd, SB_VERT, &si, TRUE);

    //
    //    Horizontal scrollbar
    //
    si.nPos = lps->nHScrollPos;        // scrollbar thumb position
    si.nPage = lps->nWindowColumns;    // number of lines in a page
    si.nMin = 0;
    si.nMax = lps->nLongestLine - 1;    // total number of lines in file

    SetScrollInfo(lps->hWnd, SB_HORZ, &si, TRUE);

    // adjust our interpretation of the max scrollbar range to make
    // range-checking easier. The scrollbars don't use these values, they
    // are for our own use.
    lps->nVScrollMax = lps->nLineCount - lps->nWindowLines;
    lps->nHScrollMax = lps->nLongestLine - lps->nWindowColumns;
}

//
//    Ensure that we never scroll off the end of the file
//
bool PinToBottomCorner_TextView(
    TextView * lps
    )
{
    bool repos = false;

    if (lps->nHScrollPos + lps->nWindowColumns > lps->nLongestLine)
    {
        lps->nHScrollPos = lps->nLongestLine - lps->nWindowColumns;
        repos = true;
    }

    if (lps->nVScrollPos + lps->nWindowLines > lps->nLineCount)
    {
        lps->nVScrollPos = lps->nLineCount - lps->nWindowLines;
        repos = true;
    }

    return repos;
}

//
//    The window has changed size - update the scrollbars
//
LONG OnSize_TextView(
    TextView * lps,
    UINT nFlags,
    int width,
    int height
    )
{
    int margin = LeftMarginWidth_TextView(lps);

    lps->nWindowLines = min((unsigned) height / lps->nLineHeight, lps->nLineCount);
    lps->nWindowColumns = min((width - margin) / lps->nFontWidth, lps->nLongestLine);

    if (PinToBottomCorner_TextView(lps))
    {
        RefreshWindow_TextView(lps);
        RepositionCaret_TextView(lps);
    }

    SetupScrollbars_TextView(lps);

    return 0;
}

//
//    ScrollRgn
//
//    Scrolls the viewport in specified direction. If fReturnUpdateRgn is true, 
//    then a HRGN is returned which holds the client-region that must be redrawn 
//    manually. This region must be deleted by the caller using DeleteObject.
//
//  Otherwise ScrollRgn returns NULL and updates the entire window 
//
HRGN ScrollRgn_TextView(
    TextView * lps,
    int dx,
    int dy,
    bool fReturnUpdateRgn
    )
{
    RECT clip;

    GetClientRect(lps->hWnd, &clip);

    //
    // make sure that dx,dy don't scroll us past the edge of the document!
    //

    // scroll up
    if (dy < 0)
    {
        dy = -(int) min((ULONG) -dy, lps->nVScrollPos);
        clip.top = -dy * lps->nLineHeight;
    }
    // scroll down
    else if (dy > 0)
    {
        dy = min((ULONG) dy, lps->nVScrollMax - lps->nVScrollPos);
        clip.bottom = (lps->nWindowLines - dy) * lps->nLineHeight;
    }


    // scroll left
    if (dx < 0)
    {
        dx = -(int) min(-dx, lps->nHScrollPos);
        clip.left = -dx * lps->nFontWidth * 4;
    }
    // scroll right
    else if (dx > 0)
    {
        dx = min((unsigned) dx, (unsigned) lps->nHScrollMax - lps->nHScrollPos);
        clip.right = (lps->nWindowColumns - dx - 4) * lps->nFontWidth;
    }

    // adjust the scrollbar thumb position
    lps->nHScrollPos += dx;
    lps->nVScrollPos += dy;

    // ignore clipping rectangle if its a whole-window scroll
    if (fReturnUpdateRgn == false)
        GetClientRect(lps->hWnd, &clip);

    // take margin into account
    clip.left += LeftMarginWidth_TextView(lps);

    // perform the scroll
    if (dx != 0 || dy != 0)
    {
        // do the scroll!
        ScrollWindowEx(
            lps->hWnd,
            -dx * lps->nFontWidth,      // scale up to pixel coords
            -dy * lps->nLineHeight,
            NULL,                       // scroll entire window
            &clip,                      // clip the non-scrolling part
            0,
            0,
            SW_INVALIDATE
            );

        SetupScrollbars_TextView(lps);

        if (fReturnUpdateRgn)
        {
            RECT client;

            GetClientRect(lps->hWnd, &client);

            //clip.left -= LeftMarginWidth();

            HRGN hrgnClient = CreateRectRgnIndirect(&client);
            HRGN hrgnUpdate = CreateRectRgnIndirect(&clip);

            // create a region that represents the area outside the
            // clipping rectangle (i.e. the part that is never scrolled)
            CombineRgn(hrgnUpdate, hrgnClient, hrgnUpdate, RGN_XOR);

            DeleteObject(hrgnClient);

            return hrgnUpdate;
        }
    }

    if (dy != 0)
    {
        GetClientRect(lps->hWnd, &clip);
        clip.right = LeftMarginWidth_TextView(lps);
        //ScrollWindow(lps->hWnd, 0, -dy * lps->nLineHeight, 0, &clip);
        InvalidateRect(lps->hWnd, &clip, 0);
    }

    return NULL;
}

//
//    Scroll viewport in specified direction
//
VOID Scroll_TextView(
    TextView * lps,
    int dx,
    int dy
    )
{
    // do a "normal" scroll - don't worry about invalid regions,
    // just scroll the whole window 
    ScrollRgn_TextView(lps, dx, dy, false);
}

//
//    Ensure that the specified file-location is visible within
//  the window-viewport, Scrolling the viewport as necessary
//
VOID ScrollToPosition_TextView(
    TextView * lps,
    int xpos,
    ULONG lineno
    )
{
    bool fRefresh = false;
    RECT rect;
    int  marginWidth = LeftMarginWidth_TextView(lps);

    GetClientRect(lps->hWnd, &rect);

    xpos -= lps->nHScrollPos * lps->nFontWidth;
    xpos += marginWidth;

    if (xpos < marginWidth)
    {
        lps->nHScrollPos -= (marginWidth - xpos) / lps->nFontWidth;
        fRefresh = true;
    }

    if (xpos >= rect.right)
    {
        lps->nHScrollPos += (xpos - rect.right) / lps->nFontWidth + 1;
        fRefresh = true;
    }

    if (lineno < lps->nVScrollPos)
    {
        lps->nVScrollPos = lineno;
        fRefresh = true;
    }
    else if (lineno > lps->nVScrollPos + lps->nWindowLines - 1)
    {
        lps->nVScrollPos = lineno - lps->nWindowLines + 1;
        fRefresh = true;
    }


    if (fRefresh)
    {
        SetupScrollbars_TextView(lps);
        RefreshWindow_TextView(lps);
        RepositionCaret_TextView(lps);
    }
}

VOID ScrollToCaret_TextView(
    TextView * lps
    )
{
    ScrollToPosition_TextView(lps, lps->nCaretPosX, lps->nCurrentLine);
}

LONG GetTrackPos32(HWND hwnd, int nBar)
{
    SCROLLINFO si = { sizeof(si), SIF_TRACKPOS };
    GetScrollInfo(hwnd, nBar, &si);
    return si.nTrackPos;
}

//
//    Vertical scrollbar support
//
LONG OnVScroll_TextView(
    TextView * lps,
    UINT nSBCode,
    UINT nPos
    )
{
    ULONG oldpos = lps->nVScrollPos;

    switch (nSBCode)
    {
    case SB_TOP:
        lps->nVScrollPos = 0;
        RefreshWindow_TextView(lps);
        break;

    case SB_BOTTOM:
        lps->nVScrollPos = lps->nVScrollMax;
        RefreshWindow_TextView(lps);
        break;

    case SB_LINEUP:
        Scroll_TextView(lps, 0, -1);
        break;

    case SB_LINEDOWN:
        Scroll_TextView(lps, 0, 1);
        break;

    case SB_PAGEDOWN:
        Scroll_TextView(lps, 0, lps->nWindowLines);
        break;

    case SB_PAGEUP:
        Scroll_TextView(lps, 0, -lps->nWindowLines);
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:

        lps->nVScrollPos = GetTrackPos32(lps->hWnd, SB_VERT);
        RefreshWindow_TextView(lps);

        break;
    }

    if (oldpos != lps->nVScrollPos)
    {
        SetupScrollbars_TextView(lps);
        RepositionCaret_TextView(lps);
    }


    return 0;
}

//
//    Horizontal scrollbar support
//
LONG OnHScroll_TextView(
    TextView * lps,
    UINT nSBCode,
    UINT nPos
    )
{
    int oldpos = lps->nHScrollPos;

    switch (nSBCode)
    {
    case SB_LEFT:
        lps->nHScrollPos = 0;
        RefreshWindow_TextView(lps);
        break;

    case SB_RIGHT:
        lps->nHScrollPos = lps->nHScrollMax;
        RefreshWindow_TextView(lps);
        break;

    case SB_LINELEFT:
        Scroll_TextView(lps, -1, 0);
        break;

    case SB_LINERIGHT:
        Scroll_TextView(lps, 1, 0);
        break;

    case SB_PAGELEFT:
        Scroll_TextView(lps, -lps->nWindowColumns, 0);
        break;

    case SB_PAGERIGHT:
        Scroll_TextView(lps, lps->nWindowColumns, 0);
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:

        lps->nHScrollPos = GetTrackPos32(lps->hWnd, SB_HORZ);
        RefreshWindow_TextView(lps);
        break;
    }

    if (oldpos != lps->nHScrollPos)
    {
        SetupScrollbars_TextView(lps);
        RepositionCaret_TextView(lps);
    }

    return 0;
}

LONG OnMouseWheel_TextView(
    TextView * lps,
    int nDelta
    )
{
#ifndef SPI_GETWHEELSCROLLLINES    
#define SPI_GETWHEELSCROLLLINES   104
#endif

    if (!IsKeyPressed(VK_SHIFT))
    {
        int nScrollLines;

        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &nScrollLines, 0);

        if (nScrollLines <= 1)
            nScrollLines = 3;

        Scroll_TextView(lps, 0, (-nDelta / 120) * nScrollLines);
        RepositionCaret_TextView(lps);
    }

    return 0;
}
