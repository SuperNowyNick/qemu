/*
 * QEmu Adaptec AHA2904 SCSI PCI Host Adapter emulation
 * Copyright (c) G.Sobczak 2020
 *
 * Based on Adaptec aic7xxx driver 
 *
 *
 *
 *
 */

#include "qemu/osdep.h"
#include "hw/pci/pci.h"

typedef struct {
	PCIDevice pdev;
	MemoryRegion mmio;
} AHA2904State;

#define AHA2904_VENDOR_ID 0x9004
#define AHA2904_DEVICE_ID 0x5078

#define TYPE_AHA2940 "aha2904"

static uint64_t aha2904_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
	//AHA2904State *aha = opaque;
	uint64_t val = ~0ULL;
	printf("AHA2904 mmio_read addr=%llx size=%x", (unsigned long long )addr, size);
	return val;
}

static void aha2904_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
	AHA2904State *aha = opaque;
	printf("AHA2904 mmio_write addr=%llx, val=%lx, size=%x", (unsigned long long)addr, val, size);
	switch(addr)
	{
		case 0x0:
			pci_set_irq(&aha->pdev, 1);
			break;
		case 0x1:
			pci_set_irq(&aha->pdev, 0);
			break;
	}
}

static const MemoryRegionOps aha2904_mmio_ops = {
	.read = aha2904_mmio_read,
	.write = aha2904_mmio_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static void aha2904_realize(PCIDevice *pdev, Error **errp)
{
	AHA2904State *aha = DO_UPCAST(AHA2904State, pdev, pdev);
	uint8_t *pci_conf = pdev->config;

	pci_config_set_interrupt_pin(pci_conf, 1); // No interrupt pin

	memory_region_init_io(&aha->mmio, OBJECT(aha), &aha2904_mmio_ops, aha, "aha2904-mmio", 8);
	pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &aha->mmio);
}

static void aha2904_exit(PCIDevice *pdev)
{
}

static void aha2904_class_init(ObjectClass *class, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(class);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

	k->realize = aha2904_realize;
	k->exit = aha2904_exit;
	k->vendor_id = AHA2904_VENDOR_ID;
	k->device_id = AHA2904_DEVICE_ID; 
	k->revision = 0x00; 
	k->class_id = PCI_CLASS_OTHERS;
	dc->desc = "AHA2904 SCSI Controller";
	set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}
static InterfaceInfo interfaces[] = {
	{INTERFACE_CONVENTIONAL_PCI_DEVICE },
	{ },
};

static const TypeInfo aha2904_info = {
	.name = "aha2904",
	.parent = TYPE_PCI_DEVICE,
	.instance_size = sizeof(AHA2904State),
	.class_init = aha2904_class_init,
	.interfaces = interfaces,
};



static void aha2904_register_types(void)
{
	type_register_static(&aha2904_info);
}

type_init(aha2904_register_types)


