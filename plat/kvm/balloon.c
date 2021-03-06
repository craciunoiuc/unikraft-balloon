/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cason Schindler & Jack Raney <cason.j.schindler@gmail.com>
 *          Cezar Craciunoiu <cezar.craciunoiu@gmail.com>
 *
 * Copyright (c) 2019, The University of Texas at Austin. All rights reserved.
 *               2020, University Politehnica of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#include <inttypes.h>
#include <errno.h>
#include <stddef.h>

#include <balloon/balloon.h>
#include <uk/plat/balloon.h>
#include <uk/asm/limits.h>

static inline int get_num_pages(int order)
{
	int num_pages = 1;
	int i;

	for (i = 0; i < order; i++)
		num_pages *= 2;
	return num_pages;
}

/**
 * Fills addresses for page range starting at first_page.
 */
static inline void fill_page_array(uintptr_t *pages_array,
			void *first_page, int num_pages)
{
	uint64_t current_pg = (uint64_t) first_page;
	int i;

	for (i = 0; i < num_pages; i++) {
		pages_array[i] = current_pg;
		current_pg += __PAGE_SIZE;
	}
}

/**
 * Calls driver inflate_balloon
 * returns number of pages actually put into balloon or < 0 on error
 */
int ukplat_inflate(void *page, int order)
{
	int num_pages = get_num_pages(order);
	uintptr_t pages_to_host[num_pages];

	if (page == NULL)
		return -EINVAL;

	fill_page_array(pages_to_host, page, num_pages);
	return inflate_balloon(pages_to_host, num_pages);
}

/**
 * Call driver deflate_balloon
 * returns number of pages actually taken from balloon or < 0 on error
 */
int ukplat_deflate(void *page, int order)
{
	int num_pages = get_num_pages(order);
	uintptr_t pages_to_guest[num_pages];

	if (page == NULL)
	return -EINVAL;

	fill_page_array(pages_to_guest, page, num_pages);
	return deflate_balloon(pages_to_guest, num_pages);
}

/**
 * Sets behaviour for KVM.
 */
void ukplat_balloon_set(char *balloon_type)
{
	*balloon_type = 2;
}
