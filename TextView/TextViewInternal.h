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

typedef int(__cdecl * COMPAREPROC) (const void *, const void *);

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
class TextView
{
public:

    TextView(HWND hwnd);
    ~TextView(void);

    LONG WINAPI WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

private:

    //
    //    Message handlers
    //
    LONG OnPaint(void);
    LONG OnNcPaint(HRGN hrgnUpdate);
    LONG OnSetFont(HFONT hFont);
    LONG OnSize(UINT nFlags, int width, int height);
    LONG OnVScroll(UINT nSBCode, UINT nPos);
    LONG OnHScroll(UINT nSBCode, UINT nPos);
    LONG OnMouseWheel(int nDelta);
    LONG OnTimer(UINT nTimer);

    LONG OnMouseActivate(HWND hwndTop, UINT nHitTest, UINT nMessage);
    LONG OnContextMenu(HWND wParam, int x, int y);

    LONG OnLButtonDown(UINT nFlags, int x, int y);
    LONG OnLButtonUp(UINT nFlags, int x, int y);
    LONG OnLButtonDblClick(UINT nFlags, int x, int y);
    LONG OnMouseMove(UINT nFlags, int x, int y);

    LONG OnKeyDown(UINT nKeyCode, UINT nFlags);
    LONG OnChar(UINT nChar, UINT nFlags);

    LONG OnSetFocus(HWND hwndOld);
    LONG OnKillFocus(HWND hwndNew);

    BOOL OnCut(void);
    BOOL OnCopy(void);
    BOOL OnPaste(void);
    BOOL OnClear(void);

private:

    //
    //    Internal private functions
    //
    LONG        OpenFile(LPTSTR szFileName);
    LONG        ClearFile(void);
    void        ResetLineCache(void);
    ULONG        GetText(LPTSTR szDest, ULONG nStartOffset, ULONG nLength);

    //
    //    Cursor/Selection
    //
    ULONG        SelectionSize(void);
    ULONG        SelectAll(void);

    //void        Toggle


    //
    //    Painting support
    //
    void        RefreshWindow(void);
    void        PaintLine(HDC hdc, ULONG line, int x, int y, HRGN hrgnUpdate);
    void        PaintText(HDC hdc, ULONG nLineNo, int x, int y, RECT *bounds);
    int            PaintMargin(HDC hdc, ULONG line, int x, int y);

    LONG        InvalidateRange(ULONG nStart, ULONG nFinish);
    LONG        InvalidateLine(ULONG nLineNo, bool forceAnalysis);
    VOID        UpdateLine(ULONG nLineNo);


    int            ApplyTextAttributes(ULONG nLineNo, ULONG offset, ULONG &nColumn, LPTSTR szText, int nTextLen, ATTR *attr);
    int            ApplySelection(USPDATA *uspData, ULONG nLineNo, ULONG nOffset, ULONG nTextLen);
    int            SyntaxColour(LPTSTR szText, ULONG nTextLen, ATTR *attr);
    int            StripCRLF(LPTSTR szText, ATTR *attrList, int nLength, bool fAllow);
    void        MarkCRLF(USPDATA *uspData, LPTSTR szText, int nLength, ATTR *attr);
    int            CRLF_size(LPTSTR szText, int nLength);

    //
    //    Font support
    //
    LONG        AddFont(HFONT);
    LONG        SetFont(HFONT, int idx);
    LONG        SetLineSpacing(int nAbove, int nBelow);
    LONG        SetLongLine(int nLength);

    //
    //    
    //
    int            NeatTextYOffset(USPFONT *font);
    int            TextWidth(HDC hdc, LPCTSTR lpString, int c);
    //int        TabWidth(void);
    int            LeftMarginWidth(void);
    void        UpdateMarginWidth(void);
    int            SetCaretWidth(int nWidth);
    BOOL        SetImageList(HIMAGELIST hImgList);
    int            SetLineImage(ULONG nLineNo, ULONG nImageIdx);
    LINEINFO *    GetLineInfo(ULONG nLineNo);

    //
    //    Caret/Cursor positioning
    //
    BOOL        MouseCoordToFilePos(int x, int y, ULONG *pnLineNo, ULONG *pnFileOffset, int *px);//, ULONG *pnLineLen=0);
    VOID        RepositionCaret(void);
    //VOID        MoveCaret(int x, int y);
    VOID        UpdateCaretXY(int x, ULONG lineno);
    VOID        UpdateCaretOffset(ULONG offset, BOOL fTrailing, int *outx = 0, ULONG *outlineno = 0);
    VOID        Smeg(BOOL fAdvancing);

