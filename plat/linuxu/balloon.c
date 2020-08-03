#include <errno.h>
#include <uk/plat/balloon.h>
#include <uk/assert.h>

/* Memory ballooning is not yet supported for linuxu */

int ukplat_inflate(void *va __unused, int order __unused)
{
	return -ENOSYS;
}

int ukplat_deflate(void *va __unused, int order __unused)
{
	return -ENOSYS;
}
