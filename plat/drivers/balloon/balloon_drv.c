
#include <uk/plat/common/sections.h>
#include <sys/types.h>
#include <uk/assert.h>
#include <kvm/config.h>
#include <inttypes.h>
#include <uk/alloc.h>
#include <uk/sglist.h>
#include <uk/list.h>
#include <uk/assert.h>
//#include <uk/mutex.h>
#include <virtio/virtio_ids.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtqueue.h>
#include <balloon/balloon.h>

#define DRIVER_NAME "virtio-balloon"
#define VTBALLOON_PAGES_PER_REQUEST	256

static struct uk_alloc *a;

static struct virtio_balloon_device *global_vb;

/* pages given to hypervisor (in the balloon) */
struct balloon_pages {

	uint32_t num_pages; /* # pages in balloon */

};

/* temporary storage for pages with which we are either
 * inflating or deflating the balloon
 */
struct transport_pages {

	uint32_t num_pages; /* # of pages in pages */
	uint32_t *pages;

};

/* wrapper for virtio device */
struct virtio_balloon_device {

	struct virtio_dev *vdev;

	struct virtqueue *inflate_vq, *deflate_vq;

	__u16 infvq_id;
	__u16 defvq_id;

	char *tag;

	struct balloon_pages *balloon;

	struct transport_pages *transport;

	uint64_t features;
	uint32_t flags;

	//uk_mutex lock;

};

static void clear_transport(struct virtio_balloon_device *vb)
{
	int num = vb->transport->num_pages;

	for (int i = 0; i < num; i++) {
		(vb->transport->pages)[i] = 0;
		vb->transport->num_pages -= 1;
	}
}

/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2011, Bryan Venteicher <bryanv@FreeBSD.org>
 * All rights reserved.
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

/* The above copyright notice applies only to the below function,
 * vtballoon_send_page_frames, which is based on FreeBSD's function
 * of the same name.
 */

static void vtballoon_send_page_frames(struct virtio_balloon_device *vb,
			struct virtqueue *vq, int npages)
{
	struct uk_sglist sg;
	struct uk_sglist_seg segs[1];
	int c;
	void *vq_cookie;
	__u32 len = 0;

	uk_sglist_init(&sg, 1, segs);

	uk_sglist_append(&sg, vb->transport->pages, npages * sizeof(uint32_t));

	virtqueue_buffer_enqueue(vq, &vq_cookie, &sg, 1, 0);

	virtqueue_host_notify(vq);

	/* wait on KVM to respond. Need a safer method for this */
	while ((c = virtqueue_buffer_dequeue(vq, &vq_cookie, &len)) < 0);

}

/**
 * this is equivalent to leaking from the balloon and
 * increasing memory reservation for guest
 */
int deflate_balloon(uintptr_t *pages_to_guest, uint32_t num)
{
	/* get pages_to_guest from the balloon and
	 *tell host we are using them now
	 */

	/* check if device is ready */
	if (!global_vb)
		return -ENXIO;

	struct virtio_balloon_device *vb = global_vb;

	//uk_mutex_lock(vb->lock);

	clear_transport(vb);

	if (vb->balloon->num_pages < num)
		num = vb->balloon->num_pages;

	for (uint32_t i = 0; i < num; i++) {
		uint32_t page = pages_to_guest[i];
		 /* put page in temp array for host */
		vb->transport->pages[i] = page;
		vb->balloon->num_pages -= 1;
		vb->transport->num_pages += 1;
	}

	int num_pages_taken = vb->transport->num_pages;

	if (vb->transport->num_pages != 0) {
		vtballoon_send_page_frames(vb, vb->deflate_vq,
			vb->transport->num_pages);
	}

	//uk_mutex_unlock(vb->lock);

	uk_pr_err("Finished deflation pages_taken=%d\n", num_pages_taken);
	return num_pages_taken;
}

/**
 * this is equivalent to filling the balloon and
 * decreasing memory reservation for guest
 */
int inflate_balloon(uintptr_t *pages_to_host, uint32_t num)
{
	/* need to put pages_to_host in the balloon for the host to use */

	/* check if device is ready */
	if (!global_vb)
		return -ENXIO;

	struct virtio_balloon_device *vb = global_vb;

	//uk_mutex_lock(vb->lock);

	clear_transport(vb);

	for (uint32_t i = 0; i < num; i++) {
		uint32_t page = pages_to_host[i] / __PAGE_SIZE;
		/* put page in temp array for host */
		vb->transport->pages[i] = page;
		vb->balloon->num_pages += 1;
		vb->transport->num_pages += 1;
	}

	int num_pages_given = vb->transport->num_pages;

	if (vb->transport->num_pages != 0) {
		vtballoon_send_page_frames(vb, vb->inflate_vq,
			vb->transport->num_pages);
	}

	//uk_mutex_unlock(vb->lock);

	// uk_pr_err("inflate.pages_given = %d\n", num_pages_given);
	return num_pages_given;
}