    VOID        MoveWordPrev(void);
    VOID        MoveWordNext(void);
    VOID        MoveWordStart(void);
    VOID        MoveWordEnd(void);
    VOID        MoveCharPrev(void);
    VOID        MoveCharNext(void);
    VOID        MoveLineUp(int numLines);
    VOID        MoveLineDown(int numLines);
    VOID        MovePageUp(void);
    VOID        MovePageDown(void);
    VOID        MoveLineStart(ULONG lineNo);
    VOID        MoveLineEnd(ULONG lineNo);
    VOID        MoveFileStart(void);
    VOID        MoveFileEnd(void);

    //
    //    Editing
    //    
    BOOL        Undo(void);
    BOOL        Redo(void);
    BOOL        CanUndo(void);
    BOOL        CanRedo(void);
    BOOL        ForwardDelete(void);
    BOOL        BackDelete(void);
    ULONG        EnterText(LPTSTR szText, ULONG nLength);

    //
    //    Scrolling
    //
    HRGN        ScrollRgn(int dx, int dy, bool fReturnUpdateRgn);
    void        Scroll(int dx, int dy);
    void        ScrollToCaret(void);
    void        ScrollToPosition(int xpos, ULONG lineno);
    VOID        SetupScrollbars(void);
    VOID        UpdateMetrics(void);
    VOID        RecalcLineHeight(void);
    bool        PinToBottomCorner(void);

    //
    //    TextView configuration
    //
    ULONG        SetStyle(ULONG uMask, ULONG uStyles);
    ULONG        SetVar(ULONG nVar, ULONG nValue);
    ULONG        GetVar(ULONG nVar);
    ULONG        GetStyleMask(ULONG uMask);
    bool        CheckStyle(ULONG uMask);

    COLORREF    SetColour(UINT idx, COLORREF rgbColour);
    COLORREF    GetColour(UINT idx);
    COLORREF    LineColour(ULONG nLineNo);
    COLORREF    LongColour(ULONG nLineNo);

    //
    //    Miscallaneous
    //
    HMENU        CreateContextMenu(void);
    ULONG        NotifyParent(UINT nNotifyCode, NMHDR *optional = 0);



    //
    // ------ Internal TextView State ------
    //

    HWND        m_hWnd;
    HTHEME        m_hTheme;
    ULONG        m_uStyleFlags;

    // File-related data
    ULONG        m_nLineCount;

    // Font-related data    
    USPFONT        m_uspFontList[MAX_FONTS];
    int            m_nNumFonts;
    int            m_nFontWidth;
    int            m_nMaxAscent;
    int            m_nLineHeight;
    int            m_nHeightAbove;
    int            m_nHeightBelow;

    // Scrollbar-related data
    ULONG        m_nVScrollPos;
    ULONG        m_nVScrollMax;
    int            m_nHScrollPos;
    int            m_nHScrollMax;

    int            m_nLongestLine;
    int            m_nWindowLines;
    int            m_nWindowColumns;

    // Cursor/Caret position 
    ULONG        m_nCurrentLine;
    ULONG        m_nSelectionStart;
    ULONG        m_nSelectionEnd;
    ULONG        m_nCursorOffset;
    ULONG        m_nSelMarginOffset1;
    ULONG        m_nSelMarginOffset2;
    int            m_nCaretPosX;
    int            m_nAnchorPosX;

    SELMODE        m_nSelectionMode;
    SELMODE        m_nSelectionType;
    CURPOS        m_cpBlockStart;
    CURPOS        m_cpBlockEnd;
    UINT        m_nEditMode;

    // Display-related data
    int            m_nTabWidthChars;
    DWORD        m_nCaretWidth;
    int            m_nLongLineLimit;
    int            m_nCRLFMode;

    LINEINFO    m_LineInfo[MAX_LINE_INFO];
    int            m_nLineInfoCount;

    // Margin information
    int            m_nLinenoWidth;
    HCURSOR        m_hMarginCursor;
    //RECT        m_rcBorder;

    COLORREF    m_rgbColourList[TXC_MAX_COLOURS];

    // Runtime data
    UINT        m_nScrollTimer;
    int            m_nScrollCounter;
    bool        m_fHideCaret;
    //bool        m_fTransparent;
    HIMAGELIST    m_hImageList;
    HMENU        m_hUserMenu;

    // Cache for USPDATA objects
    USPCACHE    *m_uspCache;
    USPDATA        *GetUspData(HDC hdc, ULONG nLineNo, ULONG *nOffset = 0);
    USPCACHE    *GetUspCache(HDC hdc, ULONG nLineNo, ULONG *nOffset = 0);
    bool         GetLogAttr(ULONG nLineNo, USPCACHE **puspCache, CSCRIPT_LOGATTR **plogAttr = 0, ULONG *pnOffset = 0);

    TextDocument *m_pTextDoc;
};

#endif