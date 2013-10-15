//
//    MODULE:        TextViewKeyNav.cpp
//
//    PURPOSE:    Keyboard navigation for TextView
//
//    NOTES:        www.catch22.net
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include "TextView.h"
#include "TextViewInternal.h"

/*struct SCRIPT_LOGATTR
{
BYTE fSoftBreak    :1;
BYTE fWhiteSpace    :1;
BYTE fCharStop    :1;
BYTE fWordStop    :1;
BYTE fInvalid        :1;
BYTE fReserved    :3;
};*/


bool IsKeyPressed(UINT nVirtKey)
{
    return GetKeyState(nVirtKey) < 0 ? true : false;
}

//
//    Get the UspCache and logical attributes for specified line
//
bool GetLogAttr_TextView(
    TextView * lps,
    ULONG nLineNo,
    USPCACHE * *puspCache,
    CSCRIPT_LOGATTR * *plogAttr,
    ULONG * pnOffset
    )
{
    if ((*puspCache = GetUspCache_TextView(lps, 0, nLineNo, pnOffset)) == 0)
        return false;

    if (plogAttr && (*plogAttr = UspGetLogAttr((*puspCache)->uspData)) == 0)
        return false;

    return true;
}

//
//    Move caret up specified number of lines
//
VOID MoveLineUp_TextView(
    TextView * lps,
    int numLines
    )
{
    USPDATA            * uspData;
    ULONG              lineOffset;

    int                  charPos;
    BOOL              trailing;

    lps->nCurrentLine -= min(lps->nCurrentLine, (unsigned) numLines);

    // get Uniscribe data for prev line
    uspData = GetUspData_TextView(lps, 0, lps->nCurrentLine, &lineOffset);

    // move up to character nearest the caret-anchor positions
    UspXToOffset(uspData, lps->nAnchorPosX, &charPos, &trailing, 0);

    lps->nCursorOffset = lineOffset + charPos + trailing;
}

//
//    Move caret down specified number of lines
//
VOID MoveLineDown_TextView(
    TextView * lps,
    int numLines
    )
{
    USPDATA            * uspData;
    ULONG              lineOffset;

    int                  charPos;
    BOOL              trailing;

    lps->nCurrentLine += min(lps->nLineCount - lps->nCurrentLine - 1, (unsigned) numLines);

    // get Uniscribe data for prev line
    uspData = GetUspData_TextView(lps, 0, lps->nCurrentLine, &lineOffset);

    // move down to character nearest the caret-anchor position
    UspXToOffset(uspData, lps->nAnchorPosX, &charPos, &trailing, 0);

    lps->nCursorOffset = lineOffset + charPos + trailing;
}

//
//    Move to start of previous word (to the left)
//
VOID MoveWordPrev_TextView(
    TextView * lps
    )
{
    USPCACHE * uspCache;
    CSCRIPT_LOGATTR * logAttr;
    ULONG lineOffset;
    int charPos;

    // get Uniscribe data for current line
    if (!GetLogAttr_TextView(lps, lps->nCurrentLine, &uspCache, &logAttr, &lineOffset))
        return;

    // move 1 character to left
    charPos = lps->nCursorOffset - lineOffset - 1;

    // skip to end of *previous* line if necessary
    if (charPos < 0)
    {
        charPos = 0;

        if (lps->nCurrentLine > 0)
        {
            MoveLineEnd_TextView(lps, lps->nCurrentLine - 1);
            return;
        }
    }

    // skip preceding whitespace
    while (charPos > 0 && logAttr[charPos].fWhiteSpace)
        charPos--;

    // skip whole characters until we hit a word-break/more whitespace
    for (; charPos > 0; charPos--)
    {
        if (logAttr[charPos].fWordStop || logAttr[charPos - 1].fWhiteSpace)
            break;
    }

    lps->nCursorOffset = lineOffset + charPos;
}

