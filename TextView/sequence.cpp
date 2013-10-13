/*
    sequence.cpp

    data-sequence class

    Copyright J Brown 1999-2006
    www.catch22.net

    Freeware
    */
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "sequence.h"


void clear_eventstack(
    eventstack * dest
    )
{
    for (size_t i = 0; i < dest->size(); i++)
    {
        free_span_range((*dest)[i]);
        delete_span_range((*dest)[i]);
    }

    dest->clear();
}

sequence * new_sequence()
{
    sequence * lps = (sequence *) malloc(sizeof(sequence));

    if (lps)
    {
        memset(lps, 0, sizeof(*lps));

        lps->undostack = new eventstack;
        lps->redostack = new eventstack;

        record_action_sequence(lps, action_invalid, 0);

        lps->head = lps->tail = 0;
        lps->sequence_length = 0;
        lps->group_id = 0;
        lps->group_refcount = 0;

        lps->head = new_span(0, 0, 0);
        lps->tail = new_span(0, 0, 0);
        lps->head->next = lps->tail;
        lps->tail->prev = lps->head;
    }

    return lps;
}

void delete_sequence(
    sequence * lps
    )
{
    clear_sequence(lps);

    delete_span(lps->head);
    delete_span(lps->tail);

    delete lps->undostack;
    delete lps->redostack;

    free(lps);
}

bool init_sequence(
    sequence * lps
    )
{
    lps->sequence_length = 0;

    if (!alloc_modifybuffer_sequence(lps, 0x10000))
    {
        return false;
    }

    record_action_sequence(lps, action_invalid, 0);
    lps->group_id = 0;
    lps->group_refcount = 0;
    lps->undoredo_index = 0;
    lps->undoredo_length = 0;

    return true;
}

bool init_sequence(
    sequence * lps,
    const seqchar * buffer,
    size_t length
    )
{
    clear_sequence(lps);

    if (!init_sequence(lps))
    {
        return false;
    }

    buffer_control *bc = alloc_modifybuffer_sequence(lps, length);
    memcpy(bc->buffer, buffer, length * sizeof(seqchar));
    bc->length = length;

    span *sptr = new_span(0, length, bc->id, lps->tail, lps->head);
    lps->head->next = sptr;
    lps->tail->prev = sptr;

    lps->sequence_length = length;
    return true;
}

//
//    Initialize from an on-disk file
//
bool open_sequence(
    sequence * lps,
    LPCTSTR filename,
    bool readonly
    )
{
    return false;
}

//
//    Initialize from an on-disk file
//
bool save_sequence(
    sequence * lps,
    LPCTSTR filename
    )
{
    return false;
}

//
//    Allocate a buffer and add it to our 'buffer control' list
//
buffer_control * alloc_buffer_sequence(
    sequence * lps,
    size_t maxsize
    )
{
    buffer_control * bc;

    if ((bc = new_buffer_control()) == 0)
        return 0;

    // allocate a new buffer of byte/wchar/long/whatever
    if ((bc->buffer = (seqchar *) malloc(sizeof(seqchar) * maxsize)) == 0)
    {
        delete_buffer_control(bc);
        return 0;
    }

    bc->length = 0;
    bc->maxsize = maxsize;
    // assign the id
    bc->id = lps->buffer_list.size();

    lps->buffer_list.push_back(bc);

    return bc;
}

buffer_control * alloc_modifybuffer_sequence(
    sequence * lps,
    size_t maxsize
    )
{
    buffer_control * bc;

    if ((bc = alloc_buffer_sequence(lps, maxsize)) == 0)
        return 0;

    lps->modifybuffer_id = bc->id;
    lps->modifybuffer_pos = 0;

    return bc;
}

