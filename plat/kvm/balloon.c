#include <inttypes.h>
#include <balloon/balloon.h>
#include <uk/plat/balloon.h>
#include <uk/asm/limits.h>

/**
 * number of pages is 2^order
 */
static inline int get_num_pages(int order)
{
	int num_pages = 1;

	for (int i = 0; i < order; i++)
		num_pages *= 2;
	return num_pages;
}

/**
 * fill addresses for page range starting at first_page
 */
static inline void fill_page_array(uintptr_t *pages_array,
			void *first_page, int num_pages)
{
	uint64_t current_pg = (uint64_t) first_page;

	for (int i = 0; i < num_pages; i++) {
		pages_array[i] = current_pg;
		current_pg += __PAGE_SIZE;
	}
}

/**
 * call driver inflate_balloon
 * returns number of pages actually put into balloon or < 0 on error
 */
int ukplat_inflate(void *page, int order)
{
	int num_pages = get_num_pages(order);
	uintptr_t pages_to_host[num_pages];

	fill_page_array(pages_to_host, page, num_pages);
	// uk_pr_err("inflate = %d\n", num_pages);
	return inflate_balloon(pages_to_host, num_pages);
}

/**
 * call driver deflate_balloon
 * returns number of pages actually taken from balloon or < 0 on error
 */
int ukplat_deflate(void *page, int order)
{
	int num_pages = get_num_pages(order);
	uintptr_t pages_to_guest[num_pages];

	fill_page_array(pages_to_guest, page, num_pages);
	// uk_pr_err("deflate = %d\n", num_pages);
	return deflate_balloon(pages_to_guest, num_pages);
}
