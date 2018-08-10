### Virtual Memory and Page Tables


<!-- * **Using the page table design for 32-bit virtual addess discussed in class, answer the following questions.** -->

#### 1) Calculate the worst case memory consumption for maintaining page table for 32 applications.

For the worst case memory consumption, we assume that page-table per process maps to the entire physical address space.

Given the machine has 32 bit architecture, possible address range is `0x00000000` to `0xffffffff`. This allows a total of `2^32` bytes or `4GB`.

A page size is `4KB`, hence the number of entries in page-table is `2^32/2^12 = 4294967296/4096` = 1048576 (~ 1 Million).

Each Page Table Entry (PTE) is 4 Bytes (32 bit), hence the entire Page Table in this case can take up to `4*1048576` Bytes, or `4MB`

So with 32 processes, we have a worst case memory consumption of `128 MB`.

#### 2) Answer (i) assuming page size (and the page frame size) to be 64KB.

For 64KB Page Size, total number of Page Table Entries are `2^32/2^16 = 65536`.

Therefore, 32 processes can consume maximum of `32 * 4 * 65536 = 8388608` Bytes, or `8 MB`.

Hence, we see a significant improvement in the space requirement for Page Tables if the Page Size is high.
