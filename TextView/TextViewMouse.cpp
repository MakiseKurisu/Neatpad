//
//    MODULE:        TextViewMouse.cpp
//
//    PURPOSE:    Mouse and caret support for the TextView control
//
//    NOTES:        www.catch22.net
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include "TextView.h"
#include "TextViewInternal.h"

int ScrollDir(int counter, int dir);
bool IsKeyPressed(UINT nVirtKey);

//
//    WM_MOUSEACTIVATE
//
//    Grab the keyboard input focus 
//    
LONG OnMouseActivate_TextView(
    TextView * lps,
    HWND hwndTop,
    UINT nHitTest,
    UINT nMessage
    )
{
    SetFocus(lps->hWnd);
    return MA_ACTIVATE;
}

HMENU CreateContextMenu_TextView(
    TextView * lps
    )
{
    HMENU hMenu = CreatePopupMenu();

    // do we have a selection?
    UINT fSelection = (lps->nSelectionStart == lps->nSelectionEnd) ?
        MF_DISABLED | MF_GRAYED : MF_ENABLED;

    // is there text on the clipboard?
    UINT fClipboard = (IsClipboardFormatAvailable(CF_TEXT) || IsClipboardFormatAvailable(CF_UNICODETEXT)) ?
    MF_ENABLED : MF_GRAYED | MF_DISABLED;

    UINT fCanUndo = CanUndo_TextView(lps) ? MF_ENABLED : MF_GRAYED | MF_DISABLED;
    UINT fCanRedo = CanRedo_TextView(lps) ? MF_ENABLED : MF_GRAYED | MF_DISABLED;

    AppendMenu(hMenu, MF_STRING | fCanUndo, WM_UNDO, TEXT("&Undo"));
    AppendMenu(hMenu, MF_STRING | fCanRedo, TXM_REDO, TEXT("&Redo"));
    AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
    AppendMenu(hMenu, MF_STRING | fSelection, WM_CUT, TEXT("Cu&t"));
    AppendMenu(hMenu, MF_STRING | fSelection, WM_COPY, TEXT("&Copy"));
    AppendMenu(hMenu, MF_STRING | fClipboard, WM_PASTE, TEXT("&Paste"));
    AppendMenu(hMenu, MF_STRING | fSelection, WM_CLEAR, TEXT("&Delete"));
    AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
    AppendMenu(hMenu, MF_STRING | MF_ENABLED, TXM_SETSELALL, TEXT("&Select All"));
    AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
    AppendMenu(hMenu, MF_STRING | MF_ENABLED, WM_USER + 7, TEXT("&Right to left Reading order"));
    AppendMenu(hMenu, MF_STRING | MF_ENABLED, WM_USER + 8, TEXT("&Show Unicode control characters"));

    return hMenu;
}

//
//    WM_CONTEXTMENU
//
//    Respond to right-click message
//
LONG OnContextMenu_TextView(
    TextView * lps,
    HWND hwndParam,
    int x,
    int y
    )
{
    if (lps->hUserMenu == 0)
    {
        HMENU hMenu = CreateContextMenu_TextView(lps);
        UINT  uCmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, x, y, 0, lps->hWnd, 0);

        if (uCmd != 0)
            PostMessage(lps->hWnd, uCmd, 0, 0);

        return 0;
    }
    else
    {
        UINT uCmd = TrackPopupMenu(lps->hUserMenu, TPM_RETURNCMD, x, y, 0, lps->hWnd, 0);

        if (uCmd != 0)
            PostMessage(GetParent(lps->hWnd), WM_COMMAND, MAKEWPARAM(uCmd, 0), (LPARAM) GetParent(lps->hWnd));

        return 0;
    }
    //PostMessage(lps->hWnd, WM_COMMAND, MAKEWPARAM(uCmd, 0), (LPARAM)lps->hWnd);

    return DefWindowProc(lps->hWnd, WM_CONTEXTMENU, (WPARAM) hwndParam, MAKELONG(x, y));
}


