/* slotted_page.c: Implementation of Slotted-Page insert, delete, and scan */
#include <stdio.h>
#include <string.h>
#include "slotted_page.h"

static int last_page = -1;

void SP_ResetLastPage(void) {
    last_page = -1;
}

/* Insert a variable-length record into the slotted-page file */
int SP_InsertRecord(fd, rec, len, rid)
int fd;
char *rec;
int len;
RID *rid;
{
    char *pagebuf;
    PageHeader *hdr;
    SlotEntry *slots;
    int pagenum, err, free_space, needed;

    needed = len + (int)sizeof(SlotEntry);

    /* Try inserting into the last used page first */
    if (last_page != -1) {
        if ((err = PF_GetThisPage(fd, last_page, &pagebuf)) == PFE_OK) {
            hdr = (PageHeader *)pagebuf;
            free_space = hdr->free_space_offset -
                         ((int)sizeof(PageHeader) + hdr->num_slots * (int)sizeof(SlotEntry));
            if (free_space >= needed) {
                slots = (SlotEntry *)(pagebuf + sizeof(PageHeader));
                hdr->free_space_offset -= len;
                memcpy(pagebuf + hdr->free_space_offset, rec, len);
                slots[hdr->num_slots].offset = hdr->free_space_offset;
                slots[hdr->num_slots].length = len;
                rid->page_num = last_page;
                rid->slot_num = hdr->num_slots;
                hdr->num_slots++;
                PF_MarkDirty(fd, last_page);
                PF_UnfixPage(fd, last_page, TRUE);
                return PFE_OK;
            }
            PF_UnfixPage(fd, last_page, FALSE);
        }
    }

    /* Otherwise, allocate a new page */
    if ((err = PF_AllocPage(fd, &pagenum, &pagebuf)) != PFE_OK) {
        return err;
    }

    hdr = (PageHeader *)pagebuf;
    hdr->num_slots = 0;
    hdr->free_space_offset = PF_PAGE_SIZE;

    slots = (SlotEntry *)(pagebuf + sizeof(PageHeader));
    hdr->free_space_offset -= len;
    memcpy(pagebuf + hdr->free_space_offset, rec, len);
    slots[0].offset = hdr->free_space_offset;
    slots[0].length = len;
    hdr->num_slots = 1;

    rid->page_num = pagenum;
    rid->slot_num = 0;
    last_page = pagenum;

    PF_MarkDirty(fd, pagenum);
    PF_UnfixPage(fd, pagenum, TRUE);
    return PFE_OK;
}

/* Delete a record by marking its slot offset as -1 (tombstone) */
int SP_DeleteRecord(fd, rid)
int fd;
RID rid;
{
    char *pagebuf;
    PageHeader *hdr;
    SlotEntry *slots;
    int err;

    if ((err = PF_GetThisPage(fd, rid.page_num, &pagebuf)) != PFE_OK) {
        return err;
    }

    hdr = (PageHeader *)pagebuf;
    if (rid.slot_num < 0 || rid.slot_num >= hdr->num_slots) {
        PF_UnfixPage(fd, rid.page_num, FALSE);
        return PFE_INVALIDPAGE;
    }

    slots = (SlotEntry *)(pagebuf + sizeof(PageHeader));
    slots[rid.slot_num].offset = -1;
    slots[rid.slot_num].length = 0;

    PF_MarkDirty(fd, rid.page_num);
    PF_UnfixPage(fd, rid.page_num, TRUE);
    return PFE_OK;
}

/* Sequentially scan all valid records across all pages */
int SP_Scan(fd, print_limit)
int fd;
int print_limit;
{
    int pagenum, err, i, count = 0;
    char *pagebuf;
    char temp[1024];
    PageHeader *hdr;
    SlotEntry *slots;

    err = PF_GetFirstPage(fd, &pagenum, &pagebuf);
    while (err == PFE_OK) {
        hdr = (PageHeader *)pagebuf;
        slots = (SlotEntry *)(pagebuf + sizeof(PageHeader));
        for (i = 0; i < hdr->num_slots; i++) {
            if (slots[i].offset != -1) {
                if (count < print_limit) {
                    int copy_len = slots[i].length < 1023 ? slots[i].length : 1023;
                    memcpy(temp, pagebuf + slots[i].offset, copy_len);
                    temp[copy_len] = '\0';
                    printf("  [Page %d, Slot %d] %s\n", pagenum, i, temp);
                }
                count++;
            }
        }
        PF_UnfixPage(fd, pagenum, FALSE);
        err = PF_GetNextPage(fd, &pagenum, &pagebuf);
    }
    return count;
}
