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
#include "hw/qdev-properties.h"

#define MMIO_SIZE 32

typedef struct {
	PCIDevice pdev;

	MemoryRegion mmio;
	// this struct should also have ram for 512 sequencer instructions
	bool bInt_enabled; // are interrupts enabled
	bool bErrorInt_enabled; // are error interrupts enabled?
	bool bPower_down; // 
	bool bPaused;
	// TODO: Implement registers here
	uint64_t membar_size;
} AHA2904State;

#define AHA2904_VENDOR_ID 0x9004
#define AHA2904_DEVICE_ID 0x5078

#define TYPE_AHA2904 "aha2904"
#define AHA2904_DEV(obj) \
    OBJECT_CHECK(AHA2904State, (obj), TYPE_AHA2904)


// TODO: Add DMA support for registers listed below

/* Following two funcitons should realize reading/writing
 * from all possible registers listed below
 *
 * These were taken from aic7xxx.reg
 * 0x000 SCSISEQ(RW) - SCSI Sequence Control
 * 0x001 SXFRCTL0(RW) - SCSI Transfer Control 0 Register
 * 0x002 SXFRCTL1(RW) - 		      1
 * 0x003 SCSISIGI(RO) - SCSI Control Signal Read Register
 * 0x003 SCSISIGO(WO) -                     Write
 * 0x004 SCSIRATE(RW) - SCSI Rate Control
 * 0x005 SCSIID(RW) - SCSI ID of the board on selected channel
 * 0x006 SCSIDATL(RW)
 * 0x007 SCSIDATH(RW)
 * 0x008 STCNT(RW)
 * 0x00b CLRSINT0(WO) - Clear SCSI Interrupt 0
 * 0x00b SSTAT0(RO) - SCSI Status 0
 * 0x00c CLRSINT1(WO) - Clear SCSI Interrupt 1
 * 0x00c SSTAT1(RO) - SCSI Status 1
 * 0x00d SSTAT2(RO) -             2
 * 0x00e SSTAT3(R0) -             3
 * 0x010 SIMODE0(RW) - SCSI Interrupt Mode 1
 * 0x011 SIMODE1(RW) -                     1
 * 0x012 SCSIBUSL(RW) - SCSI Data Bus (Low)
 * 0x013 SCSIBUSH(RW) -                High
 * 0x014 SHADDR(RO) - SCSI Host Address
 * 0x018 SELTIMER(RW) - Selection Timeout Timer
 * 0x019 SELID(RW) - (re)Selection ID
 * 0x01a SCAMCTL(RW) - ???
 * 0x01b SPIOCAP(RW) - detailed description in src file
 * 	
 * 0x01d BEDCTL(??) - ???
 * 0x01f SBLKCTL(RW) - SCSI Block Control
 * 0x060 SEQCTL(RW) - Sequencer Control
 * 	0x80 PERRORDIS - disable PCI error detection??
 * 	0x40 PAUSEDIS
 * 	0x20 FAILDIS
 * 	0x10 FASTMODE
 * 	0x08 BRKADRINTEN
 * 	0x04 STEP
 * 	0x02 SEQRESET
 * 	0x01 LOADRAM
 * 0x061 SEQRAM(RW) - Sequencer RAM Data
 * 0x062 SEQADDR0(RW) - Sequencer RAM addr
 * 0x063 SEQADDR1(RW) - 
 * 0x064 ACCUM(RW) - accumulator
 * 0x065 SINDEX(RW) - ???
 * 0x066 DINDEX(RW) - ???
 * 0x069 ALLONES(RO) - ???
 * 0x06a ALLZEROS(RO) - ???
 * 0x06a NONE(WO) - ???
 * 0x06b FLAGS(RO) - ???
 * 0x06c SINDIR(RO) - ???
 * 0x06d DINDIR(WO) - ???
 * 0x06e FUNCTION(RW) - ???
 * 0x06f STACK(RO) - ??? (STACK_SIZE=4)
 * 0x084 BCTL(RW) - Board Control
 * 0x084 DSCOMMAND0(RW) - ???
 * 0x085 DSCOMMAND1(RW) - ???
 * 0x086 DSPCISTATUS(??) - ??
 * 0x087 HCNTRL(RW) - Host Control - at least one bit must be 0
 * 	0x40 POWRDN - power down indicator
 * 	0x10 SDINT
 * 	0x08 IRQMS
 * 	0x04 PAUSE - pause chip
 * 	0x02 INTEN 
 * 	0x01 CHIPRST(ACK) - indicates if chip was initialized/reset chip	
 * 0x088 HADDR(RW) - Host address
 * 0x08c HCNT(RW) - ??? Host count ???
 * 0x090 SCBPTR(RW) - SCB Pointer
 * 0x091 INTSTAT(RW) - Interrupt status
 * 0x092 ERROR(RO) - Hard Error
 * 0x092 CLRINT(WO) - Clear interrupt
 * 	0x10 CLRPARERR - clear PCI parity error
 * 	0x08 CLRBRKADRINT
 * 	0x04 CLRSCSIINT
 * 	0x02 CLRCMDINT
 * 	0x01 CLRSEQINT 
 * 0x093 DFCNTRL(RW) - ???
 * 0x094 DFSTATUS(RO) - ???
 * 0x095 DFWADDR(RW) - ???
 * 0x097 DFRADDR(RW) - ???
 * 0x099 DFDAT(RW) - ???
 * 0x09a SCBCNT(RW) - SCB Auto increment
 * 0x09b QINFIFO(RW) - Queue in FIFO
 * 0x09c QINCNT(RO) - Queue in count
 * 0x09d QOUTFIFO(WO) - Queue out FIFO
 * 0x09d CRCCONTROL1(RW) - ???
 * 0x09e SCSIPHASE(RO) - ???
 * 0x09f SFUNCT(RW) - ???
 * 0x0a0 SCB - complex structure refer to the source size=64
 *
 * 0x020 Scratch RAM size=58
 * 0x056 Scratch RAM TODO: Is it really necessary to implement?
 * 0x05a Scratch RAM
 * 0x070 Scratch RAM
 *
 */

