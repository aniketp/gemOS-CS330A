#include<context.h>
#include<memory.h>
#include<lib.h>

void prepare_context_mm(struct exec_context *ctx)
{
        /*  PFN of the first level translation */
        ctx->pgd = os_pfn_alloc(OS_PT_REG);
        unsigned long *l4addr = osmap(ctx->pgd);
        unsigned long *l3addr, *l2addr, *l1addr;
        unsigned long *l3addr_, *l2addr_, *l1addr_;
        unsigned long *l3addr_d, *l2addr_d, *l1addr_d;

        u32 pfnL1, pfnL2, pfnL3, dataPFN;
        u32 pfnL1_, pfnL2_, pfnL3_, dataPFN_;
        u32 pfnL1_d, pfnL2_d, pfnL3_d;


        /**************** Stack Segment *******************/

        unsigned long stackVA = ctx->mms[MM_SEG_STACK].start;
        /* Extract out all 4 level page offsets */
        u32 offsetL4 = ((1 << 9) - 1) & (stackVA >> 39);
        u32 offsetL3 = ((1 << 9) - 1) & (stackVA >> 30);
        u32 offsetL2 = ((1 << 9) - 1) & (stackVA >> 21);
        u32 offsetL1 = ((1 << 9) - 1) & (stackVA >> 12);

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
        u32 offsetL4_ = ((1 << 9) - 1) & (codeVA >> 39);
        u32 offsetL3_ = ((1 << 9) - 1) & (codeVA >> 30);
        u32 offsetL2_ = ((1 << 9) - 1) & (codeVA >> 21);
        u32 offsetL1_ = ((1 << 9) - 1) & (codeVA >> 12);

        /* Fill up Page Table entries */
        /* l4addr will be same for all segments */
        if (*(l4addr + offsetL4_) & 1)        // If PT already present
                pfnL3_ = *(l4addr + offsetL4_) >> 12;
        else {
                pfnL3_ = os_pfn_alloc(OS_PT_REG);
                *(l4addr + offsetL4_) = (pfnL3_ << 12) | 7;
        }

        l3addr_ = osmap(pfnL3_);
        if (*(l3addr_ + offsetL3_) & 1)        // If PT already present
                pfnL2_ = *(l3addr_ + offsetL3_) >> 12;
        else {
                pfnL2_ = os_pfn_alloc(OS_PT_REG);
                *(l3addr_ + offsetL3_) = (pfnL2_ << 12) | 7;
        }

        l2addr_ = osmap(pfnL2_);
        if (*(l2addr_ + offsetL2_) & 1)        // If PT already present
                pfnL1_ = *(l2addr_ + offsetL2_) >> 12;
        else {
                pfnL1_ = os_pfn_alloc(OS_PT_REG);
                *(l2addr_ + offsetL2_) = (pfnL1_ << 12) | 7;
        }

        /* Final mapping to Data Physical Page */
        l1addr_ = osmap(pfnL1_);
        if (*(l1addr_ + offsetL1_) & 1)        // If PT already present
                dataPFN_ = *(l1addr_ + offsetL1_) >> 12;
        else {
                dataPFN_ = os_pfn_alloc(USER_REG);
                *(l1addr_ + offsetL1_) = (dataPFN_ << 12) | 7;
        }


        /**************** Data Segment *******************/

        unsigned long dataVA = ctx->mms[MM_SEG_DATA].start;
        /* Extract out all 4 level page offsets */
        u32 offsetL4_d = ((1 << 9) - 1) & (dataVA >> 39);
        u32 offsetL3_d = ((1 << 9) - 1) & (dataVA >> 30);
        u32 offsetL2_d = ((1 << 9) - 1) & (dataVA >> 21);
        u32 offsetL1_d = ((1 << 9) - 1) & (dataVA >> 12);

        /* Fill up Page Table entries */
        /* l4addr will be same for all segments */
        if (*(l4addr + offsetL4_d) & 1)        // If PT already present
                pfnL3_d = *(l4addr + offsetL4_d) >> 12;
        else {
                pfnL3_d = os_pfn_alloc(OS_PT_REG);
                *(l4addr + offsetL4_d) = (pfnL3_d << 12) | 7;
        }

        l3addr_d = osmap(pfnL3_d);
        if (*(l3addr_d + offsetL3_d) & 1)        // If PT already present
                pfnL2_d = *(l3addr_d + offsetL3_d) >> 12;
        else {
                pfnL2_d = os_pfn_alloc(OS_PT_REG);
                *(l3addr_d + offsetL3_d) = (pfnL2_d << 12) | 7;
        }

        l2addr_d = osmap(pfnL2_d);
        if (*(l2addr_d + offsetL2_d) & 1)        // If PT already present
                pfnL1_d = *(l2addr_d + offsetL2_d) >> 12;
        else {
                pfnL1_d = os_pfn_alloc(OS_PT_REG);
                *(l2addr_d + offsetL2_d) = (pfnL1_d << 12) | 7;
        }

        /* NOTE: We don't need to allocate Physical Page for final data */
        /* of Data Segment as it is already provided to us : arg_pfn */
        l1addr_d = osmap(pfnL1_d);
        if (*(l1addr_d + offsetL1_d) & 1)        // If PT already present
                /* Should not happen, but for the sake of uniformity */
                ctx->arg_pfn = *(l1addr_d + offsetL1_d) >> 12;
        else
                *(l1addr_d + offsetL1_d) = (ctx->arg_pfn << 12) | 7;


        /************************* DEBUG ************************/
        // Debug
        // u32 a = ctx->mms[MM_SEG_STACK].access_flags;
        // printf("%x\n", a);
        // unsigned long b = ctx->mms[MM_SEG_STACK].end;
        printf("%x %x %x\n", *(l4addr + offsetL4), *(l4addr + offsetL4_) & 1, (pfnL3 << 12) | 7);
        printf("%x %x %x\n", *(l2addr_ + offsetL2_), *(l4addr + offsetL4_) & 1, (pfnL1_ << 12) | 7);
        printf("%x %d %d\n", l2addr, offsetL2, pfnL2);
        printf("%x %d %d\n", l1addr, offsetL1, pfnL1);

        return;
}

