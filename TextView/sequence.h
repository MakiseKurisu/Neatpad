#ifndef SEQUENCE_INCLUDED
#define SEQUENCE_INCLUDED

#include <vector>

//
//    Define the underlying string/character type of the sequence.
//
//    'seqchar' can be redefined to BYTE, WCHAR, ULONG etc 
//    depending on what kind of string you want your sequence to hold
//
typedef unsigned char seqchar;

#ifdef SEQUENCE64
typedef unsigned __int64 size_w;
#else
typedef unsigned long size_w;
#endif

const size_w MAX_SEQUENCE_LENGTH = ((size_w) (-1) / sizeof(seqchar));

struct _sequence;
typedef struct _sequence sequence;
enum _action;
typedef enum _action action;
struct _span;
typedef struct _span span;
struct _span_range;
typedef struct _span_range span_range;
struct _ref;
typedef struct _ref ref;
struct _buffer_control;
typedef struct _buffer_control buffer_control;

typedef std::vector<span_range*> eventstack;
void clear_eventstack_sequence(
    sequence * lps,
    eventstack & dest
    );

typedef std::vector<buffer_control*> bufferlist;

//
//    sequence class!
//
sequence * new_sequence();
void delete_sequence(
    sequence * lps
    );
bool init_sequence(
    sequence * lps
    );
bool init_sequence(
    sequence * lps,
    const seqchar * buffer,
    size_t length
    );
bool open_sequence(
    sequence * lps,
    LPCTSTR filename,
    bool readonly
    );
bool save_sequence(
    sequence * lps,
    LPCTSTR filename
    );
buffer_control * alloc_buffer_sequence(
    sequence * lps,
    size_t maxsize
    );
buffer_control * alloc_modifybuffer_sequence(
    sequence * lps,
    size_t maxsize
    );
bool import_buffer_sequence(
    sequence * lps,
    const seqchar * buf,
    size_t len,
    size_t * buffer_offset
    );
span * spanfromindex_sequence(
    sequence * lps,
    size_w index,
    size_w * spanindex = 0
    );
void restore_spanrange_sequence(
    sequence * lps,
    span_range * range,
    bool undo_or_redo
    );
bool undoredo_sequence(
    sequence * lps,
    eventstack & source,
    eventstack & dest
    );
bool undo_sequence(
    sequence * lps
    );
bool redo_sequence(
    sequence * lps
    );
bool canundo_sequence(
    sequence * lps
    );
bool canredo_sequence(
    sequence * lps
    );
void group_sequence(
    sequence * lps
    );
void ungroup_sequence(
    sequence * lps
    );
size_w size_sequence(
    sequence * lps
    );
span_range * initundo_sequence(
    sequence * lps,
    size_w index,
    size_w length,
    action act
    );
span_range * stackback_sequence(
    sequence * lps,
    eventstack & source,
    size_t idx
    );
void record_action_sequence(
    sequence * lps,
    action act,
    size_w index
    );
bool can_optimize_sequence(
    sequence * lps,
    action act,
    size_w index
    );
bool insert_worker_sequence(
    sequence * lps,
    size_w index,
    const seqchar * buf,
    size_w length,
    action act
    );
bool insert_sequence(
    sequence * lps,
    size_w index,
    const seqchar * buf,
    size_w length
    );
bool insert_sequence(
    sequence * lps,
    size_w index,
    const seqchar val
    );
void deletefromsequence_sequence(
    sequence * lps,
    span* * psptr
    );
bool erase_worker_sequence(
    sequence * lps,
    size_w index,
    size_w length,
    action act
    );
bool erase_sequence(
    sequence * lps,
    size_w index,
    size_w len
    );
bool erase_sequence(
    sequence * lps,
    size_w index
    );
bool replace_sequence(
    sequence * lps,
    size_w index,
    const seqchar * buf,
    size_w length,
    size_w erase_length
    );
bool replace_sequence(
    sequence * lps,
    size_w index,
    const seqchar * buf,
    size_w length
    );
bool replace_sequence(
    sequence * lps,
    size_w index,
    const seqchar val
    );
bool append_sequence(
    sequence * lps,
    const seqchar * buf,
    size_w length
    );
bool append_sequence(
    sequence * lps,
    const seqchar val
    );
bool clear_sequence(
    sequence * lps
    );
size_w render_sequence(
    sequence * lps,
    size_w index,
    seqchar * dest,
    size_w length
    );
seqchar peek_sequence(
    sequence * lps,
    size_w index
    );
bool  poke_sequence(
    sequence * lps,
    size_w index,
    seqchar value
    );
void breakopt_sequence(
    sequence * lps
    );
struct _sequence
{
    //
    //    Span-table management
    //
    size_w sequence_length;
    span * head;
    span * tail;
    span * frag1;
    span * frag2;

    //
    //    Undo and redo stacks
    //
    eventstack undostack;
    eventstack redostack;
    size_t group_id;
    size_t group_refcount;
    size_w undoredo_index;
    size_w undoredo_length;

    //
    //    File and memory buffer management
    //
    bufferlist buffer_list;
    int modifybuffer_id;
    int modifybuffer_pos;

    //
    //    Sequence manipulation
    //
    size_w lastaction_index;
    action lastaction;
    bool can_quicksave;
};

//
//    action
//
//    enumeration of the type of 'edit actions' our sequence supports.
//    only important when we try to 'optimize' repeated operations on the
//    sequence by coallescing them into a single span.
//
enum _action
{
    action_invalid,
    action_insert,
    action_erase,
    action_replace
};

//
//    span
//
//    private class to the sequence
//
span * new_span(
    size_w off = 0,
    size_w len = 0,
    int buf = 0,
    span * nx = 0,
    span * pr = 0
    );
void delete_span(
    span * lps
    );
struct _span
{
    span * next;
    span * prev;    // double-link-list 

    size_w offset;
    size_w length;
    int buffer;

    int id;
};

//
//    span_range
//
//    private class to the sequence. Used to represent a contiguous range of spans.
//    used by the undo/redo stacks to store state. A span-range effectively represents
//    the range of spans affected by an event (operation) on the sequence
//  
//
span_range * new_span_range(
    size_w seqlen = 0,
    size_w idx = 0,
    size_w len = 0,
    action a = action_invalid,
    bool qs = false,
    size_t id = 0
    );
void delete_span_range(
    span_range * lps
    );
void free_span_range(
    span_range * lps
    );
void append_span_range(
    span_range * lps,
    span * sptr
    );
void append_span_range(
    span_range * lps,
    span_range * range
    );
void prepend_span_range(
    span_range * lps,
    span_range * range
    );
void spanboundary_span_range(
    span_range * lps,
    span * before,
    span * after
    );
void swap_span_range(
    span_range * src,
    span_range * dest
    );
struct _span_range
{
    // the span range
    span * first;
    span * last;
    bool boundary;

    // sequence state
    size_w sequence_length;
    size_w index;
    size_w length;
    action act;
    bool quicksave;
    size_t group_id;
};

//
//    ref
//
//    temporary 'reference' to the sequence, used for
//  non-const array access with sequence::operator[]
//
ref * new_ref(
    sequence * seq,
    size_w index
    );
void delete_ref(
    ref * lps
    );
struct _ref
{
    size_w index;
    sequence * seq;
};

//
//    buffer_control
//
struct _buffer_control
{
    seqchar * buffer;
    size_w  length;
    size_w  maxsize;
    int     id;
};

#endif