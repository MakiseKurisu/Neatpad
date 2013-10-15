//
//    MODULE:        TextDocument.cpp
//
//    PURPOSE:    Basic implementation of a text data-sequence class
//
//    NOTES:        www.catch22.net
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "TextDocument.h"
#include "TextView.h"
#include "Unicode.h"

struct _BOM_LOOKUP BOMLOOK [] =
{
    // define longest headers first
    { 0x0000FEFF, 4, NCP_UTF32 },
    { 0xFFFE0000, 4, NCP_UTF32BE },
    { 0x00BFBBEF, 3, NCP_UTF8 },
    { 0x0000FFFE, 2, NCP_UTF16BE },
    { 0x0000FEFF, 2, NCP_UTF16 },
    { 0x00000000, 0, NCP_ASCII },
};

//
//    TextDocument constructor
//
TextDocument * new_TextDocument(void)
{
    TextDocument * lps = (TextDocument *) malloc(sizeof(TextDocument));

    if (lps)
    {
        memset(lps, 0, sizeof(*lps));

        lps->nDocLength_bytes = 0;
        lps->nDocLength_chars = 0;

        lps->pLineBuf_byte = 0;
        lps->pLineBuf_char = 0;
        lps->nNumLines = 0;

        lps->nFileFormat = NCP_ASCII;
        lps->nHeaderSize = 0;

        lps->seq = new_sequence();
    }

    return lps;
}

//
//    TextDocument destructor
//
void delete_TextDocument(
    TextDocument * lps
    )
{
    clear_TextDocument(lps);
    delete_sequence(lps->seq);
    free(lps);
}

//
//    Initialize the TextDocument with the specified file
//
bool init_TextDocument(
    TextDocument * lps,
    LPTSTR filename
    )
{
    HANDLE hFile;

    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    return init_TextDocument(lps, hFile);
}

//
//    Initialize using a file-handle
//
bool init_TextDocument(
    TextDocument * lps,
    HANDLE hFile
    )
{
    ULONG numread;
    char * buffer;

    if ((lps->nDocLength_bytes = GetFileSize(hFile, 0)) == 0)
    {
        return false;
    }

    // allocate new file-buffer
    if ((buffer = (char *) malloc(sizeof(char) * lps->nDocLength_bytes)) == 0)
    {
        return false;
    }

    // read entire file into memory
    ReadFile(hFile, buffer, lps->nDocLength_bytes, &numread, 0);

    init_sequence(lps->seq, (BYTE *) buffer, lps->nDocLength_bytes);

    // try to detect if this is an ascii/unicode/utf8 file
    lps->nFileFormat = detect_file_format_TextDocument(lps);

    // work out where each line of text starts
    if (!init_linebuffer_TextDocument(lps))
        clear_TextDocument(lps);

    CloseHandle(hFile);
    free(buffer);
    return true;
}

//    Initialize the TextDocument with the specified file
//
bool save_TextDocument(
    TextDocument * lps,
    LPTSTR filename
    )
{
    HANDLE hFile;

    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    CloseHandle(hFile);
    return true;
}


//
//    Parse the file lo
//
//
//    From the unicode.org FAQ:
//
//    00 00 FE FF       UTF-32, big-endian 
//    FF FE 00 00       UTF-32, little-endian 
//    FE FF             UTF-16, big-endian 
//    FF FE             UTF-16, little-endian 
//    EF BB BF          UTF-8 
//
//    Match the first x bytes of the file against the
//    Byte-Order-Mark (BOM) lookup table
//
int detect_file_format_TextDocument(
    TextDocument * lps
    )
{
    BYTE header[4] = { 0 };
    render_sequence(lps->seq, 0, header, 4);

    for (int i = 0; BOMLOOK[i].len; i++)
    {
        if (lps->nDocLength_bytes >= BOMLOOK[i].len &&
            memcmp(header, &BOMLOOK[i].bom, BOMLOOK[i].len) == 0)
        {
            lps->nHeaderSize = BOMLOOK[i].len;
            return BOMLOOK[i].type;
        }
    }

    lps->nHeaderSize = 0;
    return NCP_ASCII;    // default to ASCII
}


