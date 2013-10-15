//
//    MODULE:        TextView.cpp
//
//    PURPOSE:    Implementation of the TextView control
//
//    NOTES:        www.catch22.net
//
#define _WIN32_WINNT 0x501
#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

// for the EM_xxx message constants
#include <richedit.h>

#include "TextView.h"
#include "TextViewInternal.h"
#include "racursor.h"

#if !defined(UNICODE)
#error "Please build as Unicode only!"
#endif

#if !defined(GetWindowLongPtr)
#error "Latest Platform SDK is required to build Neatpad - try PSDK-Feb-2003
#endif

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "UspLib.lib")

//
//    Constructor for TextView class
//
TextView * new_TextView(
    HWND hWnd
    )
{
    TextView * lps = (TextView *) malloc(sizeof(TextView));

    if (lps)
    {
        memset(lps, 0, sizeof(*lps));

        lps->hWnd = hWnd;

        lps->hTheme = OpenThemeData(hWnd, L"edit");

        // Font-related data
        lps->nNumFonts = 1;
        lps->nHeightAbove = 0;
        lps->nHeightBelow = 0;

        // File-related data
        lps->nLineCount = 0;
        lps->nLongestLine = 0;


        // Scrollbar related data
        lps->nVScrollPos = 0;
        lps->nHScrollPos = 0;
        lps->nVScrollMax = 0;
        lps->nHScrollMax = 0;

        // Display-related data
        lps->nTabWidthChars = 4;
        lps->uStyleFlags = 0;
        lps->nCaretWidth = 0;
        lps->nLongLineLimit = 80;
        lps->nLineInfoCount = 0;
        lps->nCRLFMode = TXL_CRLF;  //ALL;

        // allocate the USPDATA cache
        lps->uspCache = (USPCACHE *) malloc(sizeof(USPCACHE) * USP_CACHE_SIZE);

        for (int i = 0; i < USP_CACHE_SIZE; i++)
        {
            lps->uspCache[i].usage = 0;
            lps->uspCache[i].lineno = 0;
            lps->uspCache[i].uspData = UspAllocate();
        }

        SystemParametersInfo(SPI_GETCARETWIDTH, 0, &lps->nCaretWidth, 0);

        if (lps->nCaretWidth == 0)
        {
            lps->nCaretWidth = 2;
        }

        // Default display colours
        lps->rgbColourList[TXC_FOREGROUND] = SYSCOL(COLOR_WINDOWTEXT);
        lps->rgbColourList[TXC_BACKGROUND] = SYSCOL(COLOR_WINDOW);          // RGB(34,54,106)
        lps->rgbColourList[TXC_HIGHLIGHTTEXT] = SYSCOL(COLOR_HIGHLIGHTTEXT);
        lps->rgbColourList[TXC_HIGHLIGHT] = SYSCOL(COLOR_HIGHLIGHT);
        lps->rgbColourList[TXC_HIGHLIGHTTEXT2] = SYSCOL(COLOR_WINDOWTEXT);  //INACTIVECAPTIONTEXT);
        lps->rgbColourList[TXC_HIGHLIGHT2] = SYSCOL(COLOR_3DFACE);          //INACTIVECAPTION);
        lps->rgbColourList[TXC_SELMARGIN1] = SYSCOL(COLOR_3DFACE);
        lps->rgbColourList[TXC_SELMARGIN2] = SYSCOL(COLOR_3DHIGHLIGHT);
        lps->rgbColourList[TXC_LINENUMBERTEXT] = SYSCOL(COLOR_3DSHADOW);
        lps->rgbColourList[TXC_LINENUMBER] = SYSCOL(COLOR_3DFACE);
        lps->rgbColourList[TXC_LONGLINETEXT] = SYSCOL(COLOR_3DSHADOW);
        lps->rgbColourList[TXC_LONGLINE] = SYSCOL(COLOR_3DFACE);
        lps->rgbColourList[TXC_CURRENTLINETEXT] = SYSCOL(COLOR_WINDOWTEXT);
        lps->rgbColourList[TXC_CURRENTLINE] = RGB(230, 240, 255);


        // Runtime data
        lps->nSelectionMode = SEL_NONE;
        lps->nEditMode = MODE_INSERT;
        lps->nScrollTimer = 0;
        lps->fHideCaret = false;
        lps->hUserMenu = 0;
        lps->hImageList = 0;

        lps->nSelectionStart = 0;
        lps->nSelectionEnd = 0;
        lps->nSelectionType = SEL_NONE;
        lps->nCursorOffset = 0;
        lps->nCurrentLine = 0;

        lps->nLinenoWidth = 0;
        lps->nCaretPosX = 0;
        lps->nAnchorPosX = 0;

        //SetRect(&lps->rcBorder, 2, 2, 2, 2);

        lps->pTextDoc = new_TextDocument();

        lps->hMarginCursor = CreateCursor(GetModuleHandle(0), 21, 5, 32, 32, XORMask, ANDMask);

        //
        //    The TextView state must be fully initialized before we
        //    start calling member-functions
        //

        memset(lps->uspFontList, 0, sizeof(lps->uspFontList));

        // Set the default font
        OnSetFont_TextView(lps, (HFONT) GetStockObject(ANSI_FIXED_FONT));

        UpdateMetrics_TextView(lps);
        UpdateMarginWidth_TextView(lps);
    }

    return lps;
}

