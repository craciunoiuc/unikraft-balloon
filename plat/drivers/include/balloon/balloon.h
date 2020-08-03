
#ifndef _PLAT_DRV_BALLOON_H_
#define _PLAT_DRV_BALLOON_H_

#include <inttypes.h>
#include <sys/types.h>

int deflate_balloon(uintptr_t *pages_to_guest, uint32_t num);

int inflate_balloon(uintptr_t *pages_to_host, uint32_t num);

#endif /* _PLAT_DRV_BALLOON_H_ */