//
//    Empty the data-TextDocument
//
bool clear_TextDocument(
    TextDocument * lps
    )
{
    clear_sequence(lps->seq);
    lps->nDocLength_bytes = 0;

    if (lps->pLineBuf_byte)
    {
        free(lps->pLineBuf_byte);
        lps->pLineBuf_byte = 0;
    }

    if (lps->pLineBuf_char)
    {
        free(lps->pLineBuf_char);
        lps->pLineBuf_char = 0;
    }

    lps->nNumLines = 0;
    return true;
}

bool EmptyDoc_TextDocument(
    TextDocument * lps
    )
{
    clear_TextDocument(lps);
    init_sequence(lps->seq);

    // this is not robust. it's just to get the thing
    // up-and-running until I write a proper line-buffer mananger
    lps->pLineBuf_byte = (ULONG *) malloc(sizeof(ULONG) * 0x1000);
    lps->pLineBuf_char = (ULONG *) malloc(sizeof(ULONG) * 0x1000);

    lps->pLineBuf_byte[0] = 0;
    lps->pLineBuf_char[0] = 0;

    return true;
}


//
//    Return a UTF-32 character value
//
int getchar_TextDocument(
    TextDocument * lps,
    ULONG offset,
    ULONG lenbytes,
    ULONG * pch32
    )
{
    //    BYTE    *rawdata   = (BYTE *)(buffer + offset + lps->nHeaderSize);
    BYTE rawdata[16];

    lenbytes = min(16, lenbytes);
    render_sequence(lps->seq, offset + lps->nHeaderSize, rawdata, lenbytes);

#ifdef UNICODE

    UTF16 * rawdata_w = (UTF16 *) rawdata;//(WCHAR*)(buffer + offset + lps->nHeaderSize);
    WCHAR ch16;
    size_t ch32len = 1;

    switch (lps->nFileFormat)
    {
    case NCP_ASCII:
        MultiByteToWideChar(CP_ACP, 0, (CCHAR *) rawdata, 1, &ch16, 1);
        *pch32 = ch16;
        return 1;

    case NCP_UTF16:
        return utf16_to_utf32(rawdata_w, lenbytes / 2, pch32, &ch32len) * sizeof(WCHAR);

    case NCP_UTF16BE:
        return utf16be_to_utf32(rawdata_w, lenbytes / 2, pch32, &ch32len) * sizeof(WCHAR);

    case NCP_UTF8:
        return utf8_to_utf32(rawdata, lenbytes, pch32);

    default:
        return 0;
    }

#else

    *pch32 = (ULONG) (BYTE) rawdata[0];
    return 1;

#endif
}