//
//    Import the specified range of data into the sequence so we have our own private copy
//
bool import_buffer_sequence(
    sequence * lps,
    const seqchar * buf,
    size_t len,
    size_t * buffer_offset
    )
{
    buffer_control * bc;

    // get the current modify-buffer
    bc = lps->buffer_list[lps->modifybuffer_id];

    // if there isn't room then allocate a new modify-buffer
    if (bc->length + len >= bc->maxsize)
    {
        bc = alloc_modifybuffer_sequence(lps, len + 0x10000);

        // make sure that no old spans use this buffer
        record_action_sequence(lps, action_invalid, 0);
    }

    if (bc == 0)
        return false;

    // import the data
    memcpy(bc->buffer + bc->length, buf, len * sizeof(seqchar));

    *buffer_offset = bc->length;
    bc->length += len;

    return true;
}

//
//    sequence::spanfromindex
//
//    search the spanlist for the span which encompasses the specified index position
//
//    index        - character-position index
//    *spanindex  - index of span within sequence
//
span * spanfromindex_sequence(
    sequence * lps,
    size_w index,
    size_w * spanindex
    )
{
    span * sptr;
    size_w curidx = 0;

    // scan the list looking for the span which holds the specified index
    for (sptr = lps->head->next; sptr->next; sptr = sptr->next)
    {
        if (index >= curidx && index < curidx + sptr->length)
        {
            if (spanindex)
                *spanindex = curidx;

            return sptr;
        }

        curidx += sptr->length;
    }

    // insert at tail
    if (sptr && index == curidx)
    {
        *spanindex = curidx;
        return sptr;
    }

    return 0;
}

void restore_spanrange_sequence(
    sequence * lps,
    span_range * range,
    bool undo_or_redo
    )
{
    if (range->boundary)
    {
        span * first = range->first->next;
        span * last = range->last->prev;

        // unlink spans from main list
        range->first->next = range->last;
        range->last->prev = range->first;

        // store the span range we just removed
        range->first = first;
        range->last = last;
        range->boundary = false;
    }
    else
    {
        span * first = range->first->prev;
        span * last = range->last->next;

        // are we moving spans into an "empty" region?
        // (i.e. inbetween two adjacent spans)
        if (first->next == last)
        {
            // move the old spans back into the empty region
            first->next = range->first;
            last->prev = range->last;

            // store the span range we just removed
            range->first = first;
            range->last = last;
            range->boundary = true;
        }
        // we are replacing a range of spans in the list,
        // so swap the spans in the list with the one's in our "undo" event
        else
        {
            // find the span range that is currently in the list
            first = first->next;
            last = last->prev;

            // unlink the the spans from the main list
            first->prev->next = range->first;
            last->next->prev = range->last;

            // store the span range we just removed
            range->first = first;
            range->last = last;
            range->boundary = false;
        }
    }

    // update the 'sequence length' and 'quicksave' states
    std::swap(range->sequence_length, lps->sequence_length);
    std::swap(range->quicksave, lps->can_quicksave);

    lps->undoredo_index = range->index;

    if ((range->act == action_erase && undo_or_redo == true) || (range->act != action_erase && undo_or_redo == false))
    {
        lps->undoredo_length = range->length;
    }
    else
    {
        lps->undoredo_length = 0;
    }
}

//
//    sequence::undoredo
//
//    private routine used to undo/redo spanrange events to/from 
//    the sequence - handles 'grouped' events
//
bool undoredo_sequence(
    sequence * lps,
    eventstack * source,
    eventstack * dest
    )
{
    span_range * range = 0;
    size_t group_id;

    if (source->empty())
        return false;

    // make sure that no "optimized" actions can occur
    record_action_sequence(lps, action_invalid, 0);

    group_id = source->back()->group_id;

    do
    {
        // remove the next event from the source stack
        range = source->back();
        source->pop_back();

        // add event onto the destination stack
        dest->push_back(range);

        // do the actual work
        restore_spanrange_sequence(lps, range, source == lps->undostack ? true : false);
    } while (!source->empty() && (source->back()->group_id == group_id && group_id != 0));

    return true;
}

// 
//    UNDO the last action
//
bool undo_sequence(
    sequence * lps
    )
{
    return undoredo_sequence(lps, lps->undostack, lps->redostack);
}

//
//    REDO the last UNDO
//
bool redo_sequence(
    sequence * lps
    )
{
    return undoredo_sequence(lps, lps->redostack, lps->undostack);
}

