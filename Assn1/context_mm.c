#include<context.h>
#include<memory.h>
#include<lib.h>

#define osmap(a) (unsigned long *)osmap(a)

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
        offsetL4 = ((1 << 9) - 1) & (stackVA >> 39);
        offsetL3 = ((1 << 9) - 1) & (stackVA >> 30);
        offsetL2 = ((1 << 9) - 1) & (stackVA >> 21);
        offsetL1 = ((1 << 9) - 1) & (stackVA >> 12);

        /* Fill up Page Table Pages entries */
        if (*(l4addr + offsetL4) & 1)        // If PT already present
                pfnL3 = *(l4addr + offsetL4) >> 12;
        else {
                pfnL3 = os_pfn_alloc(OS_PT_REG);
                *(l4addr + offsetL4) = (pfnL3 << 12) | 7;
        }

        l3addr = osmap(pfnL3);
        if (*(l3addr + offsetL3) & 1)        // If PT already present
                pfnL2 = *(l3addr + offsetL3) >> 12;
        else {
                pfnL2 = os_pfn_alloc(OS_PT_REG);
                *(l3addr + offsetL3) = (pfnL2 << 12) | 7;
        }

        l2addr = osmap(pfnL2);
        if (*(l2addr + offsetL2) & 1)        // If PT already present
                pfnL1 = *(l2addr + offsetL2) >> 12;
        else {
                pfnL1 = os_pfn_alloc(OS_PT_REG);
                *(l2addr + offsetL2) = (pfnL1 << 12) | 7;
        }

        /* Final mapping to Data Physical Page */
        l1addr = osmap(pfnL1);
        if (*(l1addr + offsetL1) & 1)        // If PT already present
                dataPFN = *(l1addr + offsetL1) >> 12;
        else {
                dataPFN = os_pfn_alloc(USER_REG);
                *(l1addr + offsetL1) = (dataPFN << 12) | 7;
        }

        /**************** Code Segment *******************/

        unsigned long codeVA = ctx->mms[MM_SEG_CODE].start;
        /* Extract out all 4 level page offsets */
        offsetL4 = ((1 << 9) - 1) & (codeVA >> 39);
        offsetL3 = ((1 << 9) - 1) & (codeVA >> 30);
        offsetL2 = ((1 << 9) - 1) & (codeVA >> 21);
        offsetL1 = ((1 << 9) - 1) & (codeVA >> 12);

        /* Fill up Page Table entries */
        /* l4addr will be same for all segments */
        if (*(l4addr + offsetL4) & 1)        // If PT already present
                pfnL3 = *(l4addr + offsetL4) >> 12;
        else {
                pfnL3 = os_pfn_alloc(OS_PT_REG);
                *(l4addr + offsetL4) = (pfnL3 << 12) | 5;
        }

        l3addr = osmap(pfnL3);
        if (*(l3addr + offsetL3) & 1)        // If PT already present
                pfnL2 = *(l3addr + offsetL3) >> 12;
        else {
                pfnL2 = os_pfn_alloc(OS_PT_REG);
                *(l3addr + offsetL3) = (pfnL2 << 12) | 5;
        }

        l2addr = osmap(pfnL2);
        if (*(l2addr + offsetL2) & 1)        // If PT already present
                pfnL1 = *(l2addr + offsetL2) >> 12;
        else {
                pfnL1 = os_pfn_alloc(OS_PT_REG);
                *(l2addr + offsetL2) = (pfnL1 << 12) | 5;
        }

        /* Final mapping to Data Physical Page */
        l1addr = osmap(pfnL1);
        if (*(l1addr + offsetL1) & 1)        // If PT already present
                dataPFN = *(l1addr + offsetL1) >> 12;
        else {
                dataPFN = os_pfn_alloc(USER_REG);
                *(l1addr + offsetL1) = (dataPFN << 12) | 5;
        }


        /**************** Data Segment *******************/

        unsigned long dataVA = ctx->mms[MM_SEG_DATA].start;
        /* Extract out all 4 level page offsets */
        offsetL4 = ((1 << 9) - 1) & (dataVA >> 39);
        offsetL3 = ((1 << 9) - 1) & (dataVA >> 30);
        offsetL2 = ((1 << 9) - 1) & (dataVA >> 21);
        offsetL1 = ((1 << 9) - 1) & (dataVA >> 12);

        /* Fill up Page Table entries */
        /* l4addr will be same for all segments */
        if (*(l4addr + offsetL4) & 1)        // If PT already present
                pfnL3 = *(l4addr + offsetL4) >> 12;
        else {
                pfnL3 = os_pfn_alloc(OS_PT_REG);
                *(l4addr + offsetL4) = (pfnL3 << 12) | 7;
        }

        l3addr = osmap(pfnL3);
        if (*(l3addr + offsetL3) & 1)        // If PT already present
                pfnL2 = *(l3addr + offsetL3) >> 12;
        else {
                pfnL2 = os_pfn_alloc(OS_PT_REG);
                *(l3addr + offsetL3) = (pfnL2 << 12) | 7;
        }

        l2addr = osmap(pfnL2);
        if (*(l2addr + offsetL2) & 1)        // If PT already present
                pfnL1 = *(l2addr + offsetL2) >> 12;
        else {
                pfnL1 = os_pfn_alloc(OS_PT_REG);
                *(l2addr + offsetL2) = (pfnL1 << 12) | 7;
        }

        /* NOTE: We don't need to allocate Physical Page for final data */
        /* of Data Segment as it is already provided to us : arg_pfn */
        l1addr = osmap(pfnL1);
        if (*(l1addr + offsetL1) & 1)        // If PT already present
                /* Should not happen, but for the sake of uniformity */
                ctx->arg_pfn = *(l1addr + offsetL1) >> 12;
        else
                *(l1addr + offsetL1) = (ctx->arg_pfn << 12) | 7;

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
        offsetL4 = ((1 << 9) - 1) & (stackVA >> 39);
        offsetL3 = ((1 << 9) - 1) & (stackVA >> 30);
        offsetL2 = ((1 << 9) - 1) & (stackVA >> 21);
        offsetL1 = ((1 << 9) - 1) & (stackVA >> 12);

        /* Cleanup Page Table entries */
        if (*(l4addr + offsetL4) & 1) {
                pfnL3 = *(l4addr + offsetL4) >> 12;
                l3addr = osmap(pfnL3);
        }

        if (*(l3addr + offsetL3) & 1) {
                pfnL2 = *(l3addr + offsetL3) >> 12;
                l2addr = osmap(pfnL2);
                os_pfn_free(OS_PT_REG, pfnL3);
        }

        if (*(l2addr + offsetL2) & 1) {
                pfnL1 = *(l2addr + offsetL2) >> 12;
                l1addr = osmap(pfnL1);
                os_pfn_free(OS_PT_REG, pfnL2);
        }

        if (*(l1addr + offsetL1) & 1) {
                dataPFN = *(l1addr + offsetL1) >> 12;
                os_pfn_free(OS_PT_REG, pfnL1);
        }

        os_pfn_free(USER_REG, dataPFN);

        /**************** Code Segment *******************/

        unsigned long codeVA = ctx->mms[MM_SEG_CODE].start;
        /* Extract out all 4 level page offsets */
        offsetL4 = ((1 << 9) - 1) & (codeVA >> 39);
        offsetL3 = ((1 << 9) - 1) & (codeVA >> 30);
        offsetL2 = ((1 << 9) - 1) & (codeVA >> 21);
        offsetL1 = ((1 << 9) - 1) & (codeVA >> 12);

        /* Cleanup Page Table entries */
        if (*(l4addr + offsetL4) & 1) {
                pfnL3 = *(l4addr + offsetL4) >> 12;
                l3addr = osmap(pfnL3);
        }

        if (*(l3addr + offsetL3) & 1) {
                pfnL2 = *(l3addr + offsetL3) >> 12;
                l2addr = osmap(pfnL2);
        }

        if (*(l2addr + offsetL2) & 1) {
                pfnL1 = *(l2addr + offsetL2) >> 12;
                l1addr = osmap(pfnL1);
                os_pfn_free(OS_PT_REG, pfnL2);
        }

        if (*(l1addr + offsetL1) & 1) {
                dataPFN = *(l1addr + offsetL1) >> 12;
                os_pfn_free(OS_PT_REG, pfnL1);
        }

        os_pfn_free(USER_REG, dataPFN);

        /**************** Data Segment *******************/

        unsigned long dataVA = ctx->mms[MM_SEG_DATA].start;
        /* Extract out all 4 level page offsets */
        offsetL4 = ((1 << 9) - 1) & (dataVA >> 39);
        offsetL3 = ((1 << 9) - 1) & (dataVA >> 30);
        offsetL2 = ((1 << 9) - 1) & (dataVA >> 21);
        offsetL1 = ((1 << 9) - 1) & (dataVA >> 12);

        /* Cleanup Page Table entries */
        if (*(l4addr + offsetL4) & 1) {
                pfnL3 = *(l4addr + offsetL4) >> 12;
                l3addr = osmap(pfnL3);
        }

        if (*(l3addr + offsetL3) & 1) {
                pfnL2 = *(l3addr + offsetL3) >> 12;
                l2addr = osmap(pfnL2);
                os_pfn_free(OS_PT_REG, pfnL3);
        }

        if (*(l2addr + offsetL2) & 1) {
                pfnL1 = *(l2addr + offsetL2) >> 12;
                l1addr = osmap(pfnL1);
                os_pfn_free(OS_PT_REG, pfnL2);
        }

        if (*(l1addr + offsetL1) & 1) {
                dataPFN = *(l1addr + offsetL1) >> 12;
                os_pfn_free(OS_PT_REG, pfnL1);
        }

        os_pfn_free(USER_REG, dataPFN);

        return;
}