static inline void virtio_balloon_feature_set(struct virtio_balloon_device *vb)
{
	vb->features = 0;
	vb->flags = 0;
	vb->vdev->features = 0;
}

static int virtio_balloon_vq_alloc(struct virtio_balloon_device *vb)
{
	int vq_avail = 0;
	int rc = 0;
	__u16 qdesc_size[2];

	vq_avail = virtio_find_vqs(vb->vdev, 2, &(qdesc_size[0]));
	if (unlikely(vq_avail != 2)) {
		uk_pr_err(DRIVER_NAME": Expected: %d queues, found %d\n",
			  2, vq_avail);
		rc = -ENOMEM;
		goto exit;
	}

	vb->infvq_id = 0;
	vb->defvq_id = 1;

	vb->inflate_vq = virtio_vqueue_setup(vb->vdev, vb->infvq_id,
			qdesc_size[0], NULL, a);
	vb->inflate_vq->priv = vb;

	if (unlikely(PTRISERR(vb->inflate_vq))) {
		uk_pr_err(DRIVER_NAME": Failed to set up virtqueue %"PRIu16"\n",
			vb->infvq_id);
		rc = PTR2ERR(vb->inflate_vq);
	}

	vb->deflate_vq = virtio_vqueue_setup(vb->vdev, vb->defvq_id,
			qdesc_size[1], NULL, a);
	vb->deflate_vq->priv = vb;

	if (unlikely(PTRISERR(vb->deflate_vq))) {
		uk_pr_err(DRIVER_NAME": Failed to set up virtqueue %"PRIu16"\n",
			vb->defvq_id);
		rc = PTR2ERR(vb->deflate_vq);
	}

exit:
	return rc;
}

static int virtio_balloon_start(struct virtio_balloon_device *vb)
{
	uk_pr_err("START\n");
	virtqueue_intr_enable(vb->inflate_vq);
	virtqueue_intr_enable(vb->deflate_vq);
	virtio_dev_drv_up(vb->vdev);
	uk_pr_info(DRIVER_NAME": %s started\n", vb->tag);

	return 0;
}

/* TODO never called */
static int virtio_balloon_add_dev(struct virtio_dev *vdev)
{

	struct virtio_balloon_device *vbdev;
	int rc = 0;
	int tag_len = 30;

	uk_pr_crit("Adding Device\n");
	UK_ASSERT(vdev != NULL);

	vbdev = uk_calloc(a, 1, sizeof(*vbdev));

	if (!vbdev) {
		rc = -ENOMEM;
		goto err_out;
	}

	vbdev->tag = uk_calloc(a, 1, sizeof(tag_len));
	vbdev->tag = "VIRTIO_BALLOON_DRV_DEV";

	//uk_mutex_init(&vbdev->lock);

	vbdev->vdev = vdev;
	virtio_balloon_feature_set(vbdev);
	rc = virtio_balloon_vq_alloc(vbdev);
	if (rc)
		goto err_out;

	vbdev->transport = uk_calloc(a, 1, sizeof(struct transport_pages));
	if (!(vbdev->transport)) {
		rc = -ENOMEM;
		goto err_out;
	}
	vbdev->transport->pages = uk_calloc(a, 1,
			VTBALLOON_PAGES_PER_REQUEST * sizeof(uint32_t));
	if (!(vbdev->transport->pages)) {
		rc = -ENOMEM;
		goto err_out;
	}
	vbdev->balloon = uk_calloc(a, 1, sizeof(struct balloon_pages));

	rc = virtio_balloon_start(vbdev);
	if (rc)
		goto err_out;

exit:
	global_vb = vbdev; /* initialize global vb */
	/* initial alloc and free to trigger ballon init */
	void *alc = uk_palloc(a, 0);

	uk_pfree(a, alc, 0);
	return rc;
err_out:
	uk_free(a, vbdev->transport->pages);
	uk_free(a, vbdev->transport);
	uk_free(a, vbdev->balloon);
	uk_free(a, vbdev);
	goto exit;

}

static int virtio_balloon_drv_init(struct uk_alloc *drv_allocator)
{
	uk_pr_err("INIT Driver\n");
	/* driver initialization */
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;
    // uk_list_add_tail();
	return 0;

}

static const struct virtio_dev_id vballoon_dev_id[] = {
	{VIRTIO_ID_BALLOON},
	{VIRTIO_ID_INVALID} /* List Terminator */
};

static struct virtio_driver virtio_balloon_driver = {
	.dev_ids = vballoon_dev_id,
	.init	 = virtio_balloon_drv_init,
	.add_dev = virtio_balloon_add_dev
};
VIRTIO_BUS_REGISTER_DRIVER(&virtio_balloon_driver);