//
//    Move to start of next word
//
VOID MoveWordNext_TextView(
    TextView * lps
    )
{
    USPCACHE        * uspCache;
    CSCRIPT_LOGATTR * logAttr;
    ULONG              lineOffset;
    int                  charPos;

    // get Uniscribe data for current line
    if (!GetLogAttr_TextView(lps, lps->nCurrentLine, &uspCache, &logAttr, &lineOffset))
        return;

    charPos = lps->nCursorOffset - lineOffset;

    // if already at end-of-line, skip to next line
    if (charPos == uspCache->length_CRLF)
    {
        if (lps->nCurrentLine + 1 < lps->nLineCount)
            MoveLineStart_TextView(lps, lps->nCurrentLine + 1);

        return;
    }

    // if already on a word-break, go to next char
    if (logAttr[charPos].fWordStop)
        charPos++;

    // skip whole characters until we hit a word-break/more whitespace
    for (; charPos < uspCache->length_CRLF; charPos++)
    {
        if (logAttr[charPos].fWordStop || logAttr[charPos].fWhiteSpace)
            break;
    }

    // skip trailing whitespace
    while (charPos < uspCache->length_CRLF && logAttr[charPos].fWhiteSpace)
        charPos++;

    lps->nCursorOffset = lineOffset + charPos;
}

//
//    Move to start of current word
//
VOID MoveWordStart_TextView(
    TextView * lps
    )
{
    USPCACHE        * uspCache;
    CSCRIPT_LOGATTR * logAttr;
    ULONG              lineOffset;
    int                  charPos;

    // get Uniscribe data for current line
    if (!GetLogAttr_TextView(lps, lps->nCurrentLine, &uspCache, &logAttr, &lineOffset))
        return;

    charPos = lps->nCursorOffset - lineOffset;

    while (charPos > 0 && !logAttr[charPos - 1].fWhiteSpace)
        charPos--;

    lps->nCursorOffset = lineOffset + charPos;
}

//
//    Move to end of current word
//
VOID MoveWordEnd_TextView(
    TextView * lps
    )
{
    USPCACHE        * uspCache;
    CSCRIPT_LOGATTR * logAttr;
    ULONG              lineOffset;
    int                  charPos;

    // get Uniscribe data for current line
    if (!GetLogAttr_TextView(lps, lps->nCurrentLine, &uspCache, &logAttr, &lineOffset))
        return;

    charPos = lps->nCursorOffset - lineOffset;

    while (charPos < uspCache->length_CRLF && !logAttr[charPos].fWhiteSpace)
        charPos++;

    lps->nCursorOffset = lineOffset + charPos;
}

//
//    Move to previous character
//
VOID MoveCharPrev_TextView(
    TextView * lps
    )
{
    USPCACHE        * uspCache;
    CSCRIPT_LOGATTR * logAttr;
    ULONG              lineOffset;
    int                  charPos;

    // get Uniscribe data for current line
    if (!GetLogAttr_TextView(lps, lps->nCurrentLine, &uspCache, &logAttr, &lineOffset))
        return;

    charPos = lps->nCursorOffset - lineOffset;

    // find the previous valid character-position
    for (--charPos; charPos >= 0; charPos--)
    {
        if (logAttr[charPos].fCharStop)
            break;
    }

    // move up to end-of-last line if necessary
    if (charPos < 0)
    {
        charPos = 0;

        if (lps->nCurrentLine > 0)
        {
            MoveLineEnd_TextView(lps, lps->nCurrentLine - 1);
            return;
        }
    }

    // update cursor position
    lps->nCursorOffset = lineOffset + charPos;
}

