/* test_slotted.c: Load student.txt into slotted pages and compare space utilization */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "slotted_page.h"

#define SP_FILE "student_sp.db"
#define DATA_FILE "../../data/student.txt"

int main()
{
    FILE *fp;
    char line[1024];
    int fd, err, len, i;
    long total_records = 0;
    long total_bytes = 0;
    int max_rec_len = 0;
    int total_pages = 0;
    RID rid, first_rid;
    int static_lens[4] = {128, 256, 512, 1024};

    unlink(SP_FILE);
    SP_ResetLastPage();
    PF_Init();

    if ((err = PF_CreateFile(SP_FILE)) != PFE_OK) {
        PF_PrintError("CreateFile");
        return 1;
    }
    if ((fd = PF_OpenFile(SP_FILE)) < 0) {
        PF_PrintError("OpenFile");
        return 1;
    }

    fp = fopen(DATA_FILE, "r");
    if (!fp) {
        perror("Cannot open ../../data/student.txt");
        return 1;
    }

    /* 1. Insert all records from student.txt */
    while (fgets(line, sizeof(line), fp) != NULL) {
        len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        if ((err = SP_InsertRecord(fd, line, len, &rid)) != PFE_OK) {
            PF_PrintError("SP_InsertRecord");
            break;
        }
        if (total_records == 0) first_rid = rid;
        if (rid.page_num + 1 > total_pages) total_pages = rid.page_num + 1;
        if (len > max_rec_len) max_rec_len = len;
        total_bytes += len;
        total_records++;
    }
    fclose(fp);

    printf("Inserted %ld student records (Total Data: %ld bytes, Max Rec Len: %d bytes)\n",
           total_records, total_bytes, max_rec_len);

    /* 2. Test Sequential Scan (print first 3 records) */
    printf("\nSample Sequential Scan (first 3 records):\n");
    int scanned = SP_Scan(fd, 3);
    printf("Total records found in scan: %d\n", scanned);

    /* 3. Test Record Deletion on the first record */
    SP_DeleteRecord(fd, first_rid);
    int after_del = SP_Scan(fd, 0);
    printf("Records remaining after deleting 1 record: %d\n", after_del);

    PF_CloseFile(fd);

    /* 4. Space Utilization Comparison Table */
    double sp_util = ((double)total_bytes / ((double)total_pages * PF_PAGE_SIZE)) * 100.0;

    printf("\n=====================================================================\n");
    printf("| Storage Method   | Max Rec Len | Pages Used | Space Utilization   |\n");
    printf("=====================================================================\n");
    printf("| Slotted Page     | Variable    | %-10d | %6.2f%%             |\n",
           total_pages, sp_util);

    for (i = 0; i < 4; i++) {
        int rec_len = static_lens[i];
        int recs_per_page = PF_PAGE_SIZE / rec_len;
        int static_pages = (int)((total_records + recs_per_page - 1) / recs_per_page);
        double static_util = ((double)total_bytes / ((double)static_pages * PF_PAGE_SIZE)) * 100.0;
        printf("| Static Fixed     | %-11d | %-10d | %6.2f%%             |\n",
               rec_len, static_pages, static_util);
    }
    printf("=====================================================================\n");

    return 0;
}