//
//  Fetch a buffer of UTF-16 text from the specified byte offset - 
//  returns the number of characters stored in buf
//
//  Depending on how Neatpad was compiled (UNICODE vs ANSI) this function
//  will always return text in the "native" format - i.e. Unicode or Ansi -
//  so the necessary conversions will take place here.
//
//  TODO: make sure the CR/LF is always fetched in one go
//        make sure utf-16 surrogates kept together
//        make sure that combining chars kept together
//        make sure that bidirectional text kep together (will be *hard*) 
//
//  offset   - BYTE offset within underlying data sequence
//  lenbytes - max number of bytes to process (i.e. to limit to a line)
//  buf         - UTF16/ASCII output buffer
//  plen     - [in] - length of buffer, [out] - number of code-units stored
//
//  returns  - number of bytes processed
//
ULONG gettext_TextDocument(
    TextDocument * lps,
    ULONG offset,
    ULONG lenbytes,
    LPTSTR buf,
    ULONG * buflen
    )
{
    //    BYTE    *rawdata = (BYTE *)(buffer + offset + lps->nHeaderSize);

    ULONG chars_copied = 0;
    ULONG bytes_processed = 0;

    if (offset >= lps->nDocLength_bytes)
    {
        *buflen = 0;
        return 0;
    }

    while (lenbytes > 0 && *buflen > 0)
    {
        BYTE rawdata[0x100];
        size_t rawlen = min(lenbytes, 0x100);

        // get next block of data from the piece-table
        render_sequence(lps->seq, offset + lps->nHeaderSize, rawdata, rawlen);

        // convert to UTF-16 
        size_t tmplen = *buflen;
        rawlen = rawdata_to_utf16_TextDocument(lps, rawdata, rawlen, buf, &tmplen);

        lenbytes -= rawlen;
        offset += rawlen;
        bytes_processed += rawlen;

        buf += tmplen;
        *buflen -= tmplen;
        chars_copied += tmplen;
    }

    *buflen = chars_copied;
    return bytes_processed;

    //ULONG remaining = lenbytes;
    //int   charbuflen = *buflen;

    //while(remaining)
    /*    {
            lenbytes = min(lenbytes, sizeof(rawdata));
            render_sequence(lps->seq, offset + lps->nHeaderSize, rawdata, lenbytes);

            #ifdef UNICODE

            switch(lps->nFileFormat)
            {
            // convert from ANSI->UNICODE
            case NCP_ASCII:
            return ascii_to_utf16(rawdata, lenbytes, buf, (size_t*)buflen);

            case NCP_UTF8:
            return utf8_to_utf16(rawdata, lenbytes, buf, (size_t*)buflen);

            // already unicode, do a straight memory copy
            case NCP_UTF16:
            return copy_utf16((WCHAR*)rawdata, lenbytes/sizeof(WCHAR), buf, (size_t*)buflen);

            // need to convert from big-endian to little-endian
            case NCP_UTF16BE:
            return swap_utf16((WCHAR*)rawdata, lenbytes/sizeof(WCHAR), buf, (size_t*)buflen);

            // error! we should *never* reach this point
            default:
            *buflen = 0;
            return 0;
            }

            #else

            switch(lps->nFileFormat)
            {
            // we are already an ASCII app, so do a straight memory copy
            case NCP_ASCII:

            int len;

            len = min(*buflen, lenbytes);
            memcpy(buf, rawdata, len);

            *buflen = len;
            return len;

            // anything else is an error - we cannot support Unicode or multibyte
            // character sets with a plain ASCII app.
            default:
            *buflen = 0;
            return 0;
            }

            #endif

            //    remaining -= lenbytes;
            //    buf       += lenbytes;
            //    offset    += lenbytes;
            }*/
}

ULONG getdata_TextDocument(
    TextDocument * lps,
    ULONG offset,
    BYTE * buf,
    size_t len
    )
{
    //memcpy(buf, buffer + offset + lps->nHeaderSize, len);
    render_sequence(lps->seq, offset + lps->nHeaderSize, buf, len);
    return len;
}

