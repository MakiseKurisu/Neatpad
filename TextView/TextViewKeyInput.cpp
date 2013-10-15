//
//    MODULE:        TextViewKeyInput.cpp
//
//    PURPOSE:    Keyboard input for TextView
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
//    TextView::EnterText
//
//    Import the specified text into the TextView at the current
//    cursor position, replacing any text-selection in the process
//
ULONG EnterText_TextView(
    TextView * lps,
    LPTSTR szText,
    ULONG nLength
    )
{
    ULONG selstart = min(lps->nSelectionStart, lps->nSelectionEnd);
    ULONG selend = max(lps->nSelectionStart, lps->nSelectionEnd);

    BOOL  fReplaceSelection = (selstart == selend) ? FALSE : TRUE;
    ULONG erase_len = nLength;

    switch (lps->nEditMode)
    {
    case MODE_READONLY:
        return 0;

    case MODE_INSERT:

        // if there is a selection then remove it
        if (fReplaceSelection)
        {
            // group this erase with the insert/replace operation
            group_sequence(lps->pTextDoc->seq);
            erase_text_TextDocument(lps->pTextDoc, selstart, selend - selstart);
            lps->nCursorOffset = selstart;
        }

        if (!insert_text_TextDocument(lps->pTextDoc, lps->nCursorOffset, szText, nLength))
            return 0;

        if (fReplaceSelection)
            ungroup_sequence(lps->pTextDoc->seq);

        break;

    case MODE_OVERWRITE:

        if (fReplaceSelection)
        {
            erase_len = selend - selstart;
            lps->nCursorOffset = selstart;
        }
        else
        {
            ULONG lineoff;
            USPCACHE *uspCache = GetUspCache_TextView(lps, 0, lps->nCurrentLine, &lineoff);

            // single-character overwrite - must behave like 'forward delete'
            // and remove a whole character-cluster (i.e. maybe more than 1 char)
            if (nLength == 1)
            {
                ULONG oldpos = lps->nCursorOffset;
                MoveCharNext_TextView(lps);
                erase_len = lps->nCursorOffset - oldpos;
                lps->nCursorOffset = oldpos;
            }

            // if we are at the end of a line (just before the CRLF) then we must
            // not erase any text - instead we act like a regular insertion
            if (lps->nCursorOffset == lineoff + uspCache->length_CRLF)
                erase_len = 0;


        }

        if (!replace_text_TextDocument(lps->pTextDoc, lps->nCursorOffset, szText, nLength, erase_len))
            return 0;

        break;

    default:
        return 0;
    }

    // update cursor+selection positions
    lps->nCursorOffset += nLength;
    lps->nSelectionStart = lps->nCursorOffset;
    lps->nSelectionEnd = lps->nCursorOffset;

    // we altered the document, recalculate line+scrollbar information
    ResetLineCache_TextView(lps);
    RefreshWindow_TextView(lps);

    Smeg_TextView(lps, TRUE);
    NotifyParent_TextView(lps, TVN_CURSOR_CHANGE);

    return nLength;
}

BOOL ForwardDelete_TextView(
    TextView * lps
    )
{
    ULONG selstart = min(lps->nSelectionStart, lps->nSelectionEnd);
    ULONG selend = max(lps->nSelectionStart, lps->nSelectionEnd);

    if (selstart != selend)
    {
        erase_text_TextDocument(lps->pTextDoc, selstart, selend - selstart);
        lps->nCursorOffset = selstart;

        breakopt_sequence(lps->pTextDoc->seq);
    }
    else
    {
        BYTE tmp[2];
        //USPCACHE        * uspCache;
        //CSCRIPT_LOGATTR * logAttr;
        //ULONG              lineOffset;
        //ULONG              index;

        render_sequence(lps->pTextDoc->seq, lps->nCursorOffset, tmp, 2);

        /*GetLogAttr(lps->nCurrentLine, &uspCache, &logAttr, &lineOffset);

        index = lps->nCursorOffset - lineOffset;

        do
        {
        lps->pTextDoc->seq.erase(lps->nCursorOffset, 1);
        index++;
        }
        while(!logAttr[index].fCharStop);*/

        ULONG oldpos = lps->nCursorOffset;
        MoveCharNext_TextView(lps);

        erase_text_TextDocument(lps->pTextDoc, oldpos, lps->nCursorOffset - oldpos);
        lps->nCursorOffset = oldpos;


        //if(tmp[0] == '\r')
        //    lps->pTextDoc->erase_text(lps->nCursorOffset, 2);
        //else
        //    lps->pTextDoc->erase_text(lps->nCursorOffset, 1);
    }

    lps->nSelectionStart = lps->nCursorOffset;
    lps->nSelectionEnd = lps->nCursorOffset;

    ResetLineCache_TextView(lps);
    RefreshWindow_TextView(lps);
    Smeg_TextView(lps, FALSE);

    return TRUE;
}

