#ifndef __UKPLAT_BALLOON_H__
#define __UKPLAT_BALLOON_H__

/**
 * Inflates the memory balloon by 1 extent. After this point, the extent
 * starting at the specified virtual address will be unavailable to the guest.
 * The host will then be able to use the extent for its own purposes.
 * @param va The starting virtual address to release
 * @param order The sie of the extent to be released
 * @return >= 0 on success, < 0 otherwise.
 */
int ukplat_inflate(void *va, int order);

/**
 * Deflates the memory balloon by 1 extent, and returns a frame to the
 * hypervisor starting at the specified virtual address.
 * @param va The starting virtual address to recover
 * @param order The size of the extent to be recovered
 * @return num pages reclaimed on success, < 0 otherwise.
 */
int ukplat_deflate(void *va, int order);

#endif /* __UKPLAT_BALLOON_H__ */