//
//    WM_LBUTTONDOWN
//
//  Position caret to nearest text character under mouse
//
LONG OnLButtonDown_TextView(
    TextView * lps,
    UINT nFlags,
    int mx,
    int my
    )
{
    ULONG nLineNo;
    ULONG nFileOff;

    // regular mouse input - mouse is within 
    if (mx >= LeftMarginWidth_TextView(lps))
    {
        // map the mouse-coordinates to a real file-offset-coordinate
        MouseCoordToFilePos_TextView(lps, mx, my, &nLineNo, &nFileOff, &lps->nCaretPosX);
        lps->nAnchorPosX = lps->nCaretPosX;

        UpdateCaretXY_TextView(lps, lps->nCaretPosX, nLineNo);

        // Any key but <shift>
        if (IsKeyPressed(VK_SHIFT) == false)
        {
            // remove any existing selection
            InvalidateRange_TextView(lps, lps->nSelectionStart, lps->nSelectionEnd);

            // reset cursor and selection offsets to the same location
            lps->nSelectionStart = nFileOff;
            lps->nSelectionEnd = nFileOff;
            lps->nCursorOffset = nFileOff;
        }
        else
        {
            // redraw to cursor
            InvalidateRange_TextView(lps, lps->nSelectionEnd, nFileOff);

            // extend selection to cursor
            lps->nSelectionEnd = nFileOff;
            lps->nCursorOffset = nFileOff;
        }

        if (IsKeyPressed(VK_MENU))
        {
            lps->cpBlockStart.line = nLineNo;
            lps->cpBlockStart.xpos = lps->nCaretPosX;
            lps->nSelectionType = SEL_BLOCK;
        }
        else
        {
            lps->nSelectionType = SEL_NORMAL;
        }

        // set capture for mouse-move selections
        lps->nSelectionMode = IsKeyPressed(VK_MENU) ? SEL_BLOCK : SEL_NORMAL;
    }
    // mouse clicked within margin 
    else
    {
        // remove any existing selection
        InvalidateRange_TextView(lps, lps->nSelectionStart, lps->nSelectionEnd);

        nLineNo = (my / lps->nLineHeight) + lps->nVScrollPos;

        //
        // if we click in the margin then jump back to start of line
        //
        if (lps->nHScrollPos != 0)
        {
            lps->nHScrollPos = 0;
            SetupScrollbars_TextView(lps);
            RefreshWindow_TextView(lps);
        }

        lineinfo_from_lineno_TextDocument(lps->pTextDoc, nLineNo, &lps->nSelectionStart, &lps->nSelectionEnd, 0, 0);
        lps->nSelectionEnd += lps->nSelectionStart;
        lps->nCursorOffset = lps->nSelectionStart;

        lps->nSelMarginOffset1 = lps->nSelectionStart;
        lps->nSelMarginOffset2 = lps->nSelectionEnd;

        InvalidateRange_TextView(lps, lps->nSelectionStart, lps->nSelectionEnd);

        UpdateCaretOffset_TextView(lps, lps->nCursorOffset, FALSE, &lps->nCaretPosX, &lps->nCurrentLine);
        lps->nAnchorPosX = lps->nCaretPosX;

        // set capture for mouse-move selections
        lps->nSelectionMode = SEL_MARGIN;
    }

    UpdateLine_TextView(lps, nLineNo);

    SetCapture(lps->hWnd);

    TVNCURSORINFO ci = { { 0 }, nLineNo, 0, lps->nCursorOffset };
    NotifyParent_TextView(lps, TVN_CURSOR_CHANGE, (NMHDR *) &ci);
    return 0;
}

//
//    WM_LBUTTONUP 
//
//    Release capture and cancel any mouse-scrolling
//
LONG OnLButtonUp_TextView(
    TextView * lps,
    UINT nFlags,
    int mx,
    int my
    )
{
    // shift cursor to end of selection
    if (lps->nSelectionMode == SEL_MARGIN)
    {
        lps->nCursorOffset = lps->nSelectionEnd;
        UpdateCaretOffset_TextView(lps, lps->nCursorOffset, FALSE, &lps->nCaretPosX, &lps->nCurrentLine);
    }

    if (lps->nSelectionMode)
    {
        // cancel the scroll-timer if it is still running
        if (lps->nScrollTimer != 0)
        {
            KillTimer(lps->hWnd, lps->nScrollTimer);
            lps->nScrollTimer = 0;
        }

        lps->nSelectionMode = SEL_NONE;
        ReleaseCapture();
    }

    return 0;
}