void cleanup_context_mm(struct exec_context *ctx)
{
        /*  PFN of the first level translation */
        unsigned long *l4addr = osmap(ctx->pgd);
        unsigned long *l3addr, *l2addr, *l1addr;
        unsigned long *l3addr_, *l2addr_, *l1addr_;
        unsigned long *l3addr_d, *l2addr_d, *l1addr_d;

        u32 pfnL1, pfnL2, pfnL3, dataPFN;
        u32 pfnL1_, pfnL2_, pfnL3_, dataPFN_;
        u32 pfnL1_d, pfnL2_d, pfnL3_d;

        /**************** Stack Segment *******************/

        unsigned long stackVA = ctx->mms[MM_SEG_STACK].start;
        /* Extract out all 4 level page offsets */
        u32 offsetL4 = ((1 << 9) - 1) & (stackVA >> 39);
        u32 offsetL3 = ((1 << 9) - 1) & (stackVA >> 30);
        u32 offsetL2 = ((1 << 9) - 1) & (stackVA >> 21);
        u32 offsetL1 = ((1 << 9) - 1) & (stackVA >> 12);

        /* Cleanup Page Table entries */
        if (*(l4addr + offsetL4) & 1) {
                pfnL3 = *(l4addr + offsetL4) >> 12;
                l3addr = osmap(pfnL3);
                os_pfn_free(OS_PT_REG, pfnL3);
        }

        if (*(l3addr + offsetL3) & 1) {
                pfnL2 = *(l3addr + offsetL3) >> 12;
                l2addr = osmap(pfnL2);
                os_pfn_free(OS_PT_REG, pfnL2);
        }

        if (*(l2addr + offsetL2) & 1) {
                pfnL1 = *(l2addr + offsetL2) >> 12;
                l1addr = osmap(pfnL1);
                os_pfn_free(OS_PT_REG, pfnL1);
        }

        if (*(l1addr + offsetL1) & 1) {
                dataPFN = *(l1addr + offsetL1) >> 12;
                os_pfn_free(USER_REG, dataPFN);
        }


        /**************** Code Segment *******************/

        unsigned long codeVA = ctx->mms[MM_SEG_CODE].start;
        /* Extract out all 4 level page offsets */
        u32 offsetL4_ = ((1 << 9) - 1) & (codeVA >> 39);
        u32 offsetL3_ = ((1 << 9) - 1) & (codeVA >> 30);
        u32 offsetL2_ = ((1 << 9) - 1) & (codeVA >> 21);
        u32 offsetL1_ = ((1 << 9) - 1) & (codeVA >> 12);

        /* Cleanup Page Table entries */
        if (*(l4addr + offsetL4_) & 1) {
                pfnL3_ = *(l4addr + offsetL4_) >> 12;
                l3addr_ = osmap(pfnL3_);
                os_pfn_free(OS_PT_REG, pfnL3_);
        }

        if (*(l3addr_ + offsetL3_) & 1) {
                pfnL2_ = *(l3addr_ + offsetL3_) >> 12;
                l2addr_ = osmap(pfnL2_);
                os_pfn_free(OS_PT_REG, pfnL2_);
        }

        if (*(l2addr_ + offsetL2_) & 1) {
                pfnL1_ = *(l2addr_ + offsetL2_) >> 12;
                l1addr_ = osmap(pfnL1_);
                os_pfn_free(OS_PT_REG, pfnL1_);
        }

        if (*(l1addr_ + offsetL1_) & 1) {
                dataPFN_ = *(l1addr_ + offsetL1_) >> 12;
                os_pfn_free(USER_REG, dataPFN_);
        }


        /**************** Data Segment *******************/

        unsigned long dataVA = ctx->mms[MM_SEG_DATA].start;
        /* Extract out all 4 level page offsets */
        u32 offsetL4_d = ((1 << 9) - 1) & (dataVA >> 39);
        u32 offsetL3_d = ((1 << 9) - 1) & (dataVA >> 30);
        u32 offsetL2_d = ((1 << 9) - 1) & (dataVA >> 21);
        u32 offsetL1_d = ((1 << 9) - 1) & (dataVA >> 12);

        /* Cleanup Page Table entries */
        if (*(l4addr + offsetL4_d) & 1) {
                pfnL3_d = *(l4addr + offsetL4_d) >> 12;
                l3addr_d = osmap(pfnL3_d);
                os_pfn_free(OS_PT_REG, pfnL3_d);
        }

        if (*(l3addr_d + offsetL3_d) & 1) {
                pfnL2_d = *(l3addr_d + offsetL3_d) >> 12;
                l2addr_d = osmap(pfnL2_d);
                os_pfn_free(OS_PT_REG, pfnL2_d);
        }

        if (*(l2addr_d + offsetL2_d) & 1) {
                pfnL1_d = *(l2addr_d + offsetL2_d) >> 12;
                l1addr_d = osmap(pfnL1_d);
                os_pfn_free(OS_PT_REG, pfnL1_d);
        }

        /* Since I didn't allocate a physical frame for data page */
        /* I'll not clean it up (ctx->arg_pfn was provided to us) */
        return;
}