BOOL BackDelete_TextView(
    TextView * lps
    )
{
    ULONG selstart = min(lps->nSelectionStart, lps->nSelectionEnd);
    ULONG selend = max(lps->nSelectionStart, lps->nSelectionEnd);

    // if there's a selection then delete it
    if (selstart != selend)
    {
        erase_text_TextDocument(lps->pTextDoc, selstart, selend - selstart);
        lps->nCursorOffset = selstart;
        breakopt_sequence(lps->pTextDoc->seq);
    }
    // otherwise do a back-delete
    else if (lps->nCursorOffset > 0)
    {
        //lps->nCursorOffset--;
        ULONG oldpos = lps->nCursorOffset;
        MoveCharPrev_TextView(lps);
        //lps->pTextDoc->erase_text(lps->nCursorOffset, 1);
        erase_text_TextDocument(lps->pTextDoc, lps->nCursorOffset, oldpos - lps->nCursorOffset);
    }

    lps->nSelectionStart = lps->nCursorOffset;
    lps->nSelectionEnd = lps->nCursorOffset;

    ResetLineCache_TextView(lps);
    RefreshWindow_TextView(lps);
    Smeg_TextView(lps, FALSE);

    return TRUE;
}

VOID Smeg_TextView(
    TextView * lps,
    BOOL fAdvancing
    )
{
    init_linebuffer_TextDocument(lps->pTextDoc);

    lps->nLineCount = linecount_TextDocument(lps->pTextDoc);

    UpdateMetrics_TextView(lps);
    UpdateMarginWidth_TextView(lps);
    SetupScrollbars_TextView(lps);

    UpdateCaretOffset_TextView(lps, lps->nCursorOffset, fAdvancing, &lps->nCaretPosX, &lps->nCurrentLine);

    lps->nAnchorPosX = lps->nCaretPosX;
    ScrollToPosition_TextView(lps, lps->nCaretPosX, lps->nCurrentLine);
    RepositionCaret_TextView(lps);
}

BOOL Undo_TextView(
    TextView * lps
    )
{
    if (lps->nEditMode == MODE_READONLY)
        return FALSE;

    if (!Undo_TextDocument(lps->pTextDoc, &lps->nSelectionStart, &lps->nSelectionEnd))
        return FALSE;

    lps->nCursorOffset = lps->nSelectionEnd;

    ResetLineCache_TextView(lps);
    RefreshWindow_TextView(lps);

    Smeg_TextView(lps, lps->nSelectionStart != lps->nSelectionEnd);

    return TRUE;
}

BOOL Redo_TextView(
    TextView * lps
    )
{
    if (lps->nEditMode == MODE_READONLY)
        return FALSE;

    if (!Redo_TextDocument(lps->pTextDoc, &lps->nSelectionStart, &lps->nSelectionEnd))
        return FALSE;

    lps->nCursorOffset = lps->nSelectionEnd;

    ResetLineCache_TextView(lps);
    RefreshWindow_TextView(lps);
    Smeg_TextView(lps, lps->nSelectionStart != lps->nSelectionEnd);

    return TRUE;
}

BOOL CanUndo_TextView(
    TextView * lps
    )
{
    return canundo_sequence(lps->pTextDoc->seq) ? TRUE : FALSE;
}

BOOL CanRedo_TextView(
    TextView * lps
    )
{
    return canredo_sequence(lps->pTextDoc->seq) ? TRUE : FALSE;
}

LONG OnChar_TextView(
    TextView * lps,
    UINT nChar,
    UINT nFlags
    )
{
    WCHAR ch = (WCHAR) nChar;

    if (nChar < 32 && nChar != '\t' && nChar != '\r' && nChar != '\n')
        return 0;

    // change CR into a CR/LF sequence
    if (nChar == '\r')
        PostMessage(lps->hWnd, WM_CHAR, '\n', 1);

    if (EnterText_TextView(lps, &ch, 1))
    {
        if (nChar == '\n')
            breakopt_sequence(lps->pTextDoc->seq);

        NotifyParent_TextView(lps, TVN_CHANGED);
    }

    return 0;
}
