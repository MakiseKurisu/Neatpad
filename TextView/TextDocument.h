#ifndef TEXTDOC_INCLUDED
#define TEXTDOC_INCLUDED

#include "codepages.h"
#include "sequence.h"

struct _TextIterator;
typedef struct _TextIterator TextIterator;

class TextDocument
{
    friend class TextView;

public:
    TextDocument(void);
    ~TextDocument(void);

    bool  init(HANDLE hFile);
    bool  init(LPTSTR filename);

    bool  clear(void);
    bool EmptyDoc(void);

    bool    Undo(ULONG *offset_start, ULONG *offset_end);
    bool    Redo(ULONG *offset_start, ULONG *offset_end);

    // UTF-16 text-editing interface
    ULONG    insert_text(ULONG offset_chars, LPTSTR text, ULONG length);
    ULONG    replace_text(ULONG offset_chars, LPTSTR text, ULONG length, ULONG erase_len);
    ULONG    erase_text(ULONG offset_chars, ULONG length);

    ULONG lineno_from_offset(ULONG offset);
    ULONG offset_from_lineno(ULONG lineno);

    bool  lineinfo_from_offset(ULONG offset_chars, ULONG *lineno, ULONG *lineoff_chars, ULONG *linelen_chars, ULONG *lineoff_bytes, ULONG *linelen_bytes);
    bool  lineinfo_from_lineno(ULONG lineno, ULONG *lineoff_chars, ULONG *linelen_chars, ULONG *lineoff_bytes, ULONG *linelen_bytes);

    TextIterator iterate(ULONG offset);
    TextIterator iterate_line(ULONG lineno, ULONG *linestart = 0, ULONG *linelen = 0);
    TextIterator iterate_line_offset(ULONG offset_chars, ULONG *lineno, ULONG *linestart = 0);

    ULONG getdata(ULONG offset, BYTE *buf, size_t len);
    ULONG getline(ULONG nLineNo, LPTSTR buf, ULONG buflen, ULONG *off_chars);

    int   getformat(void);
    ULONG linecount(void);
    ULONG longestline(int tabwidth);
    ULONG size(void);

//private:

    bool init_linebuffer(void);

    ULONG charoffset_to_byteoffset(ULONG offset_chars);
    ULONG byteoffset_to_charoffset(ULONG offset_bytes);

    ULONG count_chars(ULONG offset_bytes, ULONG length_chars);

    size_t utf16_to_rawdata(LPTSTR utf16str, size_t utf16len, BYTE *rawdata, size_t *rawlen);
    size_t rawdata_to_utf16(BYTE *rawdata, size_t rawlen, LPTSTR utf16str, size_t *utf16len);

    int   detect_file_format(int *headersize);
    ULONG      gettext(ULONG offset, ULONG lenbytes, LPTSTR buf, ULONG *len);
    int   getchar(ULONG offset, ULONG lenbytes, ULONG *pch32);

    // UTF-16 text-editing interface
    ULONG    insert_raw(ULONG offset_bytes, LPTSTR text, ULONG length);
    ULONG    replace_raw(ULONG offset_bytes, LPTSTR text, ULONG length, ULONG erase_len);
    ULONG    erase_raw(ULONG offset_bytes, ULONG length);


    sequence * m_seq;

    ULONG m_nDocLength_chars;
    ULONG m_nDocLength_bytes;

    ULONG * m_pLineBuf_byte;
    ULONG * m_pLineBuf_char;
    ULONG  m_nNumLines;

    int m_nFileFormat;
    int m_nHeaderSize;
};

TextIterator * new_TextIterator(
    ULONG off = 0,
    ULONG len = 0,
    TextDocument * td = 0
    );
TextIterator * new_TextIterator(
    const TextIterator & ti
    );
TextIterator * copy_TextIterator(
    TextIterator * lps,
    const TextIterator & ti
    );
void delete_TextIterator(
    TextIterator * lps
    );
ULONG gettext_TextIterator(
    TextIterator * lps,
    LPTSTR buf,
    ULONG buflen
    );
bool valid_TextIterator(
    TextIterator * lps
    );
struct _TextIterator
{
    TextDocument * text_doc;

    ULONG off_bytes;
    ULONG len_bytes;
};

struct _BOM_LOOKUP
{
    DWORD bom;
    ULONG len;
    int type;
};

#endif