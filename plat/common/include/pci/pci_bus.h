/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
/*
 * Copyright(c) 2010-2015 Intel Corporation.
 * Copyright 2013-2014 6WIND S.A.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
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
 */

#ifndef __UKPLAT_COMMON_PCI_BUS_H__
#define __UKPLAT_COMMON_PCI_BUS_H__

#include <uk/bus.h>
#include <uk/alloc.h>

/**
 * A structure describing an ID for a PCI driver. Each driver provides a
 * table of these IDs for each device that it supports.
 *  * Derived from: lib/librte_pci/rte_pci.h
 */
struct pci_device_id {
	/**< Class ID or PCI_CLASS_ANY_ID. */
	uint32_t class_id;
	/**< Vendor ID or PCI_ANY_ID. */
	uint16_t vendor_id;
	/**< Device ID or PCI_ANY_ID. */
	uint16_t device_id;
	/**< Subsystem vendor ID or PCI_ANY_ID. */
	uint16_t subsystem_vendor_id;
	/**< Subsystem device ID or PCI_ANY_ID. */
	uint16_t subsystem_device_id;
};

/** Any PCI device identifier (vendor, device, ...) */
#define PCI_ANY_ID       (0xffff)
#define PCI_CLASS_ANY_ID (0xffffff)

/**
 * Macros used to help building up tables of device IDs
 * Derived from: lib/librte_pci/rte_pci.h
 */
#define PCI_DEVICE_ID(vend, dev)           \
	.class_id = PCI_CLASS_ANY_ID,      \
	.vendor_id = (vend),               \
	.device_id = (dev),                \
	.subsystem_vendor_id = PCI_ANY_ID, \
	.subsystem_device_id = PCI_ANY_ID

#define PCI_ANY_DEVICE_ID                  \
	.class_id = PCI_CLASS_ANY_ID,      \
	.vendor_id = PCI_ANY_ID,           \
	.device_id = PCI_ANY_ID,           \
	.subsystem_vendor_id = PCI_ANY_ID, \
	.subsystem_device_id = PCI_ANY_ID

/**
 * A structure describing the location of a PCI device.
 * Derived from: lib/librte_pci/rte_pci.h
 */
struct pci_address {
	/**< Device domain */
	uint32_t domain;
	/**< Device bus */
	uint8_t bus;
	/**< Device ID */
	uint8_t devid;
	/**< Device function. */
	uint8_t function;
};

struct pci_device;

typedef int (*pci_driver_add_func_t)(struct pci_device *);
typedef int (*pci_driver_init_func_t)(struct uk_alloc *a);

struct pci_driver {
	UK_TAILQ_ENTRY(struct pci_driver) next;
	/**< ANY-ID terminated list of device IDs that the driver handles */
	const struct pci_device_id *device_ids;
	pci_driver_init_func_t init; /* optional */
	pci_driver_add_func_t add_dev;
};
UK_TAILQ_HEAD(pci_driver_list, struct pci_driver);

enum pci_device_state {
	PCI_DEVICE_STATE_RESET = 0,
	PCI_DEVICE_STATE_RUNNING
};

struct pci_device {
	UK_TAILQ_ENTRY(struct pci_device) next; /**< used by pci_bus_handler */
	struct pci_device_id  id;
	struct pci_address    addr;
	struct pci_driver     *drv;
	enum pci_device_state state;

	uint16_t base;
	unsigned long irq;
};
UK_TAILQ_HEAD(pci_device_list, struct pci_device);


#define PCI_REGISTER_DRIVER(b)                  \
	_PCI_REGISTER_DRIVER(__LIBNAME__, b)

#define _PCI_REGFNNAME(x, y)      x##y

#define _PCI_REGISTER_DRIVER(libname, b)				\
	static void __constructor_prio(103)				\
	_PCI_REGFNNAME(libname, _pci_register_driver)(void)		\
	{								\
		_pci_register_driver((b));				\
	}

/* Do not use this function directly: */
void _pci_register_driver(struct pci_driver *drv);


/* TODO: Move these I/O functions to architecture/platform specific
 *       implementation. They are used to interact with the PCI bus.
 *       They work for x86_64 only for now
 */
static inline uint8_t inb(uint16_t port)
{
	uint8_t v;

	__asm__ __volatile__("inb %1,%0" : "=a" (v) : "dN" (port));
	return v;
}
static inline uint16_t inw(uint16_t port)
{
	uint16_t v;

	__asm__ __volatile__("inw %1,%0" : "=a" (v) : "dN" (port));
	return v;
}
static inline uint32_t inl(uint16_t port)
{
	uint32_t v;

	__asm__ __volatile__("inl %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline uint64_t inq(uint16_t port_lo)
{
	uint16_t port_hi = port_lo + 4;
	uint32_t lo, hi;

	__asm__ __volatile__("inl %1,%0" : "=a" (lo) : "dN" (port_lo));
	__asm__ __volatile__("inl %1,%0" : "=a" (hi) : "dN" (port_hi));

	return ((uint64_t)lo) | ((uint64_t)hi << 32);
}

static inline void outb(uint16_t port, uint8_t v)
{
	__asm__ __volatile__("outb %0,%1" : : "a" (v), "dN" (port));
}

static inline void outw(uint16_t port, uint16_t v)
{
	__asm__ __volatile__("outw %0,%1" : : "a" (v), "dN" (port));
}

static inline void outl(uint16_t port, uint32_t v)
{
	__asm__ __volatile__("outl %0,%1" : : "a" (v), "dN" (port));
}

#endif /* __UKPLAT_COMMON_PCI_BUS_H__ */