//
//    Destructor for TextView class
//
void delete_TextView(
    TextView * lps
    )
{
    if (lps->pTextDoc)
    {
        delete_TextDocument(lps->pTextDoc);
    }

    DestroyCursor(lps->hMarginCursor);

    for (int i = 0; i < USP_CACHE_SIZE; i++)
    {
        UspFree(lps->uspCache[i].uspData);
    }
    free(lps->uspCache);

    CloseThemeData(lps->hTheme);

    free(lps);
}

ULONG NotifyParent_TextView(
    TextView * lps,
    UINT nNotifyCode,
    NMHDR * optional
    )
{
    UINT nCtrlId = GetWindowLong(lps->hWnd, GWL_ID);
    NMHDR nmhdr = { lps->hWnd, nCtrlId, nNotifyCode };
    NMHDR * nmptr = &nmhdr;

    if (optional)
    {
        nmptr = optional;
        *nmptr = nmhdr;
    }

    return SendMessage(GetParent(lps->hWnd), WM_NOTIFY, (WPARAM) nCtrlId, (LPARAM) nmptr);
}

VOID UpdateMetrics_TextView(
    TextView * lps
    )
{
    RECT rect;
    GetClientRect(lps->hWnd, &rect);

    OnSize_TextView(lps, 0, rect.right, rect.bottom);
    RefreshWindow_TextView(lps);

    RepositionCaret_TextView(lps);
}

LONG OnSetFocus_TextView(
    TextView * lps,
    HWND hwndOld
    )
{
    CreateCaret(lps->hWnd, (HBITMAP) NULL, lps->nCaretWidth, lps->nLineHeight);
    RepositionCaret_TextView(lps);

    ShowCaret(lps->hWnd);
    RefreshWindow_TextView(lps);
    return 0;
}

LONG OnKillFocus_TextView(
    TextView * lps,
    HWND hwndNew
    )
{
    // if we are making a selection when we lost focus then
    // stop the selection logic
    if (lps->nSelectionMode != SEL_NONE)
    {
        OnLButtonUp_TextView(lps, 0, 0, 0);
    }

    HideCaret(lps->hWnd);
    DestroyCaret();
    RefreshWindow_TextView(lps);
    return 0;
}

ULONG SetStyle_TextView(
    TextView * lps,
    ULONG uMask,
    ULONG uStyles
    )
{
    ULONG oldstyle = lps->uStyleFlags;

    lps->uStyleFlags = (lps->uStyleFlags & ~uMask) | uStyles;

    ResetLineCache_TextView(lps);

    // update display here
    UpdateMetrics_TextView(lps);
    RefreshWindow_TextView(lps);

    return oldstyle;
}

ULONG SetVar_TextView(
    TextView * lps,
    ULONG nVar,
    ULONG nValue
    )
{
    return 0;//oldval;
}

ULONG GetVar_TextView(
    TextView * lps,
    ULONG nVar
    )
{
    return 0;
}

ULONG GetStyleMask_TextView(
    TextView * lps,
    ULONG uMask
    )
{
    return lps->uStyleFlags & uMask;
}

bool CheckStyle_TextView(
    TextView * lps,
    ULONG uMask
    )
{
    return (lps->uStyleFlags & uMask) ? true : false;
}

int SetCaretWidth_TextView(
    TextView * lps,
    int nWidth
    )
{
    int oldwidth = lps->nCaretWidth;
    lps->nCaretWidth = nWidth;

    return oldwidth;
}

BOOL SetImageList_TextView(
    TextView * lps,
    HIMAGELIST hImgList
    )
{
    lps->hImageList = hImgList;
    return TRUE;
}

LONG SetLongLine_TextView(
    TextView * lps,
    int nLength
    )
{
    int oldlen = lps->nLongLineLimit;
    lps->nLongLineLimit = nLength;
    return oldlen;
}

int CompareLineInfo(LINEINFO *elem1, LINEINFO *elem2)
{
    if (elem1->nLineNo < elem2->nLineNo)
        return -1;
    if (elem1->nLineNo > elem2->nLineNo)
        return 1;
    else
        return 0;
}