static void aha2904_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
	AHA2904State *d = opaque;
	d->bPaused = false;
	// TODO: Handle writing to the registers routine
}

static uint64_t aha2904_read(void *opaque, hwaddr addr, unsigned size)
{
	AHA2904State *d = opaque;
	d->bPaused=false;
	// TODO: Handle reading the registers mentioned above
	return 0ULL;
}

static uint64_t aha2904_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
	//AHA2904State *aha = opaque;
	printf("AHA2904 mmio_read addr=%llx size=%x", (unsigned long long )addr, size);
	return aha2904_read(opaque, addr, size);
}

static void aha2904_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
	AHA2904State *d = opaque;
	d->bPaused=false;
	printf("AHA2904 mmio_write addr=%llx, val=%lx, size=%x", (unsigned long long)addr, val, size);
	aha2904_write(opaque, addr, val, size);
}

static const MemoryRegionOps aha2904_mmio_ops = {
	.read = aha2904_mmio_read,
	.write = aha2904_mmio_write,
	.endianness = DEVICE_LITTLE_ENDIAN, // TODO: What endianness?
	.impl = {
		.min_access_size = 1,
		.max_access_size = 1,
	},
};

static void aha2904_realize(PCIDevice *pdev, Error **errp)
{
	AHA2904State *aha = AHA2904_DEV(pdev);
	uint8_t *pci_conf = pdev->config;
	// TODO: Should we set DMA mask here?
	// If so set it to 0xFFFFFFFF
	pci_config_set_interrupt_pin(pci_conf, 0); // No interrupt pin
	// TODO: There should be some int's so also pins
	memory_region_init_io(&aha->mmio, OBJECT(aha), &aha2904_mmio_ops, aha, "aha2904-mmio", MMIO_SIZE);
	pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &aha->mmio);
}