//
//    WM_LBUTTONDBKCLK
//
//    Select the word under the mouse
//
LONG OnLButtonDblClick_TextView(
    TextView * lps,
    UINT nFlags,
    int mx,
    int my
    )
{
    // remove any existing selection
    InvalidateRange_TextView(lps, lps->nSelectionStart, lps->nSelectionEnd);

    // regular mouse input - mouse is within scrolling viewport
    if (mx >= LeftMarginWidth_TextView(lps))
    {
        ULONG lineno, fileoff;
        int   xpos;

        // map the mouse-coordinates to a real file-offset-coordinate
        MouseCoordToFilePos_TextView(lps, mx, my, &lineno, &fileoff, &xpos);
        lps->nAnchorPosX = lps->nCaretPosX;

        // move selection-start to start of word
        MoveWordStart_TextView(lps);
        lps->nSelectionStart = lps->nCursorOffset;

        // move selection-end to end of word
        MoveWordEnd_TextView(lps);
        lps->nSelectionEnd = lps->nCursorOffset;

        // update caret position
        InvalidateRange_TextView(lps, lps->nSelectionStart, lps->nSelectionEnd);
        UpdateCaretOffset_TextView(lps, lps->nCursorOffset, TRUE, &lps->nCaretPosX, &lps->nCurrentLine);
        lps->nAnchorPosX = lps->nCaretPosX;

        NotifyParent_TextView(lps, TVN_CURSOR_CHANGE);
    }

    return 0;
}

//
//    WM_MOUSEMOVE
//
//    Set the selection end-point if we are dragging the mouse
//
LONG OnMouseMove_TextView(
    TextView * lps,
    UINT nFlags,
    int mx,
    int my
    )
{
    if (lps->nSelectionMode)
    {
        ULONG    nLineNo, nFileOff;
        BOOL    fCurChanged = FALSE;

        RECT    rect;
        POINT    pt = { mx, my };

        //
        //    First thing we must do is switch from margin-mode to normal-mode 
        //    if the mouse strays into the main document area
        //
        if (lps->nSelectionMode == SEL_MARGIN && mx > LeftMarginWidth_TextView(lps))
        {
            lps->nSelectionMode = SEL_NORMAL;
            SetCursor(LoadCursor(0, IDC_IBEAM));
        }

        //
        //    Mouse-scrolling: detect if the mouse
        //    is inside/outside of the TextView scrolling area
        //  and stop/start a scrolling timer appropriately
        //
        GetClientRect(lps->hWnd, &rect);

        // build the scrolling area
        rect.bottom -= rect.bottom % lps->nLineHeight;
        rect.left += LeftMarginWidth_TextView(lps);

        // If mouse is within this area, we don't need to scroll
        if (PtInRect(&rect, pt))
        {
            if (lps->nScrollTimer != 0)
            {
                KillTimer(lps->hWnd, lps->nScrollTimer);
                lps->nScrollTimer = 0;
            }
        }
        // If mouse is outside window, start a timer in
        // order to generate regular scrolling intervals
        else
        {
            if (lps->nScrollTimer == 0)
            {
                lps->nScrollCounter = 0;
                lps->nScrollTimer = SetTimer(lps->hWnd, 1, 30, 0);
            }
        }

        // get new cursor offset+coordinates
        MouseCoordToFilePos_TextView(lps, mx, my, &nLineNo, &nFileOff, &lps->nCaretPosX);
        lps->nAnchorPosX = lps->nCaretPosX;

        lps->cpBlockEnd.line = nLineNo;
        lps->cpBlockEnd.xpos = mx + lps->nHScrollPos * lps->nFontWidth - LeftMarginWidth_TextView(lps); //lps->nCaretPosX;


        // redraw the old and new lines if they are different
        UpdateLine_TextView(lps, nLineNo);

        // update the region of text that has changed selection state
        fCurChanged = lps->nSelectionEnd == nFileOff ? FALSE : TRUE;
        //if(lps->nSelectionEnd != nFileOff)
        {
            ULONG linelen;
            lineinfo_from_lineno_TextDocument(lps->pTextDoc, nLineNo, 0, &linelen, 0, 0);

            lps->nCursorOffset = nFileOff;

            if (lps->nSelectionMode == SEL_MARGIN)
            {
                if (nFileOff >= lps->nSelectionStart)
                {
                    nFileOff += linelen;
                    lps->nSelectionStart = lps->nSelMarginOffset1;
                }
                else
                {
                    lps->nSelectionStart = lps->nSelMarginOffset2;
                }
            }

            // redraw from old selection-pos to new position
            InvalidateRange_TextView(lps, lps->nSelectionEnd, nFileOff);

            // adjust the cursor + selection to the new offset
            lps->nSelectionEnd = nFileOff;
        }

        if (lps->nSelectionMode == SEL_BLOCK)
            RefreshWindow_TextView(lps);

        //lps->nCaretPosX = mx+lps->nHScrollPos*lps->nFontWidth-LeftMarginWidth();
        // always set the caret position because we might be scrolling
        UpdateCaretXY_TextView(lps, lps->nCaretPosX, lps->nCurrentLine);

        if (fCurChanged)
        {
            NotifyParent_TextView(lps, TVN_CURSOR_CHANGE);
        }
    }
    // mouse isn't being used for a selection, so set the cursor instead
    else
    {
        if (mx < LeftMarginWidth_TextView(lps))
        {
            SetCursor(lps->hMarginCursor);
        }
        else
        {
            //OnLButtonDown(0, mx, my);
            SetCursor(LoadCursor(0, IDC_IBEAM));
        }

    }

    return 0;
}

