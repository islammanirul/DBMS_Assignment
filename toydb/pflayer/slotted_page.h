/* slotted_page.h: Slotted-Page storage interface built on top of PF Layer */
#ifndef SLOTTED_PAGE_H
#define SLOTTED_PAGE_H

#include "pf.h"

/* Record Identifier (Page Number + Slot Index) */
typedef struct RID {
    int page_num;
    int slot_num;
} RID;

/* Directory entry for each variable-length record */
typedef struct SlotEntry {
    int offset; /* Byte offset from start of page (-1 if deleted) */
    int length; /* Length of record in bytes */
} SlotEntry;

/* Header stored at the beginning of each slotted page */
typedef struct PageHeader {
    int num_slots;
    int free_space_offset; /* Points to the start of the lowest-offset record */
} PageHeader;

extern int SP_InsertRecord(int fd, char *rec, int len, RID *rid);
extern int SP_DeleteRecord(int fd, RID rid);
extern int SP_Scan(int fd, int print_limit);
extern void SP_ResetLastPage(void);

#endif
