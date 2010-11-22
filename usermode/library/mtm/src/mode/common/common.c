#include "mtm_i.h"

void
mtm_print_properties(uint32_t prop) 
{
	printf("pr_instrumentedCode     = %d\n", (prop & pr_instrumentedCode) ? 1 : 0);
	printf("pr_uninstrumentedCode   = %d\n", (prop & pr_uninstrumentedCode) ? 1 : 0);
	printf("pr_multiwayCode         = %d\n", (prop & pr_multiwayCode) ? 1 : 0);
	printf("pr_hasNoXMMUpdate       = %d\n", (prop & pr_hasNoXMMUpdate) ? 1 : 0);
	printf("pr_hasNoAbort           = %d\n", (prop & pr_hasNoAbort) ? 1 : 0);
	printf("pr_hasNoRetry           = %d\n", (prop & pr_hasNoRetry) ? 1 : 0);
	printf("pr_hasNoIrrevocable     = %d\n", (prop & pr_hasNoIrrevocable) ? 1 : 0);
	printf("pr_doesGoIrrevocable    = %d\n", (prop & pr_doesGoIrrevocable) ? 1 : 0);
	printf("pr_hasNoSimpleReads     = %d\n", (prop & pr_hasNoSimpleReads) ? 1 : 0);
	printf("pr_aWBarriersOmitted    = %d\n", (prop & pr_aWBarriersOmitted) ? 1 : 0);
	printf("pr_RaRBarriersOmitted   = %d\n", (prop & pr_RaRBarriersOmitted) ? 1 : 0);
	printf("pr_undoLogCode          = %d\n", (prop & pr_undoLogCode) ? 1 : 0);
	printf("pr_preferUninstrumented = %d\n", (prop & pr_preferUninstrumented) ? 1 : 0);
	printf("pr_exceptionBlock       = %d\n", (prop & pr_exceptionBlock) ? 1 : 0);
	printf("pr_hasElse              = %d\n", (prop & pr_hasElse) ? 1 : 0);
}