int SetLineImage_TextView(
    TextView * lps,
    ULONG nLineNo,
    ULONG nImageIdx
    )
{
    LINEINFO *linfo = GetLineInfo_TextView(lps, nLineNo);

    // if already a line with an image
    if (linfo)
    {
        linfo->nImageIdx = nImageIdx;
    }
    else
    {
        linfo = &lps->LineInfo[lps->nLineInfoCount++];
        linfo->nLineNo = nLineNo;
        linfo->nImageIdx = nImageIdx;

        // sort the array
        qsort(
            lps->LineInfo,
            lps->nLineInfoCount,
            sizeof(LINEINFO),
            (COMPAREPROC) CompareLineInfo
            );

    }
    return 0;
}

LINEINFO * GetLineInfo_TextView(
    TextView * lps,
    ULONG nLineNo
    )
{
    LINEINFO key = { nLineNo, 0 };

    // perform the binary search
    return (LINEINFO *) bsearch(
        &key,
        lps->LineInfo,
        lps->nLineInfoCount,
        sizeof(LINEINFO),
        (COMPAREPROC) CompareLineInfo
        );
}

ULONG SelectionSize_TextView(
    TextView * lps
    )
{
    ULONG s1 = min(lps->nSelectionStart, lps->nSelectionEnd);
    ULONG s2 = max(lps->nSelectionStart, lps->nSelectionEnd);
    return s2 - s1;
}

ULONG SelectAll_TextView(
    TextView * lps
    )
{
    lps->nSelectionStart = 0;
    lps->nSelectionEnd = size_TextDocument(lps->pTextDoc);
    lps->nCursorOffset = lps->nSelectionEnd;

    Smeg_TextView(lps, TRUE);
    RefreshWindow_TextView(lps);
    return 0;
}

