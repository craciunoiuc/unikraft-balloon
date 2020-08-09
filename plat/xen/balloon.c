#include <uk/plat/common/sections.h>

#include <common/gnttab.h>
#if (defined __X86_32__) || (defined __X86_64__)
#include <xen-x86/setup.h>
#include <xen-x86/mm_pv.h>
#include <xen-x86/mm.h>
#elif (defined __ARM_32__) || (defined __ARM_64__)
#include <xen-arm/setup.h>
#include <xen-arm/mm.h>
#endif


#include <xen/memory.h>
#include <uk/plat/balloon.h>
#include <common/hypervisor.h>

/**
 * set up and call Xen hypercall to ask for memory back from Xen
 */
static int xenmem_reservation_increase(int count, xen_pfn_t *frames, int order)
{
	struct xen_memory_reservation res = {
#if __XEN_INTERFACE_VERSION__ >= 0x00030209
		.memflags = 0;
#else
		.address_bits = 0,
#endif
		.extent_order = order,
		.domid        = DOMID_SELF
	};

	set_xen_guest_handle(res.extent_start, frames);
	res.nr_extents = count;

	/* Needs physical frame number */
	return HYPERVISOR_memory_op(XENMEM_populate_physmap, &res);
}

/**
 * set up and call Xen hypercall to give memory to Xen
 */
static int xenmem_reservation_decrease(int count, xen_pfn_t *frames, int order)
{
	struct xen_memory_reservation res = {
#if __XEN_INTERFACE_VERSION__ >= 0x00030209
		.mem_flags = 0,
#else
		.address_bits = 0,
#endif
		.extent_order = order,
		.domid        = DOMID_SELF
	};

	set_xen_guest_handle(res.extent_start, frames);
	res.nr_extents = count;

	/* Needs guest frame number */
	return HYPERVISOR_memory_op(XENMEM_decrease_reservation, &res);
}

/**
 * When we inflate we will be decreasing the memory available to the VM
 * We will give the extent of extent order = order starting at va to the host.
 */
int ukplat_inflate(void *va, int order)
{
	xen_pfn_t pfn = virt_to_pfn(va);

	return xenmem_reservation_decrease(1, &pfn, order);
}

/**
 * When we deflate we will be increasing the memory available to the VM
 * We will ask for 1 extent of extent order = order back from the host.
 * It will map the extent to the address va.
 */
int ukplat_deflate(void *va, int order)
{
	/* Make sure we are sending the correct frame number. Should be a GFN */
	xen_pfn_t pfn = virt_to_pfn(va);

	return xenmem_reservation_increase(1, &pfn, order);
}