//
//    WM_TIMER handler
//
//    Used to create regular scrolling 
//
LONG OnTimer_TextView(
    TextView * lps,
    UINT nTimerId
    )
{
    int      dx = 0, dy = 0;    // scrolling vectors
    RECT  rect;
    POINT pt;

    // find client area, but make it an even no. of lines
    GetClientRect(lps->hWnd, &rect);
    rect.bottom -= rect.bottom % lps->nLineHeight;
    rect.left += LeftMarginWidth_TextView(lps);

    // get the mouse's client-coordinates
    GetCursorPos(&pt);
    ScreenToClient(lps->hWnd, &pt);

    //
    // scrolling up / down??
    //
    if (pt.y < rect.top)
        dy = ScrollDir(lps->nScrollCounter, pt.y - rect.top);

    else if (pt.y >= rect.bottom)
        dy = ScrollDir(lps->nScrollCounter, pt.y - rect.bottom);

    //
    // scrolling left / right?
    //
    if (pt.x < rect.left)
        dx = ScrollDir(lps->nScrollCounter, pt.x - rect.left);

    else if (pt.x > rect.right)
        dx = ScrollDir(lps->nScrollCounter, pt.x - rect.right);

    //
    // Scroll the window but don't update any invalid
    // areas - we will do this manually after we have 
    // repositioned the caret
    //
    HRGN hrgnUpdate = ScrollRgn_TextView(lps, dx, dy, true);

    //
    // do the redraw now that the selection offsets are all 
    // pointing to the right places and the scroll positions are valid.
    //
    if (hrgnUpdate != NULL)
    {
        // We perform a "fake" WM_MOUSEMOVE for two reasons:
        //
        // 1. To get the cursor/caret/selection offsets set to the correct place
        //    *before* we redraw (so everything is synchronized correctly)
        //
        // 2. To invalidate any areas due to mouse-movement which won't
        //    get done until the next WM_MOUSEMOVE - and then it would
        //    be too late because we need to redraw *now*
        //
        OnMouseMove_TextView(lps, 0, pt.x, pt.y);

        // invalidate the area returned by ScrollRegion
        InvalidateRgn(lps->hWnd, hrgnUpdate, FALSE);
        DeleteObject(hrgnUpdate);

        // the next time we process WM_PAINT everything 
        // should get drawn correctly!!
        UpdateWindow(lps->hWnd);
    }

    // keep track of how many WM_TIMERs we process because
    // we might want to skip the next one
    lps->nScrollCounter++;

    return 0;
}

