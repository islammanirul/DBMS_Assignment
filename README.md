# DBMS Assignment — ToyDB Extensions
**Course:** Database Management Systems  
**Instructor:** Nitin Awathare 

**Name:** Manirul Islam
**Roll No.** B24BS2192
**Course:** B.S/B.Sc. Apllied AI and Data Science
**College:** Indian Institute Of Technology,Jodhpur

## Overview
This project extends the **toyDB** storage and indexing engine across three core components:
1. **PF Layer Page Buffering (`toydb/pflayer/`)**: Configurable buffer pool size, selectable **LRU** and **MRU** replacement policies per open file, explicit dirty-page marking (`PF_MarkDirty`), and logical/physical I/O statistics tracking.
2. **Slotted-Page Storage (`toydb/pflayer/slotted_page.h`, `slotted_page.c`)**: Variable-length record management on top of 4096-byte PF pages supporting record insertion, tombstone deletion, and sequential scanning.
3. **AM Layer $B^+$ Tree Indexing & Bulk-Loading (`toydb/amlayer/test_index.c`)**: Index construction on `student.txt` (`roll-no` key) comparing batch insertion on an existing file, incremental insertion from an empty file, and bottom-up bulk-loading on pre-sorted data.

---

## Directory Structure
* `toydb/pflayer/pf.h`, `pftypes.h`, `pf.c`, `buf.c` — Modified PF layer supporting LRU/MRU strategies, configurable buffer size, `PF_MarkDirty`, and `PF_Stats`.
* `toydb/pflayer/test_buf.c` & `plot_pf.py` — Benchmark driver and plotting script for Read/Write query mixtures (`pf_buffer_stats.png`).
* `toydb/pflayer/slotted_page.h`, `slotted_page.c`, `test_slotted.c` — Slotted-page implementation and space utilization benchmark.
* `toydb/amlayer/test_index.c` — Benchmark comparing Batch, Incremental, and Bottom-Up Bulk-Loading $B^+$ tree index construction.
* `pf_buffer_stats.png` & `pf_stats.csv` — Generated I/O statistics and plot for Objective 1.

---

## Compilation & Execution Instructions

### 1. Objective 1: PF Layer Page Buffering & Graph
```bash
cd toydb/pflayer
rm -f *.o testpf test_buf
make testpf CFLAGS="-std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype"
cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -o test_buf test_buf.c pflayer.o
./test_buf
python plot_pf.py
```
* **Output**: Prints logical/physical read and write counts across Read/Write mixtures (`100%R/0%W` to `0%R/100%W`) to `pf_stats.csv` and generates `pf_buffer_stats.png`.

### 2. Objective 2: Slotted-Page Structure
```bash
cd toydb/pflayer
cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -o test_slotted test_slotted.c slotted_page.c pflayer.o
./test_slotted
```
* **Output**: Inserts all 17,814 variable-length records from `data/student.txt`, verifies sequential scanning and record deletion, and prints the space utilization comparison table.

### 3. Objective 3: $B^+$ Tree Index Construction & Bulk-Loading
```bash
cd toydb/amlayer
make
cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -o test_index test_index.c ../pflayer/slotted_page.c am*.o ../pflayer/pflayer.o
./test_index
```
* **Output**: Constructs $B^+$ tree indexes on `roll-no` using all three approaches and prints completion time and page I/O statistics.

---

## Experimental Results

### Objective 1: PF Layer Buffer Statistics
See `pf_buffer_stats.png` (and raw data in `pf_stats.csv`) for the plot of Read/Write query mixtures (X-axis) vs. Logical and Physical I/O counts (Y-axis).

### Objective 2: Slotted-Page vs. Static Record Management (17,814 Student Records)
| Storage Method | Max Record Length | Pages Used | Space Utilization |
| :--- | :--- | :---: | :---: |
| **Slotted Page** | Variable | **472** | **90.99%** |
| Static Fixed | 128 bytes | 557 | 77.10% |
| Static Fixed | 256 bytes | 1,114 | 38.55% |
| Static Fixed | 512 bytes | 2,227 | 19.28% |
| Static Fixed | 1024 bytes | 4,454 | 9.64% |

### Objective 3: $B^+$ Tree Index Construction on `roll-no` (17,814 Records)
| Approach | Completion Time (ms) | Logical I/O | Physical Reads | Physical Writes |
| :--- | :---: | :---: | :---: | :---: |
| **A: Existing File (Single Batch Op)** | 17.29 | 67,692 | 533 | 490 |
| **B: Incremental Inserts (Empty File)** | 16.38 | 120,661 | 61 | 962 |
| **C: Bottom-Up Bulk-Loading (Pre-Sorted)** | **0.70** | **294** | **0** | **60** |
