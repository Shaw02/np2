/*	$Id: interface.c,v 1.7 2004/01/23 14:33:26 monaka Exp $	*/

/*
 * Copyright (c) 2002-2003 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"
#include "cpu.h"
#include "ia32.mcr"

#include "dmap.h"
#include "bios.h"


void
ia32reset(void)
{
	int i;

	memset(&i386core.s, 0, sizeof(i386core.s));
	CPU_STATSAVE.cpu_inst_default.seg_base = (DWORD)-1;

	CPU_EDX = (CPU_FAMILY << 8) | (CPU_MODEL << 4) | CPU_STEPPING;
	CPU_EFLAG = 2;
	CPU_CR0 = CPU_CR0_CD | CPU_CR0_NW | CPU_CR0_ET;
#ifndef USE_FPU
	CPU_CR0 |= CPU_CR0_EM | CPU_CR0_NE;
	CPU_CR0 &= ~CPU_CR0_MP;
#endif
	CPU_MXCSR = 0x1f80;
	CPU_GDTR_LIMIT = 0xffff;
	CPU_IDTR_LIMIT = 0xffff;

#if CPU_FAMILY == 4
	CPU_STATSAVE.cpu_regs.dr[6] = 0xffff1ff0;
#elif (CPU_FAMILY == 5) || (CPU_FAMILY == 6)
	CPU_STATSAVE.cpu_regs.dr[6] = 0xffff0ff0;
	CPU_STATSAVE.cpu_regs.dr[7] = 0x00000400;
#endif

	for (i = 0; i < CPU_SEGREG_NUM; ++i) {
		CPU_STAT_SREG_INIT(i);
	}
	CPU_LDTR_LIMIT = 0xffff;
	CPU_TR_LIMIT = 0xffff;

	CPU_SET_SEGREG(CPU_CS_INDEX, 0xf000);
	CPU_EIP = 0xfff0;
	CPU_ADRSMASK = 0x000fffff;
}

void
ia32shut(void)
{
	SINT32	remainclock;			/* 結局ハマるのか漏れ… */
	SINT32	baseclock;
	UINT32	clock;

	remainclock = CPU_REMCLOCK;
	baseclock = CPU_BASECLOCK;
	clock = CPU_CLOCK;

	ia32reset();

	CPU_REMCLOCK = remainclock;
	CPU_BASECLOCK = baseclock;
	CPU_CLOCK = clock;
}

void
ia32(void)
{
	int rv;

	rv = sigsetjmp(exec_1step_jmpbuf, 1);
	switch (rv) {
	case 0:
		break;

	case 1:
		VERBOSE(("ia32: return from exception"));
		break;

	default:
		VERBOSE(("ia32: return from unknown cause"));
		break;
	}

	do {
		exec_1step();
	} while (CPU_REMCLOCK > 0);
}

void
ia32withtrap(void)
{
	int rv;

	rv = sigsetjmp(exec_1step_jmpbuf, 1);
	switch (rv) {
	case 0:
		break;

	case 1:
		VERBOSE(("ia32withtrap: return from exception"));
		break;

	default:
		VERBOSE(("ia32withtrap: return from unknown cause"));
		break;
	}

	do {
		exec_1step();
		if (CPU_TRAP) {
			ia32_interrupt(1);
		}
	} while (CPU_REMCLOCK > 0);
}

void
ia32withdma(void)
{
	int rv;

	rv = sigsetjmp(exec_1step_jmpbuf, 1);
	switch (rv) {
	case 0:
		break;

	case 1:
		VERBOSE(("ia32withdma: return from exception"));
		break;

	default:
		VERBOSE(("ia32withdma: return from unknown cause"));
		break;
	}

	do {
		exec_1step();
		dmap_i286();
	} while (CPU_REMCLOCK > 0);
}

void
ia32_step(void)
{
	int rv;

	rv = sigsetjmp(exec_1step_jmpbuf, 1);
	switch (rv) {
	case 0:
		break;

	case 1:
		VERBOSE(("ia32_step: return from exception"));
		break;

	default:
		VERBOSE(("ia32_step: return from unknown cause"));
		break;
	}

	do {
		exec_1step();
		if (CPU_TRAP) {
			ia32_interrupt(1);
		}
		dmap_i286();
	} while (CPU_REMCLOCK > 0);
}

void CPUCALL
ia32_interrupt(BYTE vect)
{

	INTERRUPT(vect, 0, 0, 0);
}


/*
 * error function
 */
void
ia32_panic(const char *str, ...)
{
	extern char *cpu_reg2str(void);
	char buf[2048];
	va_list ap;

	va_start(ap, str);
	vsnprintf(buf, sizeof(buf), str, ap);
	va_end(ap);
	strcat(buf, "\n");
	strcat(buf, cpu_reg2str());

	msgbox("ia32_panic", buf);

	__ASSERT(0);
	exit(1);
}

void
ia32_warning(const char *str, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, str);
	vsnprintf(buf, sizeof(buf), str, ap);
	va_end(ap);
	strcat(buf, "\n");

	msgbox("ia32_warning", buf);
}

void
ia32_printf(const char *str, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, str);
	vsnprintf(buf, sizeof(buf), str, ap);
	va_end(ap);
	strcat(buf, "\n");

	msgbox("ia32_printf", buf);
}


/*
 * bios call interface
 */
void
ia32_bioscall(void)
{
	DWORD adrs;

	if (!CPU_STAT_PM && !CPU_INST_OP32 && !CPU_INST_AS32) {
		adrs = ((CPU_IP-1) & 0xffff) + CPU_STAT_SREGBASE(CPU_CS_INDEX);
		if ((adrs >= 0xf8000) && (adrs < 0x100000)) {
			biosfunc(adrs);
			CPU_SET_SEGREG(CPU_ES_INDEX, CPU_ES);
			CPU_SET_SEGREG(CPU_CS_INDEX, CPU_CS);
			CPU_SET_SEGREG(CPU_SS_INDEX, CPU_SS);
			CPU_SET_SEGREG(CPU_DS_INDEX, CPU_DS);
		}
	}
}