//
//    Will calling sequence::undo change the sequence?
//
bool canundo_sequence(
    sequence * lps
    )
{
    return lps->undostack->size() != 0;
}

//
//    Will calling sequence::redo change the sequence?
//
bool canredo_sequence(
    sequence * lps
    )
{
    return lps->redostack->size() != 0;
}

//
//    Group repeated actions on the sequence (insert/erase etc)
//    into a single 'undoable' action
//
void group_sequence(
    sequence * lps
    )
{
    if (lps->group_refcount == 0)
    {
        if (++lps->group_id == 0)
            ++lps->group_id;

        lps->group_refcount++;
    }
}

//
//    Close the grouping
//
void ungroup_sequence(
    sequence * lps
    )
{
    if (lps->group_refcount > 0)
        lps->group_refcount--;
}

//
//    Return logical length of the sequence
//
size_w size_sequence(
    sequence * lps
    )
{
    return lps->sequence_length;
}

//
//    sequence::initundo
//
//    create a new (empty) span range and save the current sequence state
//
span_range * initundo_sequence(
    sequence * lps,
    size_w index,
    size_w length,
    action act
    )
{
    span_range * sr = new_span_range(lps->sequence_length, index, length, act, lps->can_quicksave, lps->group_refcount ? lps->group_id : 0);

    lps->undostack->push_back(sr);

    return sr;
}

span_range * stackback_sequence(
    sequence * lps,
    eventstack * source,
    size_t idx
    )
{
    size_t length = source->size();

    if (length > 0 && idx < length)
    {
        return (*source)[length - idx - 1];
    }
    else
    {
        return 0;
    }
}

void record_action_sequence(
    sequence * lps,
    action act,
    size_w index
    )
{
    lps->lastaction_index = index;
    lps->lastaction = act;
}

bool can_optimize_sequence(
    sequence * lps,
    action act,
    size_w index
    )
{
    return (lps->lastaction == act && lps->lastaction_index == index);
}

//
//    sequence::insert_worker
//
bool insert_worker_sequence(
    sequence * lps,
    size_w index,
    const seqchar * buf,
    size_w length,
    action act
    )
{
    span * sptr;
    size_w spanindex;
    size_t modbuf_offset;
    span_range * newspans = new_span_range();
    size_w insoffset;

    if (index > lps->sequence_length)
    {
        return false;
    }

    // find the span that the insertion starts at
    if ((sptr = spanfromindex_sequence(lps, index, &spanindex)) == 0)
    {
        return false;
    }

    // ensure there is room in the modify buffer...
    // allocate a new buffer if necessary and then invalidate span cache
    // to prevent a span using two buffers of data
    if (!import_buffer_sequence(lps, buf, length, &modbuf_offset))
    {
        return false;
    }

    clear_eventstack(lps->redostack);
    insoffset = index - spanindex;

    // special-case #1: inserting at the end of a prior insertion, at a span-boundary
    if (insoffset == 0 && can_optimize_sequence(lps, act, index))
    {
        // simply extend the last span's length
        span_range * sr = lps->undostack->back();
        sptr->prev->length += length;
        sr->length += length;
    }
    // general-case #1: inserting at a span boundary?
    else if (insoffset == 0)
    {
        //
        // Create a new undo event; because we are inserting at a span
        // boundary there are no spans to replace, so use a "span boundary"
        //
        span_range * oldspans = initundo_sequence(lps, index, length, act);
        spanboundary_span_range(oldspans, sptr->prev, sptr);

        // allocate new span in the modify buffer
        append_span_range(newspans, new_span(modbuf_offset, length, lps->modifybuffer_id));

        // link the span into the sequence
        swap_span_range(oldspans, newspans);
    }
    // general-case #2: inserting in the middle of a span
    else
    {
        //
        // Create a new undo event and add the span
        // that we will be "splitting" in half
        //
        span_range * oldspans = initundo_sequence(lps, index, length, act);
        append_span_range(oldspans, sptr);

        // span for the existing data before the insertion
        append_span_range(newspans, new_span(sptr->offset, insoffset, sptr->buffer));

        // make a span for the inserted data
        append_span_range(newspans, new_span(modbuf_offset, length, lps->modifybuffer_id));

        // span for the existing data after the insertion
        append_span_range(newspans, new_span(sptr->offset + insoffset, sptr->length - insoffset, sptr->buffer));

        swap_span_range(oldspans, newspans);
    }

    lps->sequence_length += length;

    return true;
}

