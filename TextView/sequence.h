#ifndef SEQUENCE_INCLUDED
#define SEQUENCE_INCLUDED

#include <vector>

//
//    Define the underlying string/character type of the sequence.
//
//    'seqchar' can be redefined to BYTE, WCHAR, ULONG etc 
//    depending on what kind of string you want your sequence to hold
//
typedef unsigned char      seqchar;

#ifdef SEQUENCE64
typedef unsigned __int64  size_w;
#else
typedef unsigned long      size_w;
#endif

const size_w MAX_SEQUENCE_LENGTH = ((size_w) (-1) / sizeof(seqchar));

enum _action;
typedef enum _action action;
struct _span;
typedef struct _span span;
struct _span_range;
typedef struct _span_range span_range;
/*
struct _ref;
typedef struct _ref ref;
*/
struct _buffer_control;
typedef struct _buffer_control buffer_control;

//
//    sequence class!
//
class sequence
{
public:

    // sequence construction
    sequence();
    ~sequence();

    //
    // initialize with a file
    //
    bool        init();
    bool        open(TCHAR *filename, bool readonly);
    bool        clear();

    //
    // initialize from an in-memory buffer
    //
    bool        init(const seqchar *buffer, size_t length);

    //
    //    sequence statistics
    //
    size_w        size() const;

    //
    // sequence manipulation 
    //
    bool        insert(size_w index, const seqchar *buf, size_w length);
    bool        insert(size_w index, const seqchar  val, size_w count);
    bool        insert(size_w index, const seqchar  val);
    bool        replace(size_w index, const seqchar *buf, size_w length, size_w erase_length);
    bool        replace(size_w index, const seqchar *buf, size_w length);
    bool        replace(size_w index, const seqchar  val, size_w count);
    bool        replace(size_w index, const seqchar  val);
    bool        erase(size_w index, size_w len);
    bool        erase(size_w index);
    bool        append(const seqchar *buf, size_w len);
    bool        append(const seqchar val);
    void        breakopt();

    //
    // undo/redo support
    //
    bool        undo();
    bool        redo();
    bool        canundo() const;
    bool        canredo() const;
    void        group();
    void        ungroup();
    size_w        event_index() const  { return undoredo_index; }
    size_w        event_length() const { return undoredo_length; }

    // print out the sequence
    void        debug1();
    void        debug2();

    //
    // access and iteration
    //
    size_w        render(size_w index, seqchar *buf, size_w len) const;
    seqchar        peek(size_w index) const;
    bool        poke(size_w index, seqchar val);

    //seqchar        operator[] (size_w index) const;
    //ref            operator[] (size_w index);

private:

    typedef            std::vector<span_range*>      eventstack;
    typedef            std::vector<buffer_control*>  bufferlist;
    template <class type> void clear_vector(type &source);

    //
    //    Span-table management
    //
    void            deletefromsequence(span **sptr);
    span        *    spanfromindex(size_w index, size_w *spanindex) const;
    void            scan(span *sptr);
    size_w            sequence_length;
    span        *    head;
    span        *    tail;
    span        *    frag1;
    span        *    frag2;


    //
    //    Undo and redo stacks
    //
    span_range *    initundo(size_w index, size_w length, action act);
    void            restore_spanrange(span_range *range, bool undo_or_redo);
    //void            swap_spanrange(span_range *src, span_range *dest);
    bool            undoredo(eventstack &source, eventstack &dest);
    void            clearstack(eventstack &source);
    span_range *    stackback(eventstack &source, size_t idx);

    eventstack        undostack;
    eventstack        redostack;
    size_t            group_id;
    size_t            group_refcount;
    size_w            undoredo_index;
    size_w            undoredo_length;

    //
    //    File and memory buffer management
    //
    buffer_control *alloc_buffer(size_t size);
    buffer_control *alloc_modifybuffer(size_t size);
    bool            import_buffer(const seqchar *buf, size_t len, size_t *buffer_offset);

    bufferlist        buffer_list;
    int                modifybuffer_id;
    int                modifybuffer_pos;

    //
    //    Sequence manipulation
    //
    bool            insert_worker(size_w index, const seqchar *buf, size_w len, action act);
    bool            erase_worker(size_w index, size_w len, action act);
    bool            can_optimize(action act, size_w index);
    void            record_action(action act, size_w index);

    size_w            lastaction_index;
    action            lastaction;
    bool            can_quicksave;

    void            LOCK();
    void            UNLOCK();


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
struct _span
{
    span   *next;
    span   *prev;    // double-link-list 

    size_w  offset;
    size_w  length;
    int     buffer;

    int        id;
};



//
//    span_range
//
//    private class to the sequence. Used to represent a contiguous range of spans.
//    used by the undo/redo stacks to store state. A span-range effectively represents
//    the range of spans affected by an event (operation) on the sequence
//  
//
struct _span_range
{
    // the span range
    span    *first;
    span    *last;
    bool     boundary;

    // sequence state
    size_w     sequence_length;
    size_w     index;
    size_w     length;
    action     act;
    bool     quicksave;
    size_t     group_id;
};

/*
//
//    ref
//
//    temporary 'reference' to the sequence, used for
//  non-const array access with sequence::operator[]
//
struct _ref
{
    size_w      index;
    sequence    *seq;
};
*/

//
//    buffer_control
//
struct _buffer_control
{
    seqchar *buffer;
    size_w  length;
    size_w  maxsize;
    int     id;
};



#endif