//
//    Initialize the line-buffer
//
//    With Unicode a newline sequence is defined as any of the following:
//
//    \u000A | \u000B | \u000C | \u000D | \u0085 | \u2028 | \u2029 | \u000D\u000A
//
bool init_linebuffer_TextDocument(
    TextDocument * lps
    )
{
    ULONG offset_bytes = 0;
    ULONG offset_chars = 0;
    ULONG linestart_bytes = 0;
    ULONG linestart_chars = 0;
    ULONG bytes_left = lps->nDocLength_bytes - lps->nHeaderSize;

    ULONG buflen = lps->nDocLength_bytes - lps->nHeaderSize;

    // allocate the line-buffer for storing each line's BYTE offset
    if ((lps->pLineBuf_byte = (ULONG *) malloc(sizeof(ULONG) * (buflen + 1))) == 0)
        return false;

    // allocate the line-buffer for storing each line's CHARACTER offset
    if ((lps->pLineBuf_char = (ULONG *) malloc(sizeof(ULONG) * (buflen + 1))) == 0)
        return false;

    lps->nNumLines = 0;


    // loop through every byte in the file
    for (offset_bytes = 0; offset_bytes < buflen;)
    {
        ULONG ch32;

        // get a UTF-32 character from the underlying file format.
        // this needs serious thought. Currently 
        ULONG len = getchar_TextDocument(lps, offset_bytes, buflen - offset_bytes, &ch32);
        offset_bytes += len;
        offset_chars += 1;

        if (ch32 == '\r')
        {
            // record where the line starts
            lps->pLineBuf_byte[lps->nNumLines] = linestart_bytes;
            lps->pLineBuf_char[lps->nNumLines] = linestart_chars;
            linestart_bytes = offset_bytes;
            linestart_chars = offset_chars;

            // look ahead to next char
            len = getchar_TextDocument(lps, offset_bytes, buflen - offset_bytes, &ch32);
            offset_bytes += len;
            offset_chars += 1;

            // carriage-return / line-feed combination
            if (ch32 == '\n')
            {
                linestart_bytes = offset_bytes;
                linestart_chars = offset_chars;
            }

            lps->nNumLines++;
        }
        else if (ch32 == '\n' || ch32 == '\x0b' || ch32 == '\x0c' || ch32 == 0x0085 || ch32 == 0x2029 || ch32 == 0x2028)
        {
            // record where the line starts
            lps->pLineBuf_byte[lps->nNumLines] = linestart_bytes;
            lps->pLineBuf_char[lps->nNumLines] = linestart_chars;
            linestart_bytes = offset_bytes;
            linestart_chars = offset_chars;
            lps->nNumLines++;
        }
        // force a 'hard break' 
        else if (offset_chars - linestart_chars > 128)
        {
            lps->pLineBuf_byte[lps->nNumLines] = linestart_bytes;
            lps->pLineBuf_char[lps->nNumLines] = linestart_chars;
            linestart_bytes = offset_bytes;
            linestart_chars = offset_chars;
            lps->nNumLines++;
        }
    }

    if (buflen > 0)
    {
        lps->pLineBuf_byte[lps->nNumLines] = linestart_bytes;
        lps->pLineBuf_char[lps->nNumLines] = linestart_chars;
        lps->nNumLines++;
    }

    lps->pLineBuf_byte[lps->nNumLines] = buflen;
    lps->pLineBuf_char[lps->nNumLines] = offset_chars;

    return true;
}


//
//    Return the number of lines
//
ULONG linecount_TextDocument(
    TextDocument * lps
    )
{
    return lps->nNumLines;
}

//
//    Return the length of longest line
//
ULONG longestline_TextDocument(
    TextDocument * lps,
    int tabwidth
    )
{
    //ULONG i;
    ULONG longest = 0;
    ULONG xpos = 0;
    //    char *bufptr = (char *)(buffer + lps->nHeaderSize);
    /*
        for(i = 0; i < length_bytes; i++)
        {
        if(bufptr[i] == '\r')
        {
        if(bufptr[i+1] == '\n')
        i++;

        longest = max(longest, xpos);
        xpos = 0;
        }
        else if(bufptr[i] == '\n')
        {
        longest = max(longest, xpos);
        xpos = 0;
        }
        else if(bufptr[i] == '\t')
        {
        xpos += tabwidth - (xpos % tabwidth);
        }
        else
        {
        xpos ++;
        }
        }

        longest = max(longest, xpos);*/
    return 100;//longest;
}

//
//    Return information about specified line
//
bool lineinfo_from_lineno_TextDocument(
    TextDocument * lps,
    ULONG lineno,
    ULONG * lineoff_chars,
    ULONG * linelen_chars,
    ULONG * lineoff_bytes,
    ULONG * linelen_bytes
    )
{
    if (lineno < lps->nNumLines)
    {
        if (linelen_chars) *linelen_chars = lps->pLineBuf_char[lineno + 1] - lps->pLineBuf_char[lineno];
        if (lineoff_chars) *lineoff_chars = lps->pLineBuf_char[lineno];

        if (linelen_bytes) *linelen_bytes = lps->pLineBuf_byte[lineno + 1] - lps->pLineBuf_byte[lineno];
        if (lineoff_bytes) *lineoff_bytes = lps->pLineBuf_byte[lineno];

        return true;
    }
    else
    {
        return false;
    }
}