//
//    sequence::insert
//
//    Insert a buffer into the sequence at the specified position.
//    Consecutive insertions are optimized into a single event
//
bool insert_sequence(
    sequence * lps,
    size_w index,
    const seqchar * buf,
    size_w length
    )
{
    if (insert_worker_sequence(lps, index, buf, length, action_insert))
    {
        record_action_sequence(lps, action_insert, index + length);
        return true;
    }
    else
    {
        return false;
    }
}

//
//    sequence::insert
//
//    Insert specified character-value into sequence
//
bool insert_sequence(
    sequence * lps,
    size_w index,
    const seqchar val
    )
{
    return insert_sequence(lps, index, &val, 1);
}

//
//    sequence::deletefromsequence
//
//    Remove + delete the specified *span* from the sequence
//
void deletefromsequence_sequence(
    sequence * lps,
    span* * psptr
    )
{
    span * sptr = *psptr;
    sptr->prev->next = sptr->next;
    sptr->next->prev = sptr->prev;

    memset(sptr, 0, sizeof(span));
    delete_span(sptr);
    *psptr = 0;
}

//
//    sequence::erase_worker
//
bool erase_worker_sequence(
    sequence * lps,
    size_w index,
    size_w length,
    action act
    )
{
    span * sptr;
    span_range * oldspans = new_span_range();
    span_range * newspans = new_span_range();
    span_range * event;
    size_w spanindex;
    size_w remoffset;
    size_w removelen;
    bool append_spanrange;

    // make sure we stay within the range of the sequence
    if (length == 0 || length > lps->sequence_length || index > lps->sequence_length - length)
    {
        return false;
    }

    // find the span that the deletion starts at
    if ((sptr = spanfromindex_sequence(lps, index, &spanindex)) == 0)
    {
        return false;
    }

    // work out the offset relative to the start of the *span*
    remoffset = index - spanindex;
    removelen = length;

    //
    //    can we optimize?
    //
    //    special-case 1: 'forward-delete'
    //    erase+replace operations will pass through here
    //
    if (index == spanindex && can_optimize_sequence(lps, act, index))
    {
        event = stackback_sequence(lps, lps->undostack, act == action_replace ? 1 : 0);
        event->length += length;
        append_spanrange = true;

        if (lps->frag2 != 0)
        {
            if (length < lps->frag2->length)
            {
                lps->frag2->length -= length;
                lps->frag2->offset += length;
                lps->sequence_length -= length;
                return true;
            }
            else
            {
                if (act == action_replace)
                    stackback_sequence(lps, lps->undostack, 0)->last = lps->frag2->next;

                removelen -= sptr->length;
                sptr = sptr->next;
                deletefromsequence_sequence(lps, &lps->frag2);
            }
        }
    }
    //
    //    special-case 2: 'backward-delete'
    //    only erase operations can pass through here
    //
    else if (index + length == spanindex + sptr->length && can_optimize_sequence(lps, action_erase, index + length))
    {
        event = lps->undostack->back();
        event->length += length;
        event->index -= length;
        append_spanrange = false;

        if (lps->frag1 != 0)
        {
            if (length < lps->frag1->length)
            {
                lps->frag1->length -= length;
                lps->frag1->offset += 0;
                lps->sequence_length -= length;
                return true;
            }
            else
            {
                removelen -= lps->frag1->length;
                deletefromsequence_sequence(lps, &lps->frag1);
            }
        }
    }
    else
    {
        append_spanrange = true;
        lps->frag1 = lps->frag2 = 0;

        if ((event = initundo_sequence(lps, index, length, act)) == 0)
            return false;
    }

    //
    //    general-case 2+3
    //
    clear_eventstack(lps->redostack);

    // does the deletion *start* mid-way through a span?
    if (remoffset != 0)
    {
        // split the span - keep the first "half"
        append_span_range(newspans, new_span(sptr->offset, remoffset, sptr->buffer));
        lps->frag1 = newspans->first;

        // have we split a single span into two?
        // i.e. the deletion is completely within a single span
        if (remoffset + removelen < sptr->length)
        {
            // make a second span for the second half of the split
            append_span_range(newspans, new_span(sptr->offset + remoffset + removelen, sptr->length - remoffset - removelen, sptr->buffer));

            lps->frag2 = newspans->last;
        }

        removelen -= min(removelen, (sptr->length - remoffset));

        // archive the span we are going to replace
        append_span_range(oldspans, sptr);
        sptr = sptr->next;
    }

    // we are now on a proper span boundary, so remove
    // any further spans that the erase-range encompasses
    while (removelen > 0 && sptr != lps->tail)
    {
        // will the entire span be removed?
        if (removelen < sptr->length)
        {
            // split the span, keeping the last "half"
            append_span_range(newspans, new_span(sptr->offset + removelen, sptr->length - removelen, sptr->buffer));

            lps->frag2 = newspans->last;
        }

        removelen -= min(removelen, sptr->length);

        // archive the span we are replacing
        append_span_range(oldspans, sptr);
        sptr = sptr->next;
    }

    // for replace operations, update the undo-event for the
    // insertion so that it knows about the newly removed spans
    if (act == action_replace && !oldspans->boundary)
        stackback_sequence(lps, lps->undostack, 0)->last = oldspans->last->next;

    swap_span_range(oldspans, newspans);
    lps->sequence_length -= length;

    if (append_spanrange)
        append_span_range(event, oldspans);
    else
        prepend_span_range(event, oldspans);

    return true;
}

