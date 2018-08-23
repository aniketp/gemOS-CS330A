#include<context.h>
#include<memory.h>
#include<lib.h>

#define osmap(a)        (unsigned long *) osmap(a)

#define PToffset       0x1FF             /* (1 << 9) - 1 : PT offset size */
#define L4offset       0x027             /* L4 Offset : 39-48 bits */
#define L3offset       0x01E             /* L3 Offset : 30-39 bits */
#define L2offset       0x015             /* L2 Offset : 21-30 bits */
#define L1offset       0x00C             /* L1 Offset : 12-21 bits */
#define PTEoffset      0x00C             /* PFN is at an offset of 12 in PTE */

#define pr             0x001             /* Present Bit */
#define rw             0x002             /* Write Bit: Not for Code Segment */
#define us             0x004             /* User Mode */


void prepare_context_mm(struct exec_context *ctx)
{
        /*  PFN of the first level translation */
        ctx->pgd = os_pfn_alloc(OS_PT_REG);
        unsigned long *l4addr = osmap(ctx->pgd);

        unsigned long *l3addr, *l2addr, *l1addr;
        u32 offsetL4, offsetL3, offsetL2, offsetL1;
        u32 pfnL1, pfnL2, pfnL3, dataPFN;

        /**************** Stack Segment *******************/

        unsigned long stackVA = ctx->mms[MM_SEG_STACK].end - 1;
        /* Extract out all 4 level page offsets */
        offsetL4 = PToffset & (stackVA >> L4offset);
        offsetL3 = PToffset & (stackVA >> L3offset);
        offsetL2 = PToffset & (stackVA >> L2offset);
        offsetL1 = PToffset & (stackVA >> L1offset);

        /* Fill up Page Table Pages entries */
        if (*(l4addr + offsetL4) & 1)        // If PT already present
                pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
        else {
                pfnL3 = os_pfn_alloc(OS_PT_REG);
                *(l4addr + offsetL4) = (pfnL3 << PTEoffset) | (pr | rw | us);
        }

        l3addr = osmap(pfnL3);
        if (*(l3addr + offsetL3) & 1)        // If PT already present
                pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
        else {
                pfnL2 = os_pfn_alloc(OS_PT_REG);
                *(l3addr + offsetL3) = (pfnL2 << PTEoffset) | (pr | rw | us);
        }

        l2addr = osmap(pfnL2);
        if (*(l2addr + offsetL2) & 1)        // If PT already present
                pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
        else {
                pfnL1 = os_pfn_alloc(OS_PT_REG);
                *(l2addr + offsetL2) = (pfnL1 << PTEoffset) | (pr | rw | us);
        }

        /* Final mapping to Data Physical Page */
        l1addr = osmap(pfnL1);
        if (*(l1addr + offsetL1) & 1)        // If PT already present
                dataPFN = *(l1addr + offsetL1) >> PTEoffset;
        else {
                dataPFN = os_pfn_alloc(USER_REG);
                *(l1addr + offsetL1) = (dataPFN << PTEoffset) | (pr | rw | us);
        }

        /**************** Code Segment *******************/

        unsigned long codeVA = ctx->mms[MM_SEG_CODE].start;
        /* Extract out all 4 level page offsets */
        offsetL4 = PToffset & (codeVA >> L4offset);
        offsetL3 = PToffset & (codeVA >> L3offset);
        offsetL2 = PToffset & (codeVA >> L2offset);
        offsetL1 = PToffset & (codeVA >> L1offset);

        /* Fill up Page Table entries */
        /* l4addr will be same for all segments */
        if (*(l4addr + offsetL4) & 1)        // If PT already present
                pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
        else {
                pfnL3 = os_pfn_alloc(OS_PT_REG);
                *(l4addr + offsetL4) = (pfnL3 << PTEoffset) | (pr | us);
        }

        l3addr = osmap(pfnL3);
        if (*(l3addr + offsetL3) & 1)        // If PT already present
                pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
        else {
                pfnL2 = os_pfn_alloc(OS_PT_REG);
                *(l3addr + offsetL3) = (pfnL2 << PTEoffset) | (pr | us);
        }

        l2addr = osmap(pfnL2);
        if (*(l2addr + offsetL2) & 1)        // If PT already present
                pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
        else {
                pfnL1 = os_pfn_alloc(OS_PT_REG);
                *(l2addr + offsetL2) = (pfnL1 << PTEoffset) | (pr | us);
        }

        /* Final mapping to Data Physical Page */
        l1addr = osmap(pfnL1);
        if (*(l1addr + offsetL1) & 1)        // If PT already present
                dataPFN = *(l1addr + offsetL1) >> PTEoffset;
        else {
                dataPFN = os_pfn_alloc(USER_REG);
                *(l1addr + offsetL1) = (dataPFN << PTEoffset) | (pr | us);
        }


        /**************** Data Segment *******************/

        unsigned long dataVA = ctx->mms[MM_SEG_DATA].start;
        /* Extract out all 4 level page offsets */
        offsetL4 = PToffset & (dataVA >> L4offset);
        offsetL3 = PToffset & (dataVA >> L3offset);
        offsetL2 = PToffset & (dataVA >> L2offset);
        offsetL1 = PToffset & (dataVA >> L1offset);

        /* Fill up Page Table entries */
        /* l4addr will be same for all segments */
        if (*(l4addr + offsetL4) & 1)        // If PT already present
                pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
        else {
                pfnL3 = os_pfn_alloc(OS_PT_REG);
                *(l4addr + offsetL4) = (pfnL3 << PTEoffset) | (pr | rw | us);
        }

        l3addr = osmap(pfnL3);
        if (*(l3addr + offsetL3) & 1)        // If PT already present
                pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
        else {
                pfnL2 = os_pfn_alloc(OS_PT_REG);
                *(l3addr + offsetL3) = (pfnL2 << PTEoffset) | (pr | rw | us);
        }

        l2addr = osmap(pfnL2);
        if (*(l2addr + offsetL2) & 1)        // If PT already present
                pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
        else {
                pfnL1 = os_pfn_alloc(OS_PT_REG);
                *(l2addr + offsetL2) = (pfnL1 << PTEoffset) | (pr | rw | us);
        }

        /* NOTE: We don't need to allocate Physical Page for final data */
        /* of Data Segment as it is already provided to us : arg_pfn */
        l1addr = osmap(pfnL1);
        if (*(l1addr + offsetL1) & 1)        // If PT already present
                /* Should not happen, but for the sake of uniformity */
                ctx->arg_pfn = *(l1addr + offsetL1) >> PTEoffset;
        else
                *(l1addr + offsetL1) = (ctx->arg_pfn << PTEoffset) |
                (pr | rw | us);

        return;
}