//
//    Convert mouse(client) coordinates to a file-relative offset
//
//    Currently only uses the main font so will not support other
//    fonts introduced by syntax highlighting
//
BOOL MouseCoordToFilePos_TextView(
    TextView * lps,
    int mx, // [in]  mouse x-coord
    int my, // [in]  mouse x-coord
    ULONG * pnLineNo, // [out] line number
    ULONG * pnFileOffset, // [out] zero-based file-offset (in chars)
    int * psnappedX  // [out] adjusted x coord of caret
    )
{
    ULONG nLineNo;
    ULONG off_chars;
    RECT  rect;
    int      cp;

    // get scrollable area
    GetClientRect(lps->hWnd, &rect);
    rect.bottom -= rect.bottom % lps->nLineHeight;

    // take left margin into account
    mx -= LeftMarginWidth_TextView(lps);

    // clip mouse to edge of window
    if (mx < 0)                mx = 0;
    if (my < 0)                my = 0;
    if (my >= rect.bottom)    my = rect.bottom - 1;
    if (mx >= rect.right)    mx = rect.right - 1;

    // It's easy to find the line-number: just divide 'y' by the line-height
    nLineNo = (my / lps->nLineHeight) + lps->nVScrollPos;

    // make sure we don't go outside of the document
    if (nLineNo >= lps->nLineCount)
    {
        nLineNo = lps->nLineCount ? lps->nLineCount - 1 : 0;
        off_chars = size_TextDocument(lps->pTextDoc);
    }

    mx += lps->nHScrollPos * lps->nFontWidth;

    // get the USPDATA object for the selected line!!
    USPDATA * uspData = GetUspData_TextView(lps, 0, nLineNo);

    // convert mouse-x coordinate to a character-offset relative to start of line
    UspSnapXToOffset(uspData, mx, &mx, &cp, 0);

    // return coords!
    TextIterator * itor = iterate_line_TextDocument(lps->pTextDoc, nLineNo, &off_chars);
    *pnLineNo = nLineNo;
    *pnFileOffset = cp + off_chars;
    *psnappedX = mx;// - lps->nHScrollPos * lps->nFontWidth;
    //*psnappedX += LeftMarginWidth();
    delete_TextIterator(itor);
    return 0;
}

LONG InvalidateLine_TextView(
    TextView * lps,
    ULONG nLineNo,
    bool forceAnalysis
    )
{
    if (nLineNo >= lps->nVScrollPos && nLineNo <= lps->nVScrollPos + lps->nWindowLines)
    {
        RECT rect;

        GetClientRect(lps->hWnd, &rect);

        rect.top = (nLineNo - lps->nVScrollPos) * lps->nLineHeight;
        rect.bottom = rect.top + lps->nLineHeight;

        InvalidateRect(lps->hWnd, &rect, FALSE);
    }

    if (forceAnalysis)
    {
        for (int i = 0; i < USP_CACHE_SIZE; i++)
        {
            if (nLineNo == lps->uspCache[i].lineno)
            {
                lps->uspCache[i].usage = 0;
                break;
            }
        }
    }

    return 0;
}
//
//    Redraw any line which spans the specified range of text
//
LONG InvalidateRange_TextView(
    TextView * lps,
    ULONG nStart,
    ULONG nFinish
    )
{
    ULONG start = min(nStart, nFinish);
    ULONG finish = max(nStart, nFinish);

    int   ypos;
    RECT  rect;
    RECT  client;
    TextIterator * itor;

    // information about current line:
    ULONG lineno;
    ULONG off_chars;
    ULONG len_chars;

    // nothing to do?
    if (start == finish)
        return 0;

    //
    //    Find the start-of-line information from specified file-offset
    //
    lineno = lineno_from_offset_TextDocument(lps->pTextDoc, start);

    // clip to top of window
    if (lineno < lps->nVScrollPos)
    {
        lineno = lps->nVScrollPos;
        itor = iterate_line_TextDocument(lps->pTextDoc, lineno, &off_chars, &len_chars);
        start = off_chars;
    }
    else
    {
        itor = iterate_line_TextDocument(lps->pTextDoc, lineno, &off_chars, &len_chars);
    }

    if (!valid_TextIterator(itor) || start >= finish)
    {
        delete_TextIterator(itor);
        return 0;
    }

    ypos = (lineno - lps->nVScrollPos) * lps->nLineHeight;
    GetClientRect(lps->hWnd, &client);

    // invalidate *whole* lines. don't care about flickering anymore because
    // all output is double-buffered now, and this method is much simpler
    while (valid_TextIterator(itor) && off_chars < finish)
    {
        SetRect(&rect, 0, ypos, client.right, ypos + lps->nLineHeight);
        rect.left -= lps->nHScrollPos * lps->nFontWidth;
        rect.left += LeftMarginWidth_TextView(lps);

        InvalidateRect(lps->hWnd, &rect, FALSE);

        // jump down to next line
        delete_TextIterator(itor);
        itor = iterate_line_TextDocument(lps->pTextDoc, ++lineno, &off_chars, &len_chars);
        ypos += lps->nLineHeight;
    }

    delete_TextIterator(itor);
    return 0;
}
/*
//
//    Wrapper around SetCaretPos, hides the caret when it goes
//  off-screen (this protects against x/y wrap around due to integer overflow)
//
VOID MoveCaret_TextView(
    TextView * lps,
    int x,
    int y
    )
{
if(x < LeftMarginWidth() && lps->fHideCaret == false)
{
lps->fHideCaret = true;
HideCaret(lps->hWnd);
}
else if(x >= LeftMarginWidth() && lps->fHideCaret == true)
{
lps->fHideCaret = false;
ShowCaret(lps->hWnd);
}

if(lps->fHideCaret == false)
SetCaretPos(x, y);
}*/

