#ifndef TEXTVIEW_INTERNAL_INCLUDED
#define TEXTVIEW_INTERNAL_INCLUDED

#define TEXTBUFSIZE     128
#define LINENO_FMT      TEXT("%d")
#define LINENO_PAD      8

#include <commctrl.h>
#include <uxtheme.h>

/*
HTHEME  (WINAPI * OpenThemeData_Proc)(HWND hwnd, LPCWSTR pszClassList);
BOOL    (WINAPI * CloseThemeData_Proc)(HTHEME hTheme);
HRESULT (WINAPI * DrawThemeBackground_Proc)(HTHEME hTheme, HDC hdc, int, int, const RECT*, const RECT*);
*/


#include "TextDocument.h"

#include "UspLib\usplib.h"

typedef struct
{
    LPUSPDATA   uspData;
    ULONG       lineno;         // line#
    ULONG       offset;         // offset (in WCHAR's) of this line
    ULONG       usage;          // cache-count

    int         length;         // length in chars INCLUDING CR/LF
    int         length_CRLF;    // length in chars EXCLUDING CR/LF

} USPCACHE, *LPUSPCACHE;

typedef const SCRIPT_LOGATTR CSCRIPT_LOGATTR;

#define USP_CACHE_SIZE  200

//
//    LINEINFO - information about a specific line
//
typedef struct
{
    ULONG   nLineNo;
    int     nImageIdx;

    // more here in the future?

} LINEINFO, *LPLINEINFO;

typedef int(__cdecl * COMPAREPROC) (const VOID *, const VOID *);

// maximum number of lines that we can hold info for at one time
#define MAX_LINE_INFO   128    

// maximum fonts that a TextView can hold
#define MAX_FONTS   32

enum SELMODE { SEL_NONE, SEL_NORMAL, SEL_MARGIN, SEL_BLOCK };

typedef struct
{
    ULONG   line;
    ULONG   xpos;

} CURPOS, *LPCURPOS;

//
//    TextView - internal window implementation
//
struct _TextView;
typedef struct _TextView TextView;

TextView * new_TextView(
    HWND hWnd
    );
VOID delete_TextView(
    TextView * lps
    );
ULONG NotifyParent_TextView(
    TextView * lps,
    UINT nNotifyCode,
    NMHDR * optional = 0
    );
VOID UpdateMetrics_TextView(
    TextView * lps
    );
LONG OnSetFocus_TextView(
    TextView * lps,
    HWND hwndOld
    );
LONG OnKillFocus_TextView(
    TextView * lps,
    HWND hwndNew
    );
ULONG SetStyle_TextView(
    TextView * lps,
    ULONG uMask,
    ULONG uStyles
    );
ULONG SetVar_TextView(
    TextView * lps,
    ULONG nVar,
    ULONG nValue
    );
ULONG GetVar_TextView(
    TextView * lps,
    ULONG nVar
    );
ULONG GetStyleMask_TextView(
    TextView * lps,
    ULONG uMask
    );
bool CheckStyle_TextView(
    TextView * lps,
    ULONG uMask
    );
int SetCaretWidth_TextView(
    TextView * lps,
    int nWidth
    );
BOOL SetImageList_TextView(
    TextView * lps,
    HIMAGELIST hImgList
    );
LONG SetLongLine_TextView(
    TextView * lps,
    int nLength
    );
int SetLineImage_TextView(
    TextView * lps,
    ULONG nLineNo,
    ULONG nImageIdx
    );
LINEINFO * GetLineInfo_TextView(
    TextView * lps,
    ULONG nLineNo
    );
ULONG SelectionSize_TextView(
    TextView * lps
    );
ULONG SelectAll_TextView(
    TextView * lps
    );
//TextViewFont
int NeatTextYOffset_TextView(
    TextView * lps,
    USPFONT * font
    );
int TextWidth_TextView(
    TextView * lps,
    HDC hdc,
    LPCTSTR lpString,
    int c
    );
VOID RecalcLineHeight_TextView(
    TextView * lps
    );
LONG SetFont_TextView(
    TextView * lps,
    HFONT hFont,
    int idx
    );
LONG AddFont_TextView(
    TextView * lps,
    HFONT hFont
    );
LONG OnSetFont_TextView(
    TextView * lps,
    HFONT hFont
    );
LONG SetLineSpacing_TextView(
    TextView * lps,
    int nAbove,
    int nBelow
    );
//TextViewPaint
LONG OnNcPaint_TextView(
    TextView * lps,
    HRGN hrgnUpdate
    );
VOID RefreshWindow_TextView(
    TextView * lps
    );
USPCACHE * GetUspCache_TextView(
    TextView * lps,
    HDC hdc,
    ULONG nLineNo,
    ULONG * nOffset = 0
    );
USPDATA * GetUspData_TextView(
    TextView * lps,
    HDC hdc,
    ULONG nLineNo,
    ULONG * nOffset = 0
    );
VOID ResetLineCache_TextView(
    TextView * lps
    );
LONG OnPaint_TextView(
    TextView * lps
    );