//
//    sequence::erase 
//
//  "removes" the specified range of data from the sequence. 
//
bool erase_sequence(
    sequence * lps,
    size_w index,
    size_w len
    )
{
    if (erase_worker_sequence(lps, index, len, action_erase))
    {
        record_action_sequence(lps, action_erase, index);
        return true;
    }
    else
    {
        return false;
    }
}

//
//    sequence::erase
//
//    remove single character from sequence
//
bool erase_sequence(
    sequence * lps,
    size_w index
    )
{
    return erase_sequence(lps, index, 1);
}

//
//    sequence::replace
//
//    A 'replace' (or 'overwrite') is a combination of erase+inserting
//  (first we erase a section of the sequence, then insert a new block
//  in it's place). 
//
//    Doing this as a distinct operation (erase+insert at the 
//  same time) is really complicated, so I just make use of the existing 
//  sequence::erase and sequence::insert and combine them into action. We
//    need to play with the undo stack to combine them in a 'true' sense.
//
bool replace_sequence(
    sequence * lps,
    size_w index,
    const seqchar * buf,
    size_w length,
    size_w erase_length
    )
{
    size_t remlen = 0;

    // make sure operation is within allowed range
    if (index > lps->sequence_length || MAX_SEQUENCE_LENGTH - index < length)
        return false;

    // for a "replace" which will overrun the sequence, make sure we 
    // only delete up to the end of the sequence
    remlen = min(lps->sequence_length - index, erase_length);

    // combine the erase+insert actions together
    group_sequence(lps);

    // first of all remove the range
    if (remlen > 0 && index < lps->sequence_length && !erase_worker_sequence(lps, index, remlen, action_replace))
    {
        ungroup_sequence(lps);
        return false;
    }

    // then insert the data
    if (insert_worker_sequence(lps, index, buf, length, action_replace))
    {
        ungroup_sequence(lps);
        record_action_sequence(lps, action_replace, index + length);
        return true;
    }
    else
    {
        // failed...cleanup what we have done so far
        ungroup_sequence(lps);
        record_action_sequence(lps, action_invalid, 0);

        span_range *range = lps->undostack->back();
        lps->undostack->pop_back();
        restore_spanrange_sequence(lps, range, true);
        delete_span_range(range);

        return false;
    }
}

