/*
 * Copyright (C) 2000-2005, DENX Software Engineering
 *		Wolfgang Denk <wd@denx.de>
 * Copyright (C) Procsys. All rights reserved.
 *		Mushtaq Khan <mushtaq_k@procsys.com>
 *			<mushtaqk_921@yahoo.co.in>
 * Copyright (C) 2008 Freescale Semiconductor, Inc.
 *		Dave Liu <daveliu@freescale.com>
 * Copyright (C) 2010 Texas Instruments
 *		Aneesh V <aneesh@ti.com>
 * Copyright (C) 2013 TechNexion Ltd.
 *		Richard Hu <linuxfae@technexion.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef CONFIG_SPL_SATA_SUPPORT

#include <common.h>
#include <spl.h>
#include <fat.h>
#include <ext4fs.h>
#include <version.h>
#include <part.h>
#include <sata.h>

static int sata_curr_device = -1;
block_dev_desc_t sata_dev_desc[CONFIG_SYS_SATA_MAX_DEVICE];

int sata_initialize(void)
{
	int rc;

	memset(&sata_dev_desc[0], 0, sizeof(struct block_dev_desc));
	sata_dev_desc[0].if_type = IF_TYPE_SATA;
	sata_dev_desc[0].dev = 0;
	sata_dev_desc[0].part_type = PART_TYPE_UNKNOWN;
	sata_dev_desc[0].type = DEV_TYPE_HARDDISK;
	sata_dev_desc[0].lba = 0;
	sata_dev_desc[0].blksz = 512;
	sata_dev_desc[0].log2blksz = LOG2(sata_dev_desc[0].blksz);
	sata_dev_desc[0].block_read = sata_read;
	sata_dev_desc[0].block_write = sata_write;

	rc = init_sata(0);
#if defined(CONFIG_SPL_FAT_SUPPORT) || defined(CONFIG_SPL_EXT_SUPPORT)
	if (!rc) {
		rc = scan_sata(0);
		if (!rc && (sata_dev_desc[0].lba > 0) &&
			(sata_dev_desc[0].blksz > 0))
			init_part(&sata_dev_desc[0]);
	}
#endif
	sata_curr_device = 0;
	return rc;
}

DECLARE_GLOBAL_DATA_PTR;

static int sata_load_image_raw(unsigned long sector)
{
	unsigned long err = 0;
	u32 image_size_sectors;
	struct image_header *header;
	ulong n;
	
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
						sizeof(struct image_header));

	n = sata_read(sata_curr_device, sector, 1, header);

	/* flush cache after read */
	flush_cache((unsigned long)header, 1 * sata_dev_desc[sata_curr_device].blksz);

	if (n != 1) {
		err = 1;
		goto end;
	}
	
	spl_parse_image_header(header);

	/* convert size to sectors - round up */
	image_size_sectors = (spl_image.size + sata_dev_desc[sata_curr_device].blksz - 1) /
				sata_dev_desc[sata_curr_device].blksz;

	printf("\nSATA read: device %d block # %ld, count %ld ... ",
	sata_curr_device, sector, image_size_sectors);

	n = sata_read(sata_curr_device, sector, image_size_sectors, (void *)spl_image.load_addr);

	/* flush cache after read */
	flush_cache((void *)spl_image.load_addr, image_size_sectors * sata_dev_desc[sata_curr_device].blksz);

	printf("%ld blocks read: %s\n", n, (n==image_size_sectors) ? "OK" : "ERROR");
	if (n != image_size_sectors) {
		err = 1;
		goto end;
	}

end:
	if (err == 1)
		printf("spl: SATA blk read err - %lu\n", err);

	return err;
}

#ifdef CONFIG_SPL_FAT_SUPPORT
static int sata_load_image_fat(const char *filename)
{
	int err;
	struct image_header *header;

	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
						sizeof(struct image_header));

	err = file_fat_read(filename, header, sizeof(struct image_header));
	if (err <= 0)
		goto end;

	spl_parse_image_header(header);

	err = file_fat_read(filename, (u8 *)spl_image.load_addr, 0);

end:
	if (err <= 0)
		printf("spl: error reading image %s, err - %d\n",
		       filename, err);

	return (err <= 0);
}
#endif

#ifdef CONFIG_SPL_EXT_SUPPORT
static int sata_load_image_ext(const char *filename)
{
	int err;
	struct image_header *header;

	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
						sizeof(struct image_header));
	
	err = ext4_read_file(filename, header, 0, sizeof(struct image_header));
	if (err <= 0)
		goto end;

	spl_parse_image_header(header);

	err = ext4_read_file(filename, (u8 *)spl_image.load_addr, 0, 0);

end:
	if (err <= 0)
		printf("spl: error reading image %s, err - %d\n",
		       filename, err);
	else
		printf("loading %s from SATA EXT...\n", filename);	

	return (err <= 0);
}
#endif

void spl_sata_load_image(void)
{
	int err;
	u32 boot_mode;
	ulong n;

	err = sata_initialize();
	if (err) {
		printf("spl: sata init failed: err - %d\n", err);
		hang();
	}

#ifdef CONFIG_SPL_FAT_SUPPORT
	/* FAT filesystem */
	err = fat_register_device(&sata_dev_desc[sata_curr_device],
					  CONFIG_SYS_MMC_SD_FAT_BOOT_PARTITION);
	if (err) {
		printf("spl: fat register err - %d\n", err);
	}

	err = sata_load_image_fat(CONFIG_SPL_FAT_LOAD_PAYLOAD_NAME);
#endif

#ifdef CONFIG_SPL_EXT_SUPPORT
	/* EXT filesystem */
	if (err) {
		disk_partition_t info;
		if (get_partition_info(&sata_dev_desc[sata_curr_device], CONFIG_SYS_MMC_SD_FAT_BOOT_PARTITION, &info)) {
			printf("Cannot find partition %d\n", CONFIG_SYS_MMC_SD_FAT_BOOT_PARTITION);
		}		
		if (ext4fs_probe(&sata_dev_desc[sata_curr_device], &info)) {
			printf("ext4fs probe failed \n");
     		}		

		err = sata_load_image_ext(CONFIG_SPL_FAT_LOAD_PAYLOAD_NAME);
	}
#endif
	
	if (err) {
		printf("Load image from RAW...\n");
		err = sata_load_image_raw(CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR);
		if (err) {
			printf("spl: wrong SATA boot mode\n");
			hang();
		}
	}
}
#endif
