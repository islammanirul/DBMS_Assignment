/* test_index.c: Compare 3 B+ Tree Index Construction Methods on student.txt */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "am.h"
#include "../pflayer/slotted_page.h"

#define DATA_FILE "../../data/student.txt"
#define MAX_RECORDS 25000

typedef struct KeyRec {
    int key;
    int recId;
} KeyRec;

int cmp_keyrec(const void *a, const void *b) {
    int ka = ((const KeyRec *)a)->key;
    int kb = ((const KeyRec *)b)->key;
    return (ka > kb) - (ka < kb);
}

/* Helper to encode RID into a single integer for AM layer */
int encode_rid(RID r) {
    return (r.page_num << 10) | (r.slot_num & 0x3FF);
}

/* Bottom-up Bulk-Loading of a B+ Tree from pre-sorted (key, recId) pairs */
int AM_BulkLoad(char *index_fname, KeyRec *entries, int count) {
    int fd, err, i, pagenum;
    char *pagebuf;
    int leaf_pages[2000];
    int leaf_first_keys[2000];
    int num_leaves = 0;

    /* Max keys per leaf (90% fill factor) */
    int rec_entry_size = sizeof(int) + sizeof(short) + sizeof(int) + sizeof(short);
    int max_leaf_keys = ((PF_PAGE_SIZE - (int)sizeof(AM_LEAFHEADER)) / rec_entry_size) * 9 / 10;
    if (max_leaf_keys < 1) max_leaf_keys = 1;

    unlink(index_fname);
    if ((err = PF_CreateFile(index_fname)) != PFE_OK) return err;
    if ((fd = PF_OpenFile(index_fname)) < 0) return fd;

    /* 1. Pack sorted entries directly into Leaf Pages */
    i = 0;
    while (i < count) {
        AM_LEAFHEADER *lhdr;
        int keys_in_page = 0;
        short rec_ptr = PF_PAGE_SIZE;
        short key_ptr = (short)sizeof(AM_LEAFHEADER);

        if ((err = PF_AllocPage(fd, &pagenum, &pagebuf)) != PFE_OK) return err;
        memset(pagebuf, 0, PF_PAGE_SIZE);

        lhdr = (AM_LEAFHEADER *)pagebuf;
        lhdr->pageType = 'l';
        lhdr->nextLeafPage = AM_NULL_PAGE;
        lhdr->attrLength = sizeof(int);
        lhdr->maxKeys = (short)max_leaf_keys;
        lhdr->freeListPtr = 0;
        lhdr->numinfreeList = 0;

        leaf_pages[num_leaves] = pagenum;
        leaf_first_keys[num_leaves] = entries[i].key;

        while (i < count && keys_in_page < max_leaf_keys) {
            short next_rec = 0;
            /* Write recId + next_ptr at the bottom of the page */
            rec_ptr -= (short)(sizeof(int) + sizeof(short));
            memcpy(pagebuf + rec_ptr, &entries[i].recId, sizeof(int));
            memcpy(pagebuf + rec_ptr + sizeof(int), &next_rec, sizeof(short));

            /* Write key + rec_ptr at the top of the page */
            memcpy(pagebuf + key_ptr, &entries[i].key, sizeof(int));
            memcpy(pagebuf + key_ptr + sizeof(int), &rec_ptr, sizeof(short));
            key_ptr += (short)(sizeof(int) + sizeof(short));

            keys_in_page++;
            i++;
        }

        lhdr->numKeys = (short)keys_in_page;
        lhdr->keyPtr = key_ptr;
        lhdr->recIdPtr = rec_ptr;

        /* Link previous leaf page to this leaf page */
        if (num_leaves > 0) {
            char *prev_buf;
            if (PF_GetThisPage(fd, leaf_pages[num_leaves - 1], &prev_buf) == PFE_OK) {
                ((AM_LEAFHEADER *)prev_buf)->nextLeafPage = pagenum;
                PF_MarkDirty(fd, leaf_pages[num_leaves - 1]);
                PF_UnfixPage(fd, leaf_pages[num_leaves - 1], TRUE);
            }
        }

        PF_MarkDirty(fd, pagenum);
        PF_UnfixPage(fd, pagenum, TRUE);
        num_leaves++;
    }

    AM_LeftPageNum = (num_leaves > 0) ? leaf_pages[0] : AM_NULL_PAGE;

    /* 2. Build Internal Levels Bottom-Up until 1 Root remains */
    int curr_pages[2000], next_pages[2000];
    int curr_keys[2000], next_keys[2000];
    int curr_count = num_leaves;

    for (i = 0; i < num_leaves; i++) {
        curr_pages[i] = leaf_pages[i];
        curr_keys[i] = leaf_first_keys[i];
    }

    int max_int_keys = (PF_PAGE_SIZE - (int)sizeof(AM_INTHEADER) - (int)sizeof(int)) /
                       ((int)sizeof(int) + (int)sizeof(int));

    while (curr_count > 1) {
        int next_count = 0;
        int idx = 0;

        while (idx < curr_count) {
            AM_INTHEADER *ihdr;
            int offset = (int)sizeof(AM_INTHEADER);
            int k_count = 0;

            if ((err = PF_AllocPage(fd, &pagenum, &pagebuf)) != PFE_OK) return err;
            memset(pagebuf, 0, PF_PAGE_SIZE);

            ihdr = (AM_INTHEADER *)pagebuf;
            ihdr->pageType = 'i';
            ihdr->attrLength = sizeof(int);
            ihdr->maxKeys = (short)max_int_keys;

            next_pages[next_count] = pagenum;
            next_keys[next_count] = curr_keys[idx];

            /* First child pointer */
            memcpy(pagebuf + offset, &curr_pages[idx], sizeof(int));
            offset += (int)sizeof(int);
            idx++;

            /* Subsequent (separator_key, child_page) pairs */
            while (idx < curr_count && k_count < max_int_keys) {
                memcpy(pagebuf + offset, &curr_keys[idx], sizeof(int));
                offset += (int)sizeof(int);
                memcpy(pagebuf + offset, &curr_pages[idx], sizeof(int));
                offset += (int)sizeof(int);
                k_count++;
                idx++;
            }

            ihdr->numKeys = (short)k_count;
            PF_MarkDirty(fd, pagenum);
            PF_UnfixPage(fd, pagenum, TRUE);
            next_count++;
        }

        for (i = 0; i < next_count; i++) {
            curr_pages[i] = next_pages[i];
            curr_keys[i] = next_keys[i];
        }
        curr_count = next_count;
    }

    AM_RootPageNum = (curr_count == 1) ? curr_pages[0] : AM_NULL_PAGE;
    PF_CloseFile(fd);
    return AME_OK;
}