//
//    sequence::replace
//
//    overwrite with the specified buffer
//
bool replace_sequence(
    sequence * lps,
    size_w index,
    const seqchar * buf,
    size_w length
    )
{
    return replace_sequence(lps, index, buf, length, length);
}

//
//    sequence::replace
//
//    overwrite with a single character-value
//
bool replace_sequence(
    sequence * lps,
    size_w index,
    const seqchar val
    )
{
    return replace_sequence(lps, index, &val, 1);
}

//
//    sequence::append
//
//    very simple wrapper around sequence::insert, just inserts at
//  the end of the sequence
//
bool append_sequence(
    sequence * lps,
    const seqchar * buf,
    size_w length
    )
{
    return insert_sequence(lps, size_sequence(lps), buf, length);
}

//
//    sequence::append
//
//    append a single character to the sequence
//
bool append_sequence(
    sequence * lps,
    const seqchar val
    )
{
    return append_sequence(lps, &val, 1);
}

//
//    sequence::clear
//
//    empty the entire sequence, clear undo/redo history etc
//
bool clear_sequence(
    sequence * lps
    )
{
    span * sptr, *tmp;

    // delete all spans in the sequence
    for (sptr = lps->head->next; sptr != lps->tail; sptr = tmp)
    {
        tmp = sptr->next;
        delete_span(sptr);
    }

    // re-link the head+tail
    lps->head->next = lps->tail;
    lps->tail->prev = lps->head;

    // delete everything in the undo/redo stacks
    clear_eventstack(lps->undostack);
    clear_eventstack(lps->redostack);

    // delete all memory-buffers
    for (size_t i = 0; i < lps->buffer_list.size(); i++)
    {
        free(lps->buffer_list[i]->buffer);
        delete_buffer_control(lps->buffer_list[i]);
    }

    lps->buffer_list.clear();
    lps->sequence_length = 0;
    return true;
}

//
//    sequence::render
//
//    render the specified range of data (index, len) and store in 'dest'
//
//    Returns number of chars copied into destination
//
size_w render_sequence(
    sequence * lps,
    size_w index,
    seqchar * dest,
    size_w length
    )
{
    size_w spanoffset = 0;
    size_w total = 0;
    span  *sptr;

    // find span to start rendering at
    if ((sptr = spanfromindex_sequence(lps, index, &spanoffset)) == 0)
    {
        return false;
    }

    // might need to start mid-way through the first span
    spanoffset = index - spanoffset;

    // copy each span's referenced data in succession
    while (length && sptr != lps->tail)
    {
        size_w copylen = min(sptr->length - spanoffset, length);
        seqchar *source = lps->buffer_list[sptr->buffer]->buffer;

        memcpy(dest, source + sptr->offset + spanoffset, copylen * sizeof(seqchar));

        dest += copylen;
        length -= copylen;
        total += copylen;

        sptr = sptr->next;
        spanoffset = 0;
    }

    return total;
}

//
//    sequence::peek
//
//    return single element at specified position in the sequence
//
seqchar peek_sequence(
    sequence * lps,
    size_w index
    )
{
    seqchar value;
    return render_sequence(lps, index, &value, 1) ? value : 0;
}

//
//    sequence::poke
//
//    modify single element at specified position in the sequence
//
bool  poke_sequence(
    sequence * lps,
    size_w index,
    seqchar value
    )
{
    return replace_sequence(lps, index, &value, 1);
}

size_w event_index_sequence(
    sequence * lps
    )
{
    return lps->undoredo_index;
}

size_w event_length_sequence(
    sequence * lps
    )
{
    return lps->undoredo_length;
}

//
//    sequence::breakopt
//
//    Prevent subsequent operations from being optimized (coalesced) 
//  with the last.
//
void breakopt_sequence(
    sequence * lps
    )
{
    lps->lastaction = action_invalid;
}

span * new_span(
    size_w off,
    size_w len,
    int buf,
    span * nx,
    span * pr
    )
{
    static int count = -2;

    span * lps = (span *) malloc(sizeof(span));

    if (lps)
    {
        memset(lps, 0, sizeof(*lps));

        lps->next = nx;
        lps->prev = pr;
        lps->offset = off;
        lps->length = len;
        lps->buffer = buf;
        lps->id = count++;
    }

    return lps;
}