//
//    Perform a reverse lookup - file-offset to line number
//
bool lineinfo_from_offset_TextDocument(
    TextDocument * lps,
    ULONG offset_chars,
    ULONG *lineno,
    ULONG *lineoff_chars,
    ULONG *linelen_chars,
    ULONG *lineoff_bytes,
    ULONG *linelen_bytes
    )
{
    ULONG low = 0;
    ULONG high = lps->nNumLines - 1;
    ULONG line = 0;

    if (lps->nNumLines == 0)
    {
        if (lineno)           *lineno = 0;
        if (lineoff_chars)    *lineoff_chars = 0;
        if (linelen_chars)    *linelen_chars = 0;
        if (lineoff_bytes)    *lineoff_bytes = 0;
        if (linelen_bytes)    *linelen_bytes = 0;

        return false;
    }

    while (low <= high)
    {
        line = (high + low) / 2;

        if (offset_chars >= lps->pLineBuf_char[line] && offset_chars < lps->pLineBuf_char[line + 1])
        {
            break;
        }
        else if (offset_chars < lps->pLineBuf_char[line])
        {
            high = line - 1;
        }
        else
        {
            low = line + 1;
        }
    }

    if (lineno)            *lineno = line;
    if (lineoff_bytes)    *lineoff_bytes = lps->pLineBuf_byte[line];
    if (linelen_bytes)    *linelen_bytes = lps->pLineBuf_byte[line + 1] - lps->pLineBuf_byte[line];
    if (lineoff_chars)    *lineoff_chars = lps->pLineBuf_char[line];
    if (linelen_chars)    *linelen_chars = lps->pLineBuf_char[line + 1] - lps->pLineBuf_char[line];

    return true;
}

int getformat_TextDocument(
    TextDocument * lps
    )
{
    return lps->nFileFormat;
}

ULONG size_TextDocument(
    TextDocument * lps
    )
{
    return lps->nDocLength_bytes;
}

TextIterator iterate_TextDocument(
    TextDocument * lps,
    ULONG offset_chars
    )
{
    ULONG off_bytes = charoffset_to_byteoffset_TextDocument(lps, offset_chars);
    ULONG len_bytes = lps->nDocLength_bytes - off_bytes;

    //if(!lineinfo_frolps->offset(offset_chars, 0, linelen, &offset_bytes, &length_bytes))
    //    return TextIterator();

    return *new_TextIterator(off_bytes, len_bytes, lps);
}


//
//
//
TextIterator iterate_line_TextDocument(
    TextDocument * lps,
    ULONG lineno,
    ULONG * linestart,
    ULONG * linelen
    )
{
    ULONG offset_bytes;
    ULONG length_bytes;

    if (!lineinfo_from_lineno_TextDocument(lps, lineno, linestart, linelen, &offset_bytes, &length_bytes))
        return *new_TextIterator();

    return *new_TextIterator(offset_bytes, length_bytes, lps);
}

TextIterator iterate_line_offset_TextDocument(
    TextDocument * lps,
    ULONG offset_chars,
    ULONG * lineno,
    ULONG * linestart
    )
{
    ULONG offset_bytes;
    ULONG length_bytes;

    if (!lineinfo_from_offset_TextDocument(lps, offset_chars, lineno, linestart, 0, &offset_bytes, &length_bytes))
        return *new_TextIterator();

    return *new_TextIterator(offset_bytes, length_bytes, lps);
}

ULONG lineno_from_offset_TextDocument(
    TextDocument * lps,
    ULONG offset
    )
{
    ULONG lineno = 0;
    lineinfo_from_offset_TextDocument(lps, offset, &lineno, 0, 0, 0, 0);
    return lineno;
}

ULONG offset_from_lineno_TextDocument(
    TextDocument * lps,
    ULONG lineno
    )
{
    ULONG lineoff = 0;
    lineinfo_from_lineno_TextDocument(lps, lineno, &lineoff, 0, 0, 0);
    return lineoff;
}

//
//    Retrieve an entire line of text
//    
ULONG getline_TextDocument(
    TextDocument * lps,
    ULONG nLineNo,
    LPTSTR buf,
    ULONG buflen,
    ULONG * off_chars
    )
{
    ULONG offset_bytes;
    ULONG length_bytes;
    ULONG offset_chars;
    ULONG length_chars;

    if (!lineinfo_from_lineno_TextDocument(lps, nLineNo, &offset_chars, &length_chars, &offset_bytes, &length_bytes))
    {
        *off_chars = 0;
        return 0;
    }

    gettext_TextDocument(lps, offset_bytes, length_bytes, buf, &buflen);

    *off_chars = offset_chars;
    return buflen;
}

