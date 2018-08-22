#include<context.h>
#include<memory.h>
#include<lib.h>

void prepare_context_mm(struct exec_context *ctx)
{
        /*  PFN of the first level translation */
        ctx->pgd = os_pfn_alloc(OS_PT_REG);
        unsigned long *l4addr = osmap(ctx->pgd);


        /**************** Stack Segment *******************/

        unsigned long stackVA = ctx->mms[MM_SEG_STACK].start;
        /* Extract out all 4 level page offsets */
        u32 offsetL4 = ((1 << 9) - 1) & (stackVA >> 39);
        u32 offsetL3 = ((1 << 9) - 1) & (stackVA >> 30);
        u32 offsetL2 = ((1 << 9) - 1) & (stackVA >> 21);
        u32 offsetL1 = ((1 << 9) - 1) & (stackVA >> 12);

        // /* Fill up Page Table Pages entries */
        // u32 pfnL3 = os_pfn_alloc(OS_PT_REG);
        // *(l4addr + offsetL4) = (pfnL3 << 12) | 7;
        //
        // unsigned long *l3addr = osmap(pfnL3);
        // u32 pfnL2 = os_pfn_alloc(OS_PT_REG);
        // *(l3addr + offsetL3) = (pfnL2 << 12) | 7;
        //
        // unsigned long *l2addr = osmap(pfnL2);
        // u32 pfnL1 = os_pfn_alloc(OS_PT_REG);
        // *(l2addr + offsetL2) = (pfnL1 << 12) | 7;
        //
        // unsigned long *l1addr = osmap(pfnL1);
        // u32 dataPFN = os_pfn_alloc(USER_REG);
        // *(l1addr + offsetL1) = (dataPFN << 12) | 7;

        /* Fill up Page Table Pages entries */
        u32 pfnL3;
        if (*(l4addr + offsetL4) & 1)        // If PT already present
                pfnL3 = *(l4addr + offsetL4) >> 12;
        else {
                pfnL3 = os_pfn_alloc(OS_PT_REG);
                *(l4addr + offsetL4) = (pfnL3 << 12) | 7;
        }

        unsigned long *l3addr = osmap(pfnL3);
        u32 pfnL2;
        if (*(l3addr + offsetL3) & 1)        // If PT already present
                pfnL2 = *(l3addr + offsetL3) >> 12;
        else {
                pfnL2 = os_pfn_alloc(OS_PT_REG);
                *(l3addr + offsetL3) = (pfnL2 << 12) | 7;
        }

        unsigned long *l2addr = osmap(pfnL2);
        u32 pfnL1;
        if (*(l2addr + offsetL2) & 1)        // If PT already present
                pfnL1 = *(l2addr + offsetL2) >> 12;
        else {
                pfnL1 = os_pfn_alloc(OS_PT_REG);
                *(l2addr + offsetL2) = (pfnL1 << 12) | 7;
        }

        /* Final mapping to Data Physical Page */
        unsigned long *l1addr = osmap(pfnL1);
        u32 dataPFN;
        if (*(l1addr + offsetL1) & 1)        // If PT already present
                dataPFN = *(l1addr + offsetL1) >> 12;
        else {
                dataPFN = os_pfn_alloc(OS_PT_REG);
                *(l1addr + offsetL1) = (dataPFN << 12) | 7;
        }

        /**************** Code Segment *******************/

        unsigned long codeVA = ctx->mms[MM_SEG_CODE].start;
        /* Extract out all 4 level page offsets */
        u32 offsetL4_ = ((1 << 9) - 1) & (codeVA >> 39);
        u32 offsetL3_ = ((1 << 9) - 1) & (codeVA >> 30);
        u32 offsetL2_ = ((1 << 9) - 1) & (codeVA >> 21);
        u32 offsetL1_ = ((1 << 9) - 1) & (codeVA >> 12);

        /* Fill up Page Table Pages entries */
        u32 pfnL3_;
        if (*(l4addr + offsetL4_) & 1)        // If PT already present
                pfnL3_ = *(l4addr + offsetL4_) >> 12;
        else {
                pfnL3_ = os_pfn_alloc(OS_PT_REG);
                *(l4addr + offsetL4_) = (pfnL3_ << 12) | 7;
        }

        unsigned long *l3addr_ = osmap(pfnL3_);
        u32 pfnL2_;
        if (*(l3addr_ + offsetL3_) & 1)        // If PT already present
                pfnL2_ = *(l3addr_ + offsetL3_) >> 12;
        else {
                pfnL2_ = os_pfn_alloc(OS_PT_REG);
                *(l3addr + offsetL3_) = (pfnL2_ << 12) | 7;
        }

        unsigned long *l2addr_ = osmap(pfnL2_);
        u32 pfnL1_;
        if (*(l2addr_ + offsetL2_) & 1)        // If PT already present
                pfnL1_ = *(l2addr_ + offsetL2_) >> 12;
        else {
                pfnL1_ = os_pfn_alloc(OS_PT_REG);
                *(l2addr + offsetL2_) = (pfnL1_ << 12) | 7;
        }

        /* Final mapping to Data Physical Page */
        unsigned long *l1addr_ = osmap(pfnL1_);
        u32 dataPFN_;
        if (*(l1addr_ + offsetL1_) & 1)        // If PT already present
                dataPFN_ = *(l1addr_ + offsetL1_) >> 12;
        else {
                dataPFN_ = os_pfn_alloc(OS_PT_REG);
                *(l1addr + offsetL1_) = (dataPFN_ << 12) | 7;
        }

        // unsigned long *l3addr_ = osmap(pfnL3_);
        //
        // u32 pfnL2_ = os_pfn_alloc(OS_PT_REG);
        // *(l3addr_ + offsetL3_) = (pfnL2_ << 12) | 7;
        //
        // unsigned long *l2addr_ = osmap(pfnL2_);
        // u32 pfnL1_ = os_pfn_alloc(OS_PT_REG);
        // *(l2addr_ + offsetL2_) = (pfnL1_ << 12) | 7;
        //
        // unsigned long *l1addr_ = osmap(pfnL1_);
        // u32 dataPFN_ = os_pfn_alloc(USER_REG);
        // *(l1addr_ + offsetL1_) = (dataPFN_ << 12) | 7;



        // Debug
        // u32 a = ctx->mms[MM_SEG_STACK].access_flags;
        // printf("%x\n", a);
        // unsigned long b = ctx->mms[MM_SEG_STACK].end;
        printf("%x %x %x\n", *(l4addr + offsetL4), *(l4addr + offsetL4_) & 1, (pfnL3 << 12) | 7);
        printf("%x %x %x\n", *(l2addr + offsetL2_), *(l4addr + offsetL4_) & 1, (pfnL1_ << 12) | 7);
        printf("%x %d %d\n", l2addr, offsetL2, pfnL2);
        printf("%x %d %d\n", l1addr, offsetL1, pfnL1);

        return;
}
void cleanup_context_mm(struct exec_context *ctx)
{

        return;
}
