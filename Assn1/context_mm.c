#include<context.h>
#include<memory.h>
#include<lib.h>

void prepare_context_mm(struct exec_context *ctx)
{
        /*  PFN of the first level translation */
        ctx->pgd = os_pfn_alloc(OS_PT_REG);
        u32 *l4addr = osmap(ctx->pgd);

        unsigned long stackVA = ctx->mms[MM_SEG_STACK].start;
        /* Extract out all 4 level page offsets */
        u32 offsetL4 = ((1 << 9) - 1) & (stackVA >> 39);
        u32 offsetL3 = ((1 << 9) - 1) & (stackVA >> 30);
        u32 offsetL2 = ((1 << 9) - 1) & (stackVA >> 21);
        u32 offsetL1 = ((1 << 9) - 1) & (stackVA >> 12);

        /* Fill up Page Table Pages entries */
        u32 pfnL3 = os_pfn_alloc(OS_PT_REG);
        *(l4addr + offsetL4) = pfnL3;
        u32 *l3addr = osmap(pfnL3);

        u32 pfnL2 = os_pfn_alloc(OS_PT_REG);
        *(l3addr + offsetL3) = pfnL2;
        u32 *l2addr = osmap(pfnL2);

        u32 pfnL1 = os_pfn_alloc(OS_PT_REG);
        *(l2addr + offsetL2) = pfnL1;
        u32 *l1addr = osmap(pfnL1);

        u32 dataPFN = os_pfn_alloc(USER_REG);
        *(l1addr + offsetL1) = dataPFN;


        // Debug
        // unsigned long a = ctx->mms[MM_SEG_STACK].start;
        // unsigned long b = ctx->mms[MM_SEG_STACK].end;
        printf("%x\n%x\n", l4addr, offsetL4);
        printf("%x\n%x\n", l3addr, offsetL3);
        printf("%x\n%x\n", l2addr, offsetL2);
        printf("%x\n%x\n", l1addr, offsetL1);

        // debug output
        // 0x2003000
        // 0x0
        // 0x2004000
        // 0x1F
        // 0x2005000
        // 0x1F8
        // 0x2006000
        // 0x0

        return;
}
void cleanup_context_mm(struct exec_context *ctx)
{
   return;
}