VOID PaintLine_TextView(
    TextView * lps,
    HDC hdc,
    ULONG nLineNo,
    int xpos,
    int ypos,
    HRGN hrgnUpdate
    );
int LeftMarginWidth_TextView(
    TextView * lps
    );
VOID UpdateMarginWidth_TextView(
    TextView * lps
    );
int PaintMargin_TextView(
    TextView * lps,
    HDC hdc,
    ULONG nLineNo,
    int xpos,
    int ypos
    );
VOID PaintText_TextView(
    TextView * lps,
    HDC hdc,
    ULONG nLineNo,
    int xpos,
    int ypos,
    RECT * bounds
    );
int ApplySelection_TextView(
    TextView * lps,
    USPDATA * uspData,
    ULONG nLine,
    ULONG nOffset,
    ULONG nTextLen
    );
int ApplyTextAttributes_TextView(
    TextView * lps,
    ULONG nLineNo,
    ULONG nOffset,
    ULONG & nColumn,
    LPTSTR szText,
    int nTextLen,
    ATTR * attr
    );
int CRLF_size_TextView(
    TextView * lps,
    LPTSTR szText,
    int nLength
    );
VOID MarkCRLF_TextView(
    TextView * lps,
    USPDATA * uspData,
    LPTSTR szText,
    int nLength,
    ATTR * attr
    );
int StripCRLF_TextView(
    TextView * lps,
    LPTSTR szText,
    ATTR * attr,
    int nLength,
    bool fAllow
    );
COLORREF LineColour_TextView(
    TextView * lps,
    ULONG nLineNo
    );
COLORREF LongColour_TextView(
    TextView * lps,
    ULONG nLineNo
    );
COLORREF GetColour_TextView(
    TextView * lps,
    UINT idx
    );
COLORREF SetColour_TextView(
    TextView * lps,
    UINT idx,
    COLORREF rgbColour
    );
//TextViewScroll
VOID SetupScrollbars_TextView(
    TextView * lps
    );
bool PinToBottomCorner_TextView(
    TextView * lps
    );
LONG OnSize_TextView(
    TextView * lps,
    UINT nFlags,
    int width,
    int height
    );
HRGN ScrollRgn_TextView(
    TextView * lps,
    int dx,
    int dy,
    bool fReturnUpdateRgn
    );
VOID Scroll_TextView(
    TextView * lps,
    int dx,
    int dy
    );
VOID ScrollToPosition_TextView(
    TextView * lps,
    int xpos,
    ULONG lineno
    );
VOID ScrollToCaret_TextView(
    TextView * lps
    );
LONG OnVScroll_TextView(
    TextView * lps,
    UINT nSBCode,
    UINT nPos
    );
LONG OnHScroll_TextView(
    TextView * lps,
    UINT nSBCode,
    UINT nPos
    );
LONG OnMouseWheel_TextView(
    TextView * lps,
    int nDelta
    );
//TextViewMouse
LONG OnMouseActivate_TextView(
    TextView * lps,
    HWND hwndTop,
    UINT nHitTest,
    UINT nMessage
    );
HMENU CreateContextMenu_TextView(
    TextView * lps
    );
LONG OnContextMenu_TextView(
    TextView * lps,
    HWND hwndParam,
    int x,
    int y
    );
LONG OnLButtonDown_TextView(
    TextView * lps,
    UINT nFlags,
    int mx,
    int my
    );
LONG OnLButtonUp_TextView(
    TextView * lps,
    UINT nFlags,
    int mx,
    int my
    );
LONG OnLButtonDblClick_TextView(
    TextView * lps,
    UINT nFlags,
    int mx,
    int my
    );
LONG OnMouseMove_TextView(
    TextView * lps,
    UINT nFlags,
    int mx,
    int my
    );
LONG OnTimer_TextView(
    TextView * lps,
    UINT nTimerId
    );
BOOL MouseCoordToFilePos_TextView(
    TextView * lps,
    int mx,
    int my,
    ULONG * pnLineNo,
    ULONG * pnFileOffset,
    int * psnappedX
    );
LONG InvalidateLine_TextView(
    TextView * lps,
    ULONG nLineNo,
    bool forceAnalysis
    );
LONG InvalidateRange_TextView(
    TextView * lps,
    ULONG nStart,
    ULONG nFinish
    );
VOID MoveCaret_TextView(
    TextView * lps,
    int x,
    int y
    );
VOID UpdateCaretXY_TextView(
    TextView * lps,
    int xpos,
    ULONG lineno
    );
VOID UpdateCaretOffset_TextView(
    TextView * lps,
    ULONG offset,
    BOOL fTrailing,
    int * outx = 0,
    ULONG * outlineno = 0
    );
VOID RepositionCaret_TextView(
    TextView * lps
    );
VOID UpdateLine_TextView(
    TextView * lps,
    ULONG nLineNo
    );
//TextViewKeyInput
ULONG EnterText_TextView(
    TextView * lps,
    LPTSTR szText,
    ULONG nLength
    );
BOOL ForwardDelete_TextView(
    TextView * lps
    );
BOOL BackDelete_TextView(
    TextView * lps
    );
VOID Smeg_TextView(
    TextView * lps,
    BOOL fAdvancing
    );
