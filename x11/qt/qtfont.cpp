/*
 * Copyright (c) 2003 NONAKA Kimihiro
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

#include "np2.h"
#include "codecnv.h"

#include "fontmng.h"

#include "qt/xnp2.h"


BOOL
fontmng_init(void)
{

	return SUCCESS;
}

void
fontmng_terminate(void)
{
}

void
fontmng_setdeffontname(const char *name)
{

	UNUSED(name);
}

void *
fontmng_create(int size, UINT type, const TCHAR *fontface)
{

	UNUSED(size);
	UNUSED(type);
	UNUSED(fontface);

	return 0;
}

void
fontmng_destroy(void *hdl)
{

	UNUSED(hdl);
}

BOOL
fontmng_getsize(void *hdl, const char *str, POINT_T *pt)
{

	UNUSED(hdl);
	UNUSED(str);
	UNUSED(pt);

	return FAILURE;
}

BOOL
fontmng_getdrawsize(void *hdl, const char *str, POINT_T *pt)
{

	UNUSED(hdl);
	UNUSED(str);
	UNUSED(pt);

	return FAILURE;
}

FNTDAT
fontmng_get(void* hdl, const char* str)
{

	UNUSED(hdl);
	UNUSED(str);

	return 0;
}