static void aha2904_reset(AHA2904State *p)
{
}

static void aha2904_exit(PCIDevice *pdev)
{
	AHA2904State *d = AHA2904_DEV(pdev);
	aha2904_reset(d);
}

/* Following two functions are used to determine capabilities of our card
 * and set configuration of it
 * Following registers should be covered (masks are indented):
 * 0x00 PCIR_DEVVENDOR
 * 0x00 PCIR_VENDOR
 * 0x02 PCIR_DEVICE
 * 0x04 PCIR_COMMAND 4bytes
 * 	0x0001 - CMD_PORTEN - access by port
 * 	0x0002 - CMD_MEMEN - memory mapped access
 * 	0x0004 - CMD_BUSMASTEREN
 * 	0x0010 - CMD_MWRICEN
 * 	0x0040 - CMD_PERRESPEN 
 * 	0x0100 - CMD_SERRESPEN - enable/disable error interrpupts
 * 0x06 PCIR_STATUS - masks and meanings in aic7xxx_pci.c
 * 	0x80 DPE - detected parity error detected during address or write data phase 
 * 	0x40 SSE - signal system error
 * 	0x20 RMA - received master abort
 * 	0x10 RTA - received target abort
 *	0x08 STA - signaled a target abort
 * 	0x01 DPR - data parity error reported via PERR  // rewrite to this addr clears PCI errors?
 * 0x08 PCIR_REVID
 * 0x09 PCIR_PROGIF
 * 0x0a PCIR_SUBCLASS
 * 0x0b PCIR_CLASS
 * 0x0c PCIR_CACHELNSZ
 * 0x0d PCIR_LATTIMER
 * 0x0e PCIR_HEADERTYPE
 * 	0x80 PCIR_MFDEV
 * 0x0f PCIR_BIST
 * 0x34 PCIR_CAP_PTR
 *
 * 0x10 PCIR_MAPS
 * 0x10 PCIR_BARS
 * 	consequent bars are 4 bytes long
 * 0x2c PCIR_SUBVEND_0
 * 0x2e PCIR_SUBDEV_0
 *
 * 0x40 DEVCONFIG size=4 bytes
 * 
 *
 */

/*
static uint32_t aha2904_config_read(PCIDevice *pdev, uint32_t addr, int len)
{
	// TODO: Handle changes in registry mentioned above
	return 0;
}

static void aha2904_config_write(PCIDevice *pdev, uint32_t addr, uint32_t data, int len)
{
	//TODO: Handle changes in registry mentioned above
}
*/
static void qdev_aha2904_reset(DeviceState *dev)
{
    AHA2904State *d = AHA2904_DEV(dev);
    aha2904_reset(d);
}

static Property aha2904_properties[] = {
	DEFINE_PROP_SIZE("membar", AHA2904State, membar_size, 0),
	DEFINE_PROP_END_OF_LIST(),
};

static void aha2904_class_init(ObjectClass *class, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(class);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

	k->realize = aha2904_realize;
	k->exit = aha2904_exit;
	k->vendor_id = AHA2904_VENDOR_ID;
	k->device_id = AHA2904_DEVICE_ID; 
	// TODO: Add subvendor and subdevice IDs (both 0x0000)
	//k->config_read = aha2904_config_read;
	//k->config_write = aha2904_config_write;
	// TODO: Check if romfile bar needed
	k->revision = 0x00; 
	k->class_id = PCI_CLASS_OTHERS;
	dc->desc = "AHA2904 SCSI Controller";
	set_bit(DEVICE_CATEGORY_MISC, dc->categories);
	dc->reset = qdev_aha2904_reset;
	device_class_set_props(dc, aha2904_properties);
}
static InterfaceInfo interfaces[] = {
	{INTERFACE_CONVENTIONAL_PCI_DEVICE },
	{ },
};

static const TypeInfo aha2904_info = {
	.name = TYPE_AHA2904,
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