//
//    Public memberfunction 
//
LONG WINAPI WndProc_TextView(
    TextView * lps,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg)
    {
        // Draw contents of TextView whenever window needs updating
    case WM_ERASEBKGND:
        return 1;

        // Need to custom-draw the border for XP/Vista themes
    case WM_NCPAINT:
        return OnNcPaint_TextView(lps, (HRGN) wParam);

    case WM_PAINT:
        return OnPaint_TextView(lps);

        // Set a new font 
    case WM_SETFONT:
        return OnSetFont_TextView(lps, (HFONT) wParam);

    case WM_SIZE:
        return OnSize_TextView(lps, wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_VSCROLL:
        return OnVScroll_TextView(lps, LOWORD(wParam), HIWORD(wParam));

    case WM_HSCROLL:
        return OnHScroll_TextView(lps, LOWORD(wParam), HIWORD(wParam));

    case WM_MOUSEACTIVATE:
        return OnMouseActivate_TextView(lps, (HWND) wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_CONTEXTMENU:
        return OnContextMenu_TextView(lps, (HWND) wParam, (short) LOWORD(lParam), (short) HIWORD(lParam));

    case WM_MOUSEWHEEL:
        return OnMouseWheel_TextView(lps, (short) HIWORD(wParam));

    case WM_SETFOCUS:
        return OnSetFocus_TextView(lps, (HWND) wParam);

    case WM_KILLFOCUS:
        return OnKillFocus_TextView(lps, (HWND) wParam);

        // make sure we get arrow-keys, enter, tab, etc when hosted inside a dialog
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;

    case WM_LBUTTONDOWN:
        return OnLButtonDown_TextView(lps, wParam, (short) LOWORD(lParam), (short) HIWORD(lParam));

    case WM_LBUTTONUP:
        return OnLButtonUp_TextView(lps, wParam, (short) LOWORD(lParam), (short) HIWORD(lParam));

    case WM_LBUTTONDBLCLK:
        return OnLButtonDblClick_TextView(lps, wParam, (short) LOWORD(lParam), (short) HIWORD(lParam));

    case WM_MOUSEMOVE:
        return OnMouseMove_TextView(lps, wParam, (short) LOWORD(lParam), (short) HIWORD(lParam));

    case WM_KEYDOWN:
        return OnKeyDown_TextView(lps, wParam, lParam);

    case WM_UNDO: case TXM_UNDO: case EM_UNDO:
        return Undo_TextView(lps);

    case TXM_REDO: case EM_REDO:
        return Redo_TextView(lps);

    case TXM_CANUNDO: case EM_CANUNDO:
        return CanUndo_TextView(lps);

    case TXM_CANREDO: case EM_CANREDO:
        return CanRedo_TextView(lps);

    case WM_CHAR:
        return OnChar_TextView(lps, wParam, lParam);

    case WM_SETCURSOR:

        if (LOWORD(lParam) == HTCLIENT)
            return TRUE;

        break;

    case WM_COPY:
        return OnCopy_TextView(lps);

    case WM_CUT:
        return OnCut_TextView(lps);

    case WM_PASTE:
        return OnPaste_TextView(lps);

    case WM_CLEAR:
        return OnClear_TextView(lps);

    case WM_GETTEXT:
        return 0;

    case WM_TIMER:
        return OnTimer_TextView(lps, wParam);

        //
    case TXM_OPENFILE:
        return OpenFile_TextView(lps, (LPTSTR) lParam);

    case TXM_CLEAR:
        return ClearFile_TextView(lps);

    case TXM_SETLINESPACING:
        return SetLineSpacing_TextView(lps, wParam, lParam);

    case TXM_ADDFONT:
        return AddFont_TextView(lps, (HFONT) wParam);

    case TXM_SETCOLOR:
        return SetColour_TextView(lps, wParam, lParam);

    case TXM_SETSTYLE:
        return SetStyle_TextView(lps, wParam, lParam);

    case TXM_SETCARETWIDTH:
        return SetCaretWidth_TextView(lps, wParam);

    case TXM_SETIMAGELIST:
        return SetImageList_TextView(lps, (HIMAGELIST) wParam);

    case TXM_SETLONGLINE:
        return SetLongLine_TextView(lps, lParam);

    case TXM_SETLINEIMAGE:
        return SetLineImage_TextView(lps, wParam, lParam);

    case TXM_GETFORMAT:
        return getformat_TextDocument(lps->pTextDoc);

    case TXM_GETSELSIZE:
        return SelectionSize_TextView(lps);

    case TXM_SETSELALL:
        return SelectAll_TextView(lps);

    case TXM_GETCURPOS:
        return lps->nCursorOffset;

    case TXM_GETCURLINE:
        return lps->nCurrentLine;

    case TXM_GETCURCOL:
        ULONG nOffset;
        GetUspData_TextView(lps, 0, lps->nCurrentLine, &nOffset);
        return lps->nCursorOffset - nOffset;

    case TXM_GETEDITMODE:
        return lps->nEditMode;

    case TXM_SETEDITMODE:
        lParam = lps->nEditMode;
        lps->nEditMode = wParam;
        return lParam;

    case TXM_SETCONTEXTMENU:
        lps->hUserMenu = (HMENU) wParam;
        return 0;

    default:
        break;
    }

    return DefWindowProc(lps->hWnd, msg, wParam, lParam);
}

//
//    Win32 TextView window procedure stub
//
LRESULT WINAPI TextViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TextView *ptv = (TextView *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg)
    {
        // First message received by any window - make a new TextView object
        // and store pointer to it in our extra-window-bytes
    case WM_NCCREATE:

        if ((ptv = new_TextView(hwnd)) == 0)
            return FALSE;

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG) ptv);
        return TRUE;

        // Last message received by any window - delete the TextView object
    case WM_NCDESTROY:
        delete_TextView(ptv);
        SetWindowLongPtr(hwnd, 0, 0);
        return 0;

        // Pass everything to the TextView window procedure
    default:
        if (ptv)
            return WndProc_TextView(ptv, msg, wParam, lParam);
        else
            return 0;
    }
}

//
//    Register the TextView window class
//
BOOL InitTextView()
{
    WNDCLASSEX    wcx;

    //Window class for the main application parent window
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_DBLCLKS;
    wcx.lpfnWndProc = TextViewWndProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = sizeof(TextView *);
    wcx.hInstance = GetModuleHandle(0);
    wcx.hIcon = 0;
    wcx.hCursor = LoadCursor(NULL, IDC_IBEAM);
    wcx.hbrBackground = (HBRUSH) 0;        //NO FLICKERING FOR US!!
    wcx.lpszMenuName = 0;
    wcx.lpszClassName = TEXTVIEW_CLASS;
    wcx.hIconSm = 0;

    return RegisterClassEx(&wcx) ? TRUE : FALSE;
}

//
//    Create a TextView control!
//
HWND CreateTextView(HWND hwndParent)
{
    return CreateWindowEx(WS_EX_CLIENTEDGE,
        //        L"EDIT", L"",
        TEXTVIEW_CLASS, TEXT(""),
        WS_VSCROLL | WS_HSCROLL | WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        hwndParent,
        0,
        GetModuleHandle(0),
        0);
}

BOOL WINAPI DllMain(
    _In_    HINSTANCE hinstDLL,
    _In_    DWORD fdwReason,
    _In_    LPVOID lpvReserved
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}