//
//    Convert the RAW buffer in underlying file-format to UTF-16
//
//    
//    utf16len    - [in/out]    on input holds size of utf16str buffer, 
//                            on output holds number of utf16 characters stored
//
//    returns bytes processed from rawdata
//
size_t rawdata_to_utf16_TextDocument(
    TextDocument * lps,
    BYTE * rawdata,
    size_t rawlen,
    LPTSTR utf16str,
    size_t * utf16len
    )
{
    switch (lps->nFileFormat)
    {
        // convert from ANSI->UNICODE
    case NCP_ASCII:
        return ascii_to_utf16(rawdata, rawlen, (UTF16 *) utf16str, utf16len);

    case NCP_UTF8:
        return utf8_to_utf16(rawdata, rawlen, (UTF16 *) utf16str, utf16len);

        // already unicode, do a straight memory copy
    case NCP_UTF16:
        rawlen /= sizeof(TCHAR);
        return copy_utf16((UTF16 *) rawdata, rawlen, (UTF16 *) utf16str, utf16len) * sizeof(TCHAR);

        // need to convert from big-endian to little-endian
    case NCP_UTF16BE:
        rawlen /= sizeof(TCHAR);
        return swap_utf16((UTF16 *) rawdata, rawlen, (UTF16 *) utf16str, utf16len) * sizeof(TCHAR);

        // error! we should *never* reach this point
    default:
        *utf16len = 0;
        return 0;
    }
}

//
//    Converts specified UTF16 string to the underlying RAW format of the text-document
//    (i.e. UTF-16 -> UTF-8
//          UTF-16 -> UTF-32 etc)
//
//    returns number of WCHARs processed from utf16str
//
size_t utf16_to_rawdata_TextDocument(
    TextDocument * lps,
    LPTSTR utf16str,
    size_t utf16len,
    BYTE * rawdata,
    size_t * rawlen
    )
{
    switch (lps->nFileFormat)
    {
        // convert from UTF16 -> ASCII
    case NCP_ASCII:
        return utf16_to_ascii((UTF16 *) utf16str, utf16len, rawdata, rawlen);

        // convert from UTF16 -> UTF8
    case NCP_UTF8:
        return utf16_to_utf8((UTF16 *) utf16str, utf16len, rawdata, rawlen);

        // already unicode, do a straight memory copy
    case NCP_UTF16:
        *rawlen /= sizeof(TCHAR);
        utf16len = copy_utf16((UTF16 *) utf16str, utf16len, (UTF16 *) rawdata, rawlen);
        *rawlen *= sizeof(TCHAR);
        return utf16len;

        // need to convert from big-endian to little-endian
    case NCP_UTF16BE:
        *rawlen /= sizeof(TCHAR);
        utf16len = swap_utf16((UTF16 *) utf16str, utf16len, (UTF16 *) rawdata, rawlen);
        *rawlen *= sizeof(TCHAR);
        return utf16len;

        // error! we should *never* reach this point
    default:
        *rawlen = 0;
        return 0;
    }

}

//
//    Insert UTF-16 text at specified BYTE offset
//
//    returns number of BYTEs stored
//
ULONG insert_raw_TextDocument(
    TextDocument * lps,
    ULONG offset_bytes,
    LPTSTR text,
    ULONG length
    )
{
    BYTE  buf[0x100];
    ULONG buflen;
    ULONG copied;
    ULONG rawlen = 0;
    ULONG offset = offset_bytes + lps->nHeaderSize;

    while (length)
    {
        buflen = 0x100;
        copied = utf16_to_rawdata_TextDocument(lps, text, length, buf, (size_t *) &buflen);

        // do the piece-table insertion!
        if (!insert_sequence(lps->seq, offset, buf, buflen))
            break;

        text += copied;
        length -= copied;
        rawlen += buflen;
        offset += buflen;
    }

    lps->nDocLength_bytes = size_sequence(lps->seq);
    return rawlen;
}

