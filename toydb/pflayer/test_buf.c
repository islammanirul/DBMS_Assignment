/* test_buf.c: Benchmark PF Layer with LRU and MRU across Read/Write mixtures */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pf.h"

#define TEST_FILE "buf_bench_file"
#define NUM_PAGES 25
#define BUF_SIZE  10
#define NUM_PASSES 10

void run_benchmark(strategy, strat_name, write_pct, csv)
int strategy;
char *strat_name;
int write_pct;
FILE *csv;
{
    int fd, i, pass, pagenum, err;
    char *buf;
    PF_Stats s;
    int access_count = 0;

    /* Remove old test file if it exists */
    unlink(TEST_FILE);

    PF_SetBufferSize(BUF_SIZE);
    PF_Init();

    if ((err = PF_CreateFile(TEST_FILE)) != PFE_OK) {
        PF_PrintError("CreateFile");
        exit(1);
    }

    if ((fd = PF_OpenFileWithStrategy(TEST_FILE, strategy)) < 0) {
        PF_PrintError("OpenFile");
        exit(1);
    }

    /* Pre-populate file with NUM_PAGES pages */
    for (i = 0; i < NUM_PAGES; i++) {
        if ((err = PF_AllocPage(fd, &pagenum, &buf)) != PFE_OK) {
            PF_PrintError("AllocPage");
            exit(1);
        }
        buf[0] = (char)i;
        PF_UnfixPage(fd, pagenum, TRUE);
    }

    /* Close and reopen to start with a clean buffer and reset stats */
    PF_CloseFile(fd);
    PF_ResetStats();

    if ((fd = PF_OpenFileWithStrategy(TEST_FILE, strategy)) < 0) {
        PF_PrintError("ReopenFile");
        exit(1);
    }

    /* Repeated sequential scans (highlights LRU vs MRU difference) */
    for (pass = 0; pass < NUM_PASSES; pass++) {
        for (i = 0; i < NUM_PAGES; i++) {
            if ((err = PF_GetThisPage(fd, i, &buf)) != PFE_OK) {
                PF_PrintError("GetThisPage");
                exit(1);
            }
            /* Decide whether this query is a write based on write_pct */
            if ((access_count % 100) < write_pct) {
                buf[0] = (char)(buf[0] + 1);
                PF_MarkDirty(fd, i);
            }
            PF_UnfixPage(fd, i, FALSE);
            access_count++;
        }
    }

    PF_CloseFile(fd);
    s = PF_GetStats();

    printf("%-4s | %3d%%R/%3d%%W | LogR: %4ld | LogW: %4ld | PhyR: %4ld | PhyW: %4ld\n",
           strat_name, 100 - write_pct, write_pct,
           s.logical_reads, s.logical_writes, s.physical_reads, s.physical_writes);

    fprintf(csv, "%s,%dR/%dW,%ld,%ld,%ld,%ld,%ld,%ld\n",
            strat_name, 100 - write_pct, write_pct,
            s.logical_reads, s.logical_writes,
            s.logical_reads + s.logical_writes,
            s.physical_reads, s.physical_writes,
            s.physical_reads + s.physical_writes);

    unlink(TEST_FILE);
}

int main()
{
    int mixtures[5] = {0, 20, 50, 80, 100}; /* Write percentages */
    int i;
    FILE *csv = fopen("pf_stats.csv", "w");
    if (!csv) {
        perror("fopen");
        return 1;
    }

    fprintf(csv, "Strategy,Mixture,Logical_Reads,Logical_Writes,Total_Logical_IO,Physical_Reads,Physical_Writes,Total_Physical_IO\n");

    printf("Strat| Mixture    | LogR       | LogW       | PhyR       | PhyW\n");
    printf("----------------------------------------------------------------------\n");

    for (i = 0; i < 5; i++) {
        run_benchmark(PF_STRATEGY_LRU, "LRU", mixtures[i], csv);
        run_benchmark(PF_STRATEGY_MRU, "MRU", mixtures[i], csv);
    }

    fclose(csv);
    printf("\nSaved results to pf_stats.csv!\n");
    return 0;
}