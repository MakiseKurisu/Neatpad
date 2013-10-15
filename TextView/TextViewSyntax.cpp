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

#define COOKIE_BEGIN 0
#define COOKIE_END   1

//typedef struct LEX;

typedef struct
{
    ATTR    attr;
    ULONG   type;

} TYPE, *LPTYPE;

typedef struct _LEX
{
    LPTYPE  type;

    int     num_branches;
    _LEX *  branch[1];
} LEX, *LPLEX;

LEX firstchar[256];

ULONG lexer(ULONG cookie, LPTSTR buf, ULONG len, LPATTR attr)
{
    LPLEX   state = 0;
    int     i = 0;
    ULONG   ch = buf[i++];

    // start at the correct place in our state machine
    LPLEX   node = &state[cookie];

    // look for a match against the current character
    //    for(i = 0; i < node->num_branches; i++)
    //    {
    //    if(node->branch[i].
    //    }


    if (ch < 256)
    {

    }


    //switc
    return COOKIE_END;
}

LPTSTR gettok(LPTSTR ptr, LPTSTR buf, int buflen)
{
    int ch = *ptr++;
    int i = 0;

    if (buf == 0)
        return 0;

    // end of string?
    if (ptr == 0 || ch == 0)
    {
        buf[0] = '\0';
        return 0;
    }

    // strip any leading whitespace
    // whitspace: { <space> | <tab> | <new line> }
    while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
    {
        if (*ptr == 0)
            return 0;

        ch = *ptr++;
    }

    // found a quote - skip to matching pair
    /*    if(ch == '\"')
        {
        ch = *ptr++;
        while(i < buflen && ch != '\0' && ch != '\"' && ch != '\n' && ch != '\r')
        {
        buf[i++] = ch;
        ch = *ptr++;
        }
        }
        else*/
    {
        while (i < buflen && ch)
        {
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')// || ch == '\"')
                break;

            buf[i++] = ch;


            if (ch = *ptr) ptr++;


        }
    }

    buf[i] = 0;

    return ptr;
}

VOID xSetStyle(ATTR *attr, ULONG nPos, ULONG nLen, COLORREF fg, COLORREF bg, int bold = 0)
{
    for (ULONG i = nPos; i < nPos + nLen; i++)
    {
        attr[i].fg = fg;
        //attr[i].bg = bg;

        //if(bold)
        //    attr[i].font = 1;
    }
}

typedef struct
{
    TCHAR firstchar[256];

} SYNTAX_NODE;

int SyntaxColour_TextView(
    TextView * lps,
    LPTSTR szText,
    ULONG nTextLen,
    ATTR * attr
    )
{
    TCHAR tok[128];

    LPTSTR ptr1 = szText;
    LPTSTR ptr2;

    if (nTextLen == 0)
        return 0;

    szText[nTextLen] = 0;

    while ((ptr2 = gettok(ptr1, tok, 128)) != 0)
    {
        if (isdigit(tok[0]))
        {
            xSetStyle(attr, ptr1 - szText, ptr2 - ptr1, RGB(255, 0, 2550), RGB(255, 255, 255), 0);
        }
        else if (memcmp(tok, TEXT("if"), 2) == 0 || memcmp(tok, TEXT("for"), 3) == 0)
        {
            xSetStyle(attr, ptr1 - szText, ptr2 - ptr1, RGB(0,0,255), RGB(255,255,255), 1);
        }
        else if (tok[0] == '\"' || tok[0] == '<' || tok[0] == '\'')
        {
            xSetStyle(attr, ptr1 - szText, ptr2 - ptr1, RGB(200, 100, 200), RGB(255, 255, 255));
        }
        else if (tok[0] == '#')//(memcmp(tok, TEXT("#include"),8) == 0)
        {
            //SetStyle(attr, ptr1 - szText, ptr2 - ptr1, RGB(255,255,0), RGB(255,0,0));
            //xSetStyle(attr, ptr1 - szText, ptr2-ptr1, RGB(65,102,190), RGB(255,255,255));
            xSetStyle(attr, ptr1 - szText, ptr2 - ptr1, RGB(100, 150, 200), RGB(255, 255, 255));
        }
        else if (lstrcmpi(tok, TEXT("ULONG")) == 0)
        {
            xSetStyle(attr, ptr1 - szText, ptr2 - ptr1, RGB(100, 200, 255), RGB(255, 255, 255));
        }
        else if (memcmp(tok, TEXT("//"), 2) == 0)
        {
            //xSetStyle(attr, ptr1 - szText, nTextLen - (ptr1-szText), RGB(128,90,20), RGB(255,255,255));
            xSetStyle(attr, ptr1 - szText, ptr2 - ptr1, RGB(0, 127, 0), RGB(255, 255, 255));
            break;
        }
        else
        {
            //xSetStyle(attr, ptr1 - szText, nTextLen - (ptr1-szText), RGB(200,200,200), RGB(255,255,255));
        }

        ptr1 = ptr2;
    }

    return 0;
}