//
//    Move to next character
//
VOID MoveCharNext_TextView(
    TextView * lps
    )
{
    USPCACHE        * uspCache;
    CSCRIPT_LOGATTR * logAttr;
    ULONG              lineOffset;
    int                  charPos;

    // get Uniscribe data for specified line
    if (!GetLogAttr_TextView(lps, lps->nCurrentLine, &uspCache, &logAttr, &lineOffset))
        return;

    charPos = lps->nCursorOffset - lineOffset;

    // find the next valid character-position
    for (++charPos; charPos <= uspCache->length_CRLF; charPos++)
    {
        if (logAttr[charPos].fCharStop)
            break;
    }

    // skip to beginning of next line if we hit the CR/LF
    if (charPos > uspCache->length_CRLF)
    {
        if (lps->nCurrentLine + 1 < lps->nLineCount)
            MoveLineStart_TextView(lps, lps->nCurrentLine + 1);
    }
    // otherwise advance the character-position
    else
    {
        lps->nCursorOffset = lineOffset + charPos;
    }
}

//
//    Move to start of specified line
//
VOID MoveLineStart_TextView(
    TextView * lps,
    ULONG lineNo
    )
{
    ULONG              lineOffset;
    USPCACHE        * uspCache;
    CSCRIPT_LOGATTR * logAttr;
    int                  charPos;

    // get Uniscribe data for current line
    if (!GetLogAttr_TextView(lps, lineNo, &uspCache, &logAttr, &lineOffset))
        return;

    charPos = lps->nCursorOffset - lineOffset;

    // if already at start of line, skip *forwards* past any whitespace
    if (lps->nCursorOffset == lineOffset)
    {
        // skip whitespace
        while (logAttr[lps->nCursorOffset - lineOffset].fWhiteSpace)
            lps->nCursorOffset++;
    }
    // if not at start, goto start
    else
    {
        lps->nCursorOffset = lineOffset;
    }
}

//
//    Move to end of specified line
//
VOID MoveLineEnd_TextView(
    TextView * lps,
    ULONG lineNo
    )
{
    USPCACHE *uspCache;

    if ((uspCache = GetUspCache_TextView(lps, 0, lineNo)) == 0)
        return;

    lps->nCursorOffset = uspCache->offset + uspCache->length_CRLF;
}

//
//    Move to start of file
//
VOID MoveFileStart_TextView(
    TextView * lps
    )
{
    lps->nCursorOffset = 0;
}

//
//    Move to end of file
//
VOID MoveFileEnd_TextView(
    TextView * lps
    )
{
    lps->nCursorOffset = size_TextDocument(lps->pTextDoc);
}