BOOL Undo_TextView(
    TextView * lps
    );
BOOL Redo_TextView(
    TextView * lps
    );
BOOL CanUndo_TextView(
    TextView * lps
    );
BOOL CanRedo_TextView(
    TextView * lps
    );
LONG OnChar_TextView(
    TextView * lps,
    UINT nChar,
    UINT nFlags
    );
ULONG EnterText_TextView(
    TextView * lps,
    LPTSTR szText,
    ULONG nLength
    );
BOOL ForwardDelete_TextView(
    TextView * lps
    );
BOOL BackDelete_TextView(
    TextView * lps
    );
VOID Smeg_TextView(
    TextView * lps,
    BOOL fAdvancing
    );
BOOL Undo_TextView(
    TextView * lps
    );
BOOL Redo_TextView(
    TextView * lps
    );
BOOL CanUndo_TextView(
    TextView * lps
    );
BOOL CanRedo_TextView(
    TextView * lps
    );
LONG OnChar_TextView(
    TextView * lps,
    UINT nChar,
    UINT nFlags
    );
//TextViewKeyNav
bool GetLogAttr_TextView(
    TextView * lps,
    ULONG nLineNo,
    USPCACHE * *puspCache,
    CSCRIPT_LOGATTR * *plogAttr = 0,
    ULONG * pnOffset = 0
    );
VOID MoveLineUp_TextView(
    TextView * lps,
    int numLines
    );
VOID MoveLineDown_TextView(
    TextView * lps,
    int numLines
    );
VOID MoveWordPrev_TextView(
    TextView * lps
    );
VOID MoveWordNext_TextView(
    TextView * lps
    );
VOID MoveWordStart_TextView(
    TextView * lps
    );
VOID MoveWordEnd_TextView(
    TextView * lps
    );
VOID MoveCharPrev_TextView(
    TextView * lps
    );
VOID MoveCharNext_TextView(
    TextView * lps
    );
VOID MoveLineStart_TextView(
    TextView * lps,
    ULONG lineNo
    );
VOID MoveLineEnd_TextView(
    TextView * lps,
    ULONG lineNo
    );
VOID MoveFileStart_TextView(
    TextView * lps
    );
VOID MoveFileEnd_TextView(
    TextView * lps
    );
LONG OnKeyDown_TextView(
    TextView * lps,
    UINT nKeyCode,
    UINT nFlags
    );
//TextViewClipboard
BOOL OnPaste_TextView(
    TextView * lps
    );
ULONG GetText_TextView(
    TextView * lps,
    LPTSTR szDest,
    ULONG nStartOffset,
    ULONG nLength
    );
BOOL OnCopy_TextView(
    TextView * lps
    );
BOOL OnCut_TextView(
    TextView * lps
    );
BOOL OnClear_TextView(
    TextView * lps
    );
//TextViewFile
LONG OpenFile_TextView(
    TextView * lps,
    LPTSTR szFileName
    );
LONG ClearFile_TextView(
    TextView * lps
    );
//TextViewSyntax
int SyntaxColour_TextView(
    TextView * lps,
    LPTSTR szText,
    ULONG nTextLen,
    ATTR * attr
    );
struct _TextView
{
    //
    // ------ Internal TextView State ------
    //
    HWND hWnd;
    HTHEME hTheme;
    ULONG uStyleFlags;

    // File-related data
    ULONG nLineCount;

    // Font-related data    
    USPFONT uspFontList[MAX_FONTS];
    int nNumFonts;
    int nFontWidth;
    int nMaxAscent;
    int nLineHeight;
    int nHeightAbove;
    int nHeightBelow;

    // Scrollbar-related data
    ULONG nVScrollPos;
    ULONG nVScrollMax;
    int nHScrollPos;
    int nHScrollMax;

    int nLongestLine;
    int nWindowLines;
    int nWindowColumns;

    // Cursor/Caret position 
    ULONG nCurrentLine;
    ULONG nSelectionStart;
    ULONG nSelectionEnd;
    ULONG nCursorOffset;
    ULONG nSelMarginOffset1;
    ULONG nSelMarginOffset2;
    int nCaretPosX;
    int nAnchorPosX;

    SELMODE nSelectionMode;
    SELMODE nSelectionType;
    CURPOS cpBlockStart;
    CURPOS cpBlockEnd;
    UINT nEditMode;

    // Display-related data
    int nTabWidthChars;
    DWORD nCaretWidth;
    int nLongLineLimit;
    int nCRLFMode;

    LINEINFO LineInfo[MAX_LINE_INFO];
    int nLineInfoCount;

    // Margin information
    int nLinenoWidth;
    HCURSOR hMarginCursor;
    //RECT rcBorder;

    COLORREF rgbColourList[TXC_MAX_COLOURS];

    // Runtime data
    UINT nScrollTimer;
    int nScrollCounter;
    bool fHideCaret;
    //bool fTransparent;
    HIMAGELIST hImageList;
    HMENU hUserMenu;

    // Cache for USPDATA objects
    USPCACHE * uspCache;

    TextDocument * pTextDoc;
};

#endif