void delete_span(
    span * lps
    )
{
    free(lps);
}

span_range * new_span_range(
    size_w seqlen,
    size_w idx,
    size_w len,
    action a,
    bool qs,
    size_t id
    )
{
    span_range * lps = (span_range *) malloc(sizeof(span_range));

    if (lps)
    {
        memset(lps, 0, sizeof(*lps));

        lps->first = 0;
        lps->last = 0;
        lps->boundary = true;
        lps->sequence_length = seqlen;
        lps->index = idx;
        lps->length = len;
        lps->act = a;
        lps->quicksave = qs;
        lps->group_id = id;
    }

    return lps;
}

void delete_span_range(
    span_range * lps
    )
{
    free(lps);
}

// separate 'destruction' used when appropriate
void free_span_range(
    span_range * lps
    )
{
    span *sptr, *next, *term;

    if (lps->boundary == false)
    {
        // delete the range of spans
        for (sptr = lps->first, term = lps->last->next; sptr && sptr != term; sptr = next)
        {
            next = sptr->next;
            delete_span(sptr);
        }
    }
}

// add a span into the range
void append_span_range(
    span_range * lps,
    span * sptr
    )
{
    if (sptr != 0)
    {
        // first time a span has been added?
        if (lps->first == 0)
        {
            lps->first = sptr;
        }
        // otherwise chain the spans together.
        else
        {
            lps->last->next = sptr;
            sptr->prev = lps->last;
        }

        lps->last = sptr;
        lps->boundary = false;
    }
}

// join two span-ranges together
void append_span_range(
    span_range * lps,
    span_range * range
    )
{
    if (range->boundary == false)
    {
        if (lps->boundary)
        {
            lps->first = range->first;
            lps->last = range->last;
            lps->boundary = false;
        }
        else
        {
            range->first->prev = lps->last;
            lps->last->next = range->first;
            lps->last = range->last;
        }
    }
}

// join two span-ranges together. used only for 'back-delete'
void prepend_span_range(
    span_range * lps,
    span_range * range
    )
{
    if (range->boundary == false)
    {
        if (lps->boundary)
        {
            lps->first = range->first;
            lps->last = range->last;
            lps->boundary = false;
        }
        else
        {
            range->last->next = lps->first;
            lps->first->prev = range->last;
            lps->first = range->first;
        }
    }
}

// An 'empty' range is represented by storing pointers to the
// spans ***either side*** of the span-boundary position. Input is
// always the span following the boundary.
void spanboundary_span_range(
    span_range * lps,
    span * before,
    span * after
    )
{
    lps->first = before;
    lps->last = after;
    lps->boundary = true;
}

void swap_span_range(
    span_range * src,
    span_range * dest
    )
{
    if (src->boundary)
    {
        if (!dest->boundary)
        {
            src->first->next = dest->first;
            src->last->prev = dest->last;
            dest->first->prev = src->first;
            dest->last->next = src->last;
        }
    }
    else
    {
        if (dest->boundary)
        {
            src->first->prev->next = src->last->next;
            src->last->next->prev = src->first->prev;
        }
        else
        {
            src->first->prev->next = dest->first;
            src->last->next->prev = dest->last;
            dest->first->prev = src->first->prev;
            dest->last->next = src->last->next;
        }
    }
}

ref * new_ref(
    sequence * seq,
    size_w index
    )
{
    ref * lps = (ref *) malloc(sizeof(ref));

    if (lps)
    {
        memset(lps, 0, sizeof(*lps));

        lps->seq = seq;
        lps->index = index;
    }

    return lps;
}

void delete_ref(
    ref * lps
    )
{
    free(lps);
}

buffer_control * new_buffer_control()
{
    buffer_control * lps = (buffer_control *) malloc(sizeof(buffer_control));

    if (lps)
    {
        memset(lps, 0, sizeof(*lps));
    }

    return lps;
}

void delete_buffer_control(
    buffer_control * lps
    )
{
    free(lps);
}