//
//    x        - x-coord relative to start of line
//    lineno    - line-number
//
VOID UpdateCaretXY_TextView(
    TextView * lps,
    int xpos,
    ULONG lineno
    )
{
    bool visible = false;

    // convert x-coord to window-relative
    xpos -= lps->nHScrollPos * lps->nFontWidth;
    xpos += LeftMarginWidth_TextView(lps);

    // only show caret if it is visible within viewport
    if (lineno >= lps->nVScrollPos && lineno <= lps->nVScrollPos + lps->nWindowLines)
    {
        if (xpos >= LeftMarginWidth_TextView(lps))
            visible = true;
    }

    // hide caret if it was previously visible
    if (visible == false && lps->fHideCaret == false)
    {
        lps->fHideCaret = true;
        HideCaret(lps->hWnd);
    }
    // show caret if it was previously hidden
    else if (visible == true && lps->fHideCaret == true)
    {
        lps->fHideCaret = false;
        ShowCaret(lps->hWnd);
    }

    // set caret position if within window viewport
    if (lps->fHideCaret == false)
    {
        SetCaretPos(xpos, (lineno - lps->nVScrollPos) * lps->nLineHeight);
    }
}

//
//    Reposition the caret based on cursor-offset
//    return the resulting x-coord and line#
//
VOID UpdateCaretOffset_TextView(
    TextView * lps,
    ULONG offset,
    BOOL fTrailing,
    int * outx,
    ULONG * outlineno
    )
{
    ULONG        lineno = 0;
    int            xpos = 0;
    ULONG        off_chars;
    USPDATA      * uspData;

    // get line information from cursor-offset
    if (lineinfo_from_offset_TextDocument(lps->pTextDoc, offset, &lineno, &off_chars, 0, 0, 0))
    {
        // locate the USPDATA for this line
        if ((uspData = GetUspData_TextView(lps, NULL, lineno)) != 0)
        {
            // convert character-offset to x-coordinate
            off_chars = lps->nCursorOffset - off_chars;

            if (fTrailing && off_chars > 0)
                UspOffsetToX(uspData, off_chars - 1, TRUE, &xpos);
            else
                UspOffsetToX(uspData, off_chars, FALSE, &xpos);

            // update caret position
            UpdateCaretXY_TextView(lps, xpos, lineno);
        }
    }

    if (outx)      *outx = xpos;
    if (outlineno) *outlineno = lineno;
}

VOID RepositionCaret_TextView(
    TextView * lps
    )
{
    UpdateCaretXY_TextView(lps, lps->nCaretPosX, lps->nCurrentLine);
}

void UpdateLine_TextView(
    TextView * lps,
    ULONG nLineNo
    )
{
    // redraw the old and new lines if they are different
    if (lps->nCurrentLine != nLineNo)
    {
        if (CheckStyle_TextView(lps, TXS_HIGHLIGHTCURLINE))
            InvalidateLine_TextView(lps, lps->nCurrentLine, true);

        lps->nCurrentLine = nLineNo;

        if (CheckStyle_TextView(lps, TXS_HIGHLIGHTCURLINE))
            InvalidateLine_TextView(lps, lps->nCurrentLine, true);
    }
}

//
//    return direction to scroll (+ve, -ve or 0) based on 
//  distance of mouse from window edge
//
//    note: counter now redundant, we scroll multiple lines at
//  a time (with a slower timer than before) to achieve
//    variable-speed scrolling
//
int ScrollDir(int counter, int distance)
{
    if (distance > 48)        return 5;
    if (distance > 16)        return 2;
    if (distance > 3)        return 1;
    if (distance > 0)        return counter % 5 == 0 ? 1 : 0;

    if (distance < -48)        return -5;
    if (distance < -16)        return -2;
    if (distance < -3)        return -1;
    if (distance < 0)        return counter % 5 == 0 ? -1 : 0;

    return 0;
}



