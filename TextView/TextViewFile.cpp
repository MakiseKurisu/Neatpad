//
//    MODULE:        TextViewFile.cpp
//
//    PURPOSE:    TextView file input routines
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
//    
//
LONG OpenFile_TextView(
    TextView * lps,
    LPTSTR szFileName
    )
{
    ClearFile_TextView(lps);

    if (init_TextDocument(lps->pTextDoc, szFileName))
    {
        lps->nLineCount = linecount_TextDocument(lps->pTextDoc);
        lps->nLongestLine = longestline_TextDocument(lps->pTextDoc, lps->nTabWidthChars);

        lps->nVScrollPos = 0;
        lps->nHScrollPos = 0;

        lps->nSelectionStart = 0;
        lps->nSelectionEnd = 0;
        lps->nCursorOffset = 0;

        UpdateMarginWidth_TextView(lps);
        UpdateMetrics_TextView(lps);
        ResetLineCache_TextView(lps);
        return TRUE;
    }

    return FALSE;
}

//
//
//
LONG ClearFile_TextView(
    TextView * lps
    )
{
    if (lps->pTextDoc)
    {
        clear_TextDocument(lps->pTextDoc);
        EmptyDoc_TextDocument(lps->pTextDoc);
    }

    ResetLineCache_TextView(lps);

    lps->nLineCount = linecount_TextDocument(lps->pTextDoc);
    lps->nLongestLine = longestline_TextDocument(lps->pTextDoc, lps->nTabWidthChars);

    lps->nVScrollPos = 0;
    lps->nHScrollPos = 0;

    lps->nSelectionStart = 0;
    lps->nSelectionEnd = 0;
    lps->nCursorOffset = 0;

    lps->nCurrentLine = 0;
    lps->nCaretPosX = 0;

    UpdateMetrics_TextView(lps);

    return TRUE;
}