void cleanup_context_mm(struct exec_context *ctx)
{
        /*  PFN of the first level translation */
        unsigned long *l4addr = osmap(ctx->pgd);
        unsigned long *l3addr, *l2addr, *l1addr;
        u32 offsetL4, offsetL3, offsetL2, offsetL1;
        u32 pfnL1, pfnL2, pfnL3, dataPFN;

        /**************** Stack Segment *******************/

        unsigned long stackVA = ctx->mms[MM_SEG_STACK].end - 1;
        /* Extract out all 4 level page offsets */
        offsetL4 = PToffset & (stackVA >> L4offset);
        offsetL3 = PToffset & (stackVA >> L3offset);
        offsetL2 = PToffset & (stackVA >> L2offset);
        offsetL1 = PToffset & (stackVA >> L1offset);

        /* Cleanup Page Table entries */
        if (*(l4addr + offsetL4) & 1) {
                pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
                l3addr = osmap(pfnL3);
        }

        if (*(l3addr + offsetL3) & 1) {
                pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
                l2addr = osmap(pfnL2);
                os_pfn_free(OS_PT_REG, pfnL3);
        }

        if (*(l2addr + offsetL2) & 1) {
                pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
                l1addr = osmap(pfnL1);
                os_pfn_free(OS_PT_REG, pfnL2);
        }

        if (*(l1addr + offsetL1) & 1) {
                dataPFN = *(l1addr + offsetL1) >> PTEoffset;
                os_pfn_free(OS_PT_REG, pfnL1);
        }

        os_pfn_free(USER_REG, dataPFN);

        /**************** Code Segment *******************/

        unsigned long codeVA = ctx->mms[MM_SEG_CODE].start;
        /* Extract out all 4 level page offsets */
        offsetL4 = PToffset & (codeVA >> L4offset);
        offsetL3 = PToffset & (codeVA >> L3offset);
        offsetL2 = PToffset & (codeVA >> L2offset);
        offsetL1 = PToffset & (codeVA >> L1offset);

        /* Cleanup Page Table entries */
        if (*(l4addr + offsetL4) & 1) {
                pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
                l3addr = osmap(pfnL3);
        }

        if (*(l3addr + offsetL3) & 1) {
                pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
                l2addr = osmap(pfnL2);
        }

        if (*(l2addr + offsetL2) & 1) {
                pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
                l1addr = osmap(pfnL1);
                os_pfn_free(OS_PT_REG, pfnL2);
        }

        if (*(l1addr + offsetL1) & 1) {
                dataPFN = *(l1addr + offsetL1) >> PTEoffset;
                os_pfn_free(OS_PT_REG, pfnL1);
        }

        os_pfn_free(USER_REG, dataPFN);

        /**************** Data Segment *******************/

        unsigned long dataVA = ctx->mms[MM_SEG_DATA].start;
        /* Extract out all 4 level page offsets */
        offsetL4 = PToffset & (dataVA >> L4offset);
        offsetL3 = PToffset & (dataVA >> L3offset);
        offsetL2 = PToffset & (dataVA >> L2offset);
        offsetL1 = PToffset & (dataVA >> L1offset);

        /* Cleanup Page Table entries */
        if (*(l4addr + offsetL4) & 1) {
                pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
                l3addr = osmap(pfnL3);
        }

        if (*(l3addr + offsetL3) & 1) {
                pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
                l2addr = osmap(pfnL2);
                os_pfn_free(OS_PT_REG, pfnL3);
        }

        if (*(l2addr + offsetL2) & 1) {
                pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
                l1addr = osmap(pfnL1);
                os_pfn_free(OS_PT_REG, pfnL2);
        }

        if (*(l1addr + offsetL1) & 1) {
                dataPFN = *(l1addr + offsetL1) >> PTEoffset;
                os_pfn_free(OS_PT_REG, pfnL1);
        }

        os_pfn_free(USER_REG, dataPFN);

        return;
}