ULONG replace_raw_TextDocument(
    TextDocument * lps,
    ULONG offset_bytes,
    LPTSTR text,
    ULONG length,
    ULONG erase_chars
    )
{
    BYTE  buf[0x100];
    ULONG buflen;
    ULONG copied;
    ULONG rawlen = 0;
    ULONG offset = offset_bytes + lps->nHeaderSize;

    ULONG erase_bytes = count_chars_TextDocument(lps, offset_bytes, erase_chars);

    while (length)
    {
        buflen = 0x100;
        copied = utf16_to_rawdata_TextDocument(lps, text, length, buf, (size_t *) &buflen);

        // do the piece-table replacement!
        if (!replace_sequence(lps->seq, offset, buf, buflen, erase_bytes))
            break;

        text += copied;
        length -= copied;
        rawlen += buflen;
        offset += buflen;

        erase_bytes = 0;
    }

    lps->nDocLength_bytes = size_sequence(lps->seq);
    return rawlen;
}

//
//    Erase is a little different. Need to work out how many
//  bytes the specified number of UTF16 characters takes up
//
ULONG erase_raw_TextDocument(
    TextDocument * lps,
    ULONG offset_bytes,
    ULONG length
    )
{
    /*TCHAR  buf[0x100];
    ULONG  buflen;
    ULONG  bytes;
    ULONG  erase_bytes  = 0;
    ULONG  erase_offset = offset_bytes;

    while(length)
    {
    buflen = min(0x100, length);
    bytes = gettext(offset_bytes, 0x100, buf, &buflen);

    erase_bytes  += bytes;
    offset_bytes += bytes;
    length       -= buflen;
    }

    // do the piece-table deletion!
    if(erase_sequence(lps->seq, erase_offset + lps->nHeaderSize, erase_bytes))
    {
    lps->nDocLength_bytes = size_sequence(lps->seq);
    return length;
    }*/

    ULONG erase_bytes = count_chars_TextDocument(lps, offset_bytes, length);

    if (erase_sequence(lps->seq, offset_bytes + lps->nHeaderSize, erase_bytes))
    {
        lps->nDocLength_bytes = size_sequence(lps->seq);
        return length;
    }

    return 0;
}

//
//    return number of bytes comprising 'length_chars' characters
//    in the underlying raw file
//
ULONG count_chars_TextDocument(
    TextDocument * lps,
    ULONG offset_bytes,
    ULONG length_chars
    )
{
    switch (lps->nFileFormat)
    {
    case NCP_ASCII:
        return length_chars;

    case NCP_UTF16:
    case NCP_UTF16BE:
        return length_chars * sizeof(WCHAR);

    default:
        break;
    }

    ULONG offset_start = offset_bytes;

    while (length_chars && offset_bytes < lps->nDocLength_bytes)
    {
        TCHAR buf[0x100];
        ULONG charlen = min(length_chars, 0x100);
        ULONG bytelen;

        bytelen = gettext_TextDocument(lps, offset_bytes, lps->nDocLength_bytes - offset_bytes, buf, &charlen);

        length_chars -= charlen;
        offset_bytes += bytelen;
    }

    return offset_bytes - offset_start;
}

ULONG byteoffset_to_charoffset_TextDocument(
    TextDocument * lps,
    ULONG offset_bytes
    )
{
    switch (lps->nFileFormat)
    {
    case NCP_ASCII:
        return offset_bytes;

    case NCP_UTF16:
    case NCP_UTF16BE:
        return offset_bytes / sizeof(WCHAR);

    case NCP_UTF8:
    case NCP_UTF32:
    case NCP_UTF32BE:
        // bug bug! need to implement this. 
    default:
        break;
    }

    return 0;
}

ULONG charoffset_to_byteoffset_TextDocument(
    TextDocument * lps,
    ULONG offset_chars
    )
{
    switch (lps->nFileFormat)
    {
    case NCP_ASCII:
        return offset_chars;

    case NCP_UTF16:
    case NCP_UTF16BE:
        return offset_chars * sizeof(WCHAR);

    case NCP_UTF8:
    case NCP_UTF32:
    case NCP_UTF32BE:
    default:
        break;
    }

    ULONG lineoff_chars;
    ULONG lineoff_bytes;

    if (lineinfo_from_offset_TextDocument(lps, offset_chars, 0, &lineoff_chars, 0, &lineoff_bytes, 0))
    {
        return count_chars_TextDocument(lps, lineoff_bytes, offset_chars - lineoff_chars) + lineoff_bytes;
    }
    else
    {
        return 0;
    }
}