//
//    Process keyboard-navigation keys
//
LONG OnKeyDown_TextView(
    TextView * lps,
    UINT nKeyCode,
    UINT nFlags
    )
{
    bool fCtrlDown = IsKeyPressed(VK_CONTROL);
    bool fShiftDown = IsKeyPressed(VK_SHIFT);
    BOOL fAdvancing = FALSE;

    //
    //    Process the key-press. Cursor movement is different depending
    //    on if <ctrl> is held down or not, so act accordingly
    //
    switch (nKeyCode)
    {
    case VK_SHIFT: case VK_CONTROL:
        return 0;

        // CTRL+Z undo
    case 'z': case 'Z':

        if (fCtrlDown && Undo_TextView(lps))
            NotifyParent_TextView(lps, TVN_CHANGED);

        return 0;

        // CTRL+Y redo
    case 'y': case 'Y':

        if (fCtrlDown && Redo_TextView(lps))
            NotifyParent_TextView(lps, TVN_CHANGED);

        return 0;

        // Change insert mode / clipboard copy&paste
    case VK_INSERT:

        if (fCtrlDown)
        {
            OnCopy_TextView(lps);
            NotifyParent_TextView(lps, TVN_CHANGED);
        }
        else if (fShiftDown)
        {
            OnPaste_TextView(lps);
            NotifyParent_TextView(lps, TVN_CHANGED);
        }
        else
        {
            if (lps->nEditMode == MODE_INSERT)
                lps->nEditMode = MODE_OVERWRITE;

            else if (lps->nEditMode == MODE_OVERWRITE)
                lps->nEditMode = MODE_INSERT;

            NotifyParent_TextView(lps, TVN_EDITMODE_CHANGE);
        }

        return 0;

    case VK_DELETE:

        if (lps->nEditMode != MODE_READONLY)
        {
            if (fShiftDown)
                OnCut_TextView(lps);
            else
                ForwardDelete_TextView(lps);

            NotifyParent_TextView(lps, TVN_CHANGED);
        }
        return 0;

    case VK_BACK:

        if (lps->nEditMode != MODE_READONLY)
        {
            BackDelete_TextView(lps);
            fAdvancing = FALSE;

            NotifyParent_TextView(lps, TVN_CHANGED);
        }
        return 0;

    case VK_LEFT:

        if (fCtrlDown)
        {
            MoveWordPrev_TextView(lps);
        }
        else
        {
            MoveCharPrev_TextView(lps);
        }

        fAdvancing = FALSE;
        break;

    case VK_RIGHT:

        if (fCtrlDown)
        {
            MoveWordNext_TextView(lps);
        }
        else
        {
            MoveCharNext_TextView(lps);
        }

        fAdvancing = TRUE;
        break;

    case VK_UP:
        if (fCtrlDown)
        {
            Scroll_TextView(lps, 0, -1);
        }
        else
        {
            MoveLineUp_TextView(lps, 1);
        }
        break;

    case VK_DOWN:
        if (fCtrlDown)
        {
            Scroll_TextView(lps, 0, 1);
        }
        else
        {
            MoveLineDown_TextView(lps, 1);
        }
        break;

    case VK_PRIOR:
        if (!fCtrlDown)
        {
            MoveLineUp_TextView(lps, lps->nWindowLines);
        }
        break;

    case VK_NEXT:
        if (!fCtrlDown)
        {
            MoveLineDown_TextView(lps, lps->nWindowLines);
        }
        break;

    case VK_HOME:
        if (fCtrlDown)
        {
            MoveFileStart_TextView(lps);
        }
        else
        {
            MoveLineStart_TextView(lps, lps->nCurrentLine);
        }
        break;

    case VK_END:
        if (fCtrlDown)
        {
            MoveFileEnd_TextView(lps);
        }
        else
        {
            MoveLineEnd_TextView(lps, lps->nCurrentLine);
        }
        break;

    default:
        return 0;
    }

    // Extend selection if <shift> is down
    if (fShiftDown)
    {
        InvalidateRange_TextView(lps, lps->nSelectionEnd, lps->nCursorOffset);
        lps->nSelectionEnd = lps->nCursorOffset;
    }
    // Otherwise clear the selection
    else
    {
        if (lps->nSelectionStart != lps->nSelectionEnd)
        {
            InvalidateRange_TextView(lps, lps->nSelectionStart, lps->nSelectionEnd);
        }

        lps->nSelectionEnd = lps->nCursorOffset;
        lps->nSelectionStart = lps->nCursorOffset;
    }

    // update caret-location (xpos, line#) from the offset
    UpdateCaretOffset_TextView(lps, lps->nCursorOffset, fAdvancing, &lps->nCaretPosX, &lps->nCurrentLine);

    // maintain the caret 'anchor' position *except* for up/down actions
    if (nKeyCode != VK_UP && nKeyCode != VK_DOWN)
    {
        lps->nAnchorPosX = lps->nCaretPosX;

        // scroll as necessary to keep caret within viewport
        ScrollToPosition_TextView(lps, lps->nCaretPosX, lps->nCurrentLine);
    }
    else
    {
        // scroll as necessary to keep caret within viewport
        if (!fCtrlDown)
        {
            ScrollToPosition_TextView(lps, lps->nCaretPosX, lps->nCurrentLine);
        }
    }

    NotifyParent_TextView(lps, TVN_CURSOR_CHANGE);

    return 0;
}
