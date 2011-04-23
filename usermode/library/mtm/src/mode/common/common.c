/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

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
