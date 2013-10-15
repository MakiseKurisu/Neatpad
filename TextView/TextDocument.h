#ifndef TEXTDOC_INCLUDED
#define TEXTDOC_INCLUDED

#include "codepages.h"
#include "sequence.h"

struct _TextIterator;
typedef struct _TextIterator TextIterator;
struct _TextDocument;
typedef struct _TextDocument TextDocument;

TextDocument * new_TextDocument(void);
void delete_TextDocument(
    TextDocument * lps
    );
bool init_TextDocument(
    TextDocument * lps,
    LPTSTR filename
    );
bool init_TextDocument(
    TextDocument * lps,
    HANDLE hFile
    );
bool save_TextDocument(
    TextDocument * lps,
    LPTSTR filename
    );
int detect_file_format_TextDocument(
    TextDocument * lps
    );
bool clear_TextDocument(
    TextDocument * lps
    );
bool EmptyDoc_TextDocument(
    TextDocument * lps
    );
int getchar_TextDocument(
    TextDocument * lps,
    ULONG offset,
    ULONG lenbytes,
    ULONG * pch32
    );
ULONG gettext_TextDocument(
    TextDocument * lps,
    ULONG offset,
    ULONG lenbytes,
    LPTSTR buf,
    ULONG * buflen
    );
ULONG getdata_TextDocument(
    TextDocument * lps,
    ULONG offset,
    BYTE * buf,
    size_t len
    );
bool init_linebuffer_TextDocument(
    TextDocument * lps
    );
ULONG linecount_TextDocument(
    TextDocument * lps
    );
ULONG longestline_TextDocument(
    TextDocument * lps,
    int tabwidth
    );
bool lineinfo_from_lineno_TextDocument(
    TextDocument * lps,
    ULONG lineno,
    ULONG * lineoff_chars,
    ULONG * linelen_chars,
    ULONG * lineoff_bytes,
    ULONG * linelen_bytes
    );
bool lineinfo_from_offset_TextDocument(
    TextDocument * lps,
    ULONG offset_chars,
    ULONG *lineno,
    ULONG *lineoff_chars,
    ULONG *linelen_chars,
    ULONG *lineoff_bytes,
    ULONG *linelen_bytes
    );
int getformat_TextDocument(
    TextDocument * lps
    );
ULONG size_TextDocument(
    TextDocument * lps
    );
TextIterator * iterate_TextDocument(
    TextDocument * lps,
    ULONG offset_chars
    );
TextIterator * iterate_line_TextDocument(
    TextDocument * lps,
    ULONG lineno,
    ULONG * linestart = 0,
    ULONG * linelen = 0
    );
TextIterator * iterate_line_offset_TextDocument(
    TextDocument * lps,
    ULONG offset_chars,
    ULONG * lineno,
    ULONG * linestart = 0
    );
ULONG lineno_from_offset_TextDocument(
    TextDocument * lps,
    ULONG offset
    );
ULONG offset_from_lineno_TextDocument(
    TextDocument * lps,
    ULONG lineno
    );
ULONG getline_TextDocument(
    TextDocument * lps,
    ULONG nLineNo,
    LPTSTR buf,
    ULONG buflen,
    ULONG * off_chars
    );
size_t rawdata_to_utf16_TextDocument(
    TextDocument * lps,
    BYTE * rawdata,
    size_t rawlen,
    LPTSTR utf16str,
    size_t * utf16len
    );
size_t utf16_to_rawdata_TextDocument(
    TextDocument * lps,
    LPTSTR utf16str,
    size_t utf16len,
    BYTE * rawdata,
    size_t * rawlen
    );
ULONG insert_raw_TextDocument(
    TextDocument * lps,
    ULONG offset_bytes,
    LPTSTR text,
    ULONG length
    );
ULONG replace_raw_TextDocument(
    TextDocument * lps,
    ULONG offset_bytes,
    LPTSTR text,
    ULONG length,
    ULONG erase_chars
    );
ULONG erase_raw_TextDocument(
    TextDocument * lps,
    ULONG offset_bytes,
    ULONG length
    );
ULONG count_chars_TextDocument(
    TextDocument * lps,
    ULONG offset_bytes,
    ULONG length_chars
    );
ULONG byteoffset_to_charoffset_TextDocument(
    TextDocument * lps,
    ULONG offset_bytes
    );
ULONG charoffset_to_byteoffset_TextDocument(
    TextDocument * lps,
    ULONG offset_chars
    );
ULONG insert_text_TextDocument(
    TextDocument * lps,
    ULONG offset_chars,
    LPTSTR text,
    ULONG length
    );
ULONG replace_text_TextDocument(
    TextDocument * lps,
    ULONG offset_chars,
    LPTSTR text,
    ULONG length,
    ULONG erase_len
    );
ULONG erase_text_TextDocument(
    TextDocument * lps,
    ULONG offset_chars,
    ULONG length
    );
bool Undo_TextDocument(
    TextDocument * lps,
    ULONG *offset_start,
    ULONG *offset_end
    );
bool Redo_TextDocument(
    TextDocument * lps,
    ULONG *offset_start,
    ULONG *offset_end
    );

struct _TextDocument
{
    sequence * seq;

    ULONG nDocLength_chars;
    ULONG nDocLength_bytes;

    ULONG * pLineBuf_byte;
    ULONG * pLineBuf_char;
    ULONG  nNumLines;

    int nFileFormat;
    int nHeaderSize;
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