//
//    MODULE:        TextViewClipboard.cpp
//
//    PURPOSE:    Basic clipboard support for TextView
//                Just uses GetClipboardData/SetClipboardData until I migrate
//                to the OLE code from my drag+drop tutorials
//
//    NOTES:        www.catch22.net
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include "TextView.h"
#include "TextViewInternal.h"

#ifdef UNICODE
#define CF_TCHARTEXT CF_UNICODETEXT
#else
#define CF_TCHARTEXT CF_TEXT
#endif

//
//    Paste any CF_TEXT/CF_UNICODE text from the clipboard
//
BOOL TextView::OnPaste()
{
    BOOL success = FALSE;

    if (m_nEditMode == MODE_READONLY)
        return FALSE;

    if (OpenClipboard(m_hWnd))
    {
        HANDLE hMem = GetClipboardData(CF_TCHARTEXT);
        LPTSTR szText = (LPTSTR) GlobalLock(hMem);

        if (szText)
        {
            ULONG textlen = lstrlen(szText);
            EnterText(szText, textlen);

            if (textlen > 1)
                breakopt_sequence(m_pTextDoc->seq);

            GlobalUnlock(hMem);

            success = TRUE;
        }

        CloseClipboard();
    }

    return success;
}

//
//    Retrieve the specified range of text and copy it to supplied buffer
//    szDest must be big enough to hold nLength characters
//    nLength includes the terminating NULL
//
ULONG TextView::GetText(LPTSTR szDest, ULONG nStartOffset, ULONG nLength)
{
    ULONG copied = 0;

    if (nLength > 1)
    {
        TextIterator * itor = new_TextIterator(iterate_TextDocument(m_pTextDoc, nStartOffset));
        copied = gettext_TextIterator(itor, szDest, nLength - 1);
        delete_TextIterator(itor);
        // null-terminate
        szDest[copied] = 0;
    }

    return copied;
}

//
//    Copy the currently selected text to the clipboard as CF_TEXT/CF_UNICODE
//
BOOL TextView::OnCopy()
{
    ULONG    selstart = min(m_nSelectionStart, m_nSelectionEnd);
    ULONG    sellen = SelectionSize();
    BOOL    success = FALSE;

    if (sellen == 0)
        return FALSE;

    if (OpenClipboard(m_hWnd))
    {
        HANDLE hMem;
        TCHAR  *ptr;

        if ((hMem = GlobalAlloc(GPTR, (sellen + 1) * sizeof(TCHAR))) != 0)
        {
            if ((ptr = (LPTSTR) GlobalLock(hMem)) != 0)
            {
                EmptyClipboard();

                GetText(ptr, selstart, sellen + 1);

                SetClipboardData(CF_TCHARTEXT, hMem);
                success = TRUE;

                GlobalUnlock(hMem);
            }
        }

        CloseClipboard();
    }

    return success;
}

//
//    Remove current selection and copy to the clipboard
//
BOOL TextView::OnCut()
{
    BOOL success = FALSE;

    if (m_nEditMode == MODE_READONLY)
        return FALSE;

    if (SelectionSize() > 0)
    {
        // copy selected text to clipboard then erase current selection
        success = OnCopy();
        success = success && ForwardDelete();
    }

    return success;
}

//
//    Remove the current selection
//
BOOL TextView::OnClear()
{
    BOOL success = FALSE;

    if (m_nEditMode == MODE_READONLY)
        return FALSE;

    if (SelectionSize() > 0)
    {
        ForwardDelete();
        success = TRUE;
    }

    return success;
}