//
//    Insert text at specified character-offset
//
ULONG insert_text_TextDocument(
    TextDocument * lps,
    ULONG offset_chars,
    LPTSTR text,
    ULONG length
    )
{
    ULONG offset_bytes = charoffset_to_byteoffset_TextDocument(lps, offset_chars);
    return insert_raw_TextDocument(lps, offset_bytes, text, length);
}

//
//    Overwrite text at specified character-offset
//
ULONG replace_text_TextDocument(
    TextDocument * lps,
    ULONG offset_chars,
    LPTSTR text,
    ULONG length,
    ULONG erase_len
    )
{
    ULONG offset_bytes = charoffset_to_byteoffset_TextDocument(lps, offset_chars);
    return replace_raw_TextDocument(lps, offset_bytes, text, length, erase_len);
}

//
//    Erase text at specified character-offset
//
ULONG erase_text_TextDocument(
    TextDocument * lps,
    ULONG offset_chars,
    ULONG length
    )
{
    ULONG offset_bytes = charoffset_to_byteoffset_TextDocument(lps, offset_chars);
    return erase_raw_TextDocument(lps, offset_bytes, length);
}

bool Undo_TextDocument(
    TextDocument * lps,
    ULONG *offset_start,
    ULONG *offset_end
    )
{
    ULONG start, length;

    if (!undo_sequence(lps->seq))
        return false;

    start = event_index_sequence(lps->seq) - lps->nHeaderSize;
    length = event_length_sequence(lps->seq);

    *offset_start = byteoffset_to_charoffset_TextDocument(lps, start);
    *offset_end = byteoffset_to_charoffset_TextDocument(lps, start + length);

    lps->nDocLength_bytes = size_sequence(lps->seq);

    return true;
}

bool Redo_TextDocument(
    TextDocument * lps,
    ULONG *offset_start,
    ULONG *offset_end
    )
{
    ULONG start, length;

    if (!redo_sequence(lps->seq))
        return false;

    start = event_index_sequence(lps->seq) - lps->nHeaderSize;
    length = event_length_sequence(lps->seq);

    *offset_start = byteoffset_to_charoffset_TextDocument(lps, start);
    *offset_end = byteoffset_to_charoffset_TextDocument(lps, start + length);

    lps->nDocLength_bytes = size_sequence(lps->seq);

    return true;
}

TextIterator * new_TextIterator(
    ULONG off,
    ULONG len,
    TextDocument * td
    )
{
    TextIterator * lps = (TextIterator *) malloc(sizeof(TextIterator));

    if (lps)
    {
        memset(lps, 0, sizeof(*lps));

        lps->text_doc = td;
        lps->off_bytes = off;
        lps->len_bytes = len;
    }

    return lps;
}

TextIterator * new_TextIterator(
    const TextIterator & ti
    )
{
    return new_TextIterator(ti.off_bytes, ti.len_bytes, ti.text_doc);
}

TextIterator * copy_TextIterator(
    TextIterator * lps,
    const TextIterator & ti
    )
{
    lps->text_doc = ti.text_doc;
    lps->off_bytes = ti.off_bytes;
    lps->len_bytes = ti.len_bytes;

    return lps;
}

void delete_TextIterator(
    TextIterator * lps
    )
{
    free(lps);
}

ULONG gettext_TextIterator(
    TextIterator * lps,
    LPTSTR buf,
    ULONG buflen
    )
{
    if (lps->text_doc)
    {
        // get text from the TextDocument at the specified byte-offset
        ULONG len = gettext_TextDocument(lps->text_doc, lps->off_bytes, lps->len_bytes, buf, &buflen);

        // adjust the iterator's internal position
        lps->off_bytes += len;
        lps->len_bytes -= len;

        return buflen;
    }
    else
    {
        return 0;
    }
}

bool valid_TextIterator(
    TextIterator * lps
    )
{
    return lps->text_doc ? true : false;
}