int main() {
    FILE *fp;
    char line[1024];
    int sp_fd, am_fd, pagenum, err, i;
    char *pagebuf;
    RID rid;
    clock_t start, end;
    double time_a, time_b, time_c;
    PF_Stats stats_a, stats_b, stats_c;
    static KeyRec entries[MAX_RECORDS];
    int total_records = 0;

    PF_SetBufferSize(20);
    PF_Init();

    /* ================================================================
       METHOD A: Existing File -> Build Index in a Single Operation
       ================================================================ */
    unlink("stud_a.db");
    unlink("stud_a.0");
    SP_ResetLastPage();
    PF_CreateFile("stud_a.db");
    sp_fd = PF_OpenFile("stud_a.db");

    fp = fopen(DATA_FILE, "r");
    if (!fp) { perror("Cannot open ../../data/student.txt"); return 1; }
    while (fgets(line, sizeof(line), fp) != NULL && total_records < MAX_RECORDS) {
        int len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        if (len == 0) continue;
        SP_InsertRecord(sp_fd, line, len, &rid);
        entries[total_records].key = atoi(line);
        entries[total_records].recId = encode_rid(rid);
        total_records++;
    }
    fclose(fp);
    PF_CloseFile(sp_fd);

    /* Now time ONLY the index construction on the existing file */
    PF_ResetStats();
    start = clock();

    AM_CreateIndex("stud_a", 0, 'i', sizeof(int));
    am_fd = PF_OpenFile("stud_a.0");
    sp_fd = PF_OpenFile("stud_a.db");

    err = PF_GetFirstPage(sp_fd, &pagenum, &pagebuf);
    while (err == PFE_OK) {
        PageHeader *hdr = (PageHeader *)pagebuf;
        SlotEntry *slots = (SlotEntry *)(pagebuf + sizeof(PageHeader));
        for (i = 0; i < hdr->num_slots; i++) {
            if (slots[i].offset != -1) {
                char temp[64];
                int clen = slots[i].length < 63 ? slots[i].length : 63;
                memcpy(temp, pagebuf + slots[i].offset, clen);
                temp[clen] = '\0';
                int roll = atoi(temp);
                RID r; r.page_num = pagenum; r.slot_num = i;
                AM_InsertEntry(am_fd, 'i', sizeof(int), (char *)&roll, encode_rid(r));
            }
        }
        PF_UnfixPage(sp_fd, pagenum, FALSE);
        err = PF_GetNextPage(sp_fd, &pagenum, &pagebuf);
    }

    PF_CloseFile(sp_fd);
    PF_CloseFile(am_fd);
    end = clock();
    time_a = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    stats_a = PF_GetStats();

    /* ================================================================
       METHOD B: Empty File -> Incremental Inserts
       ================================================================ */
    unlink("stud_b.db");
    unlink("stud_b.0");
    SP_ResetLastPage();
    PF_ResetStats();
    start = clock();

    PF_CreateFile("stud_b.db");
    sp_fd = PF_OpenFile("stud_b.db");
    AM_CreateIndex("stud_b", 0, 'i', sizeof(int));
    am_fd = PF_OpenFile("stud_b.0");

    fp = fopen(DATA_FILE, "r");
    int count_b = 0;
    while (fgets(line, sizeof(line), fp) != NULL && count_b < MAX_RECORDS) {
        int len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        if (len == 0) continue;
        SP_InsertRecord(sp_fd, line, len, &rid);
        int roll = atoi(line);
        AM_InsertEntry(am_fd, 'i', sizeof(int), (char *)&roll, encode_rid(rid));
        count_b++;
    }
    fclose(fp);

    PF_CloseFile(sp_fd);
    PF_CloseFile(am_fd);
    end = clock();
    time_b = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    stats_b = PF_GetStats();

    /* ================================================================
       METHOD C: Pre-Sorted File -> Bottom-Up Bulk-Loading
       ================================================================ */
    qsort(entries, total_records, sizeof(KeyRec), cmp_keyrec);

    PF_ResetStats();
    start = clock();

    AM_BulkLoad("stud_c.0", entries, total_records);

    end = clock();
    time_c = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    stats_c = PF_GetStats();

    /* ================================================================
       PRINT COMPARISON TABLE
       ================================================================ */
    printf("\nIndexed %d student records using roll-no:\n", total_records);
    printf("================================================================================\n");
    printf("| Approach                  | Time (ms) | Logical I/O | Phy Reads | Phy Writes |\n");
    printf("================================================================================\n");
    printf("| A: Existing File (Batch)  | %9.2f | %11ld | %9ld | %10ld |\n",
           time_a, stats_a.logical_reads + stats_a.logical_writes,
           stats_a.physical_reads, stats_a.physical_writes);
    printf("| B: Incremental Inserts    | %9.2f | %11ld | %9ld | %10ld |\n",
           time_b, stats_b.logical_reads + stats_b.logical_writes,
           stats_b.physical_reads, stats_b.physical_writes);
    printf("| C: Bulk-Loading (Sorted)  | %9.2f | %11ld | %9ld | %10ld |\n",
           time_c, stats_c.logical_reads + stats_c.logical_writes,
           stats_c.physical_reads, stats_c.physical_writes);
    printf("================================================================================\n");

    return 0;
}
