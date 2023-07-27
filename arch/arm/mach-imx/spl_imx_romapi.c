// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#include <common.h>
#include <errno.h>
#include <image.h>
#include <imx_container.h>
#include <log.h>
#include <asm/global_data.h>
#include <linux/libfdt.h>
#include <spl.h>
#include <asm/arch/sys_proto.h>
#include <asm/cache.h>
#include <malloc.h>

DECLARE_GLOBAL_DATA_PTR;

u32 rom_dev_page_size;

/* Caller need ensure the offset and size to align with page size */
static ulong spl_romapi_raw_seekable_read(u32 offset, u32 size, void *buf)
{
	int ret;

	debug("%s 0x%x, size 0x%x\n", __func__, offset, size);

	ret = rom_api_download_image(buf, offset, size);

	if (ret == ROM_API_OKAY)
		return size;

	printf("%s Failure when load 0x%x, size 0x%x\n", __func__, offset, size);

	return 0;
}

ulong spl_romapi_read(u32 offset, u32 size, void *buf)
{
	u32 off_in_page, aligned_size, readsize;
	int ret;
	u8 *tmp;

	if (!rom_dev_page_size) {
		ret = rom_api_query_boot_infor(QUERY_PAGE_SZ, &rom_dev_page_size);
		if (ret != ROM_API_OKAY) {
			puts("ROMAPI: Failure query boot infor pagesize/offset\n");
			return 0;
		}
	}

	off_in_page = offset % rom_dev_page_size;
	aligned_size = ALIGN(size + off_in_page, rom_dev_page_size);

	if (aligned_size != size) {
		tmp = malloc(aligned_size);
		if (!tmp) {
			printf("%s: Failed to malloc %u bytes\n", __func__, aligned_size);
			return 0;
		}

		readsize = spl_romapi_raw_seekable_read(offset - off_in_page, aligned_size, tmp);
		if (readsize != aligned_size) {
			printf("%s: Failed read %u, actual %u\n", __func__, aligned_size, readsize);
			free(tmp);
			return 0;
		}

		memcpy(buf, tmp + off_in_page, size);
		free(tmp);
		return size;
	}

	return spl_romapi_raw_seekable_read(offset, size, buf);
}

ulong __weak spl_romapi_get_uboot_base(u32 image_offset, u32 rom_bt_dev, u32 pagesize)
{
	u32 offset;

	if (((rom_bt_dev >> 16) & 0xff) ==  BT_DEV_TYPE_FLEXSPINOR)
		offset = CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512;
	else
		offset = image_offset + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512 - 0x8000;

	return offset;
}

static int is_boot_from_stream_device(u32 boot)
{
	u32 interface;

	interface = boot >> 16;
	if (interface >= BT_DEV_TYPE_USB)
		return 1;

	if (interface == BT_DEV_TYPE_MMC && (boot & 1))
		return 1;

	return 0;
}

static ulong spl_romapi_read_seekable(struct spl_load_info *load,
				      ulong offset, ulong byte,
				      void *buf)
{
	/* Handle corner case for ocram 0x980000 to 0x98ffff ecc region, ROM does not allow to access it */
	if (is_imx8mp()) {
		ulong ret;
		void *new_buf;
		if (((ulong)buf >= 0x980000 && (ulong)buf <= 0x98ffff)) {
			new_buf = memalign(ARCH_DMA_MINALIGN, byte);
			if (!new_buf) {
				printf("Fail to allocate read buffer\n");
				return 0;
			}
			ret = spl_romapi_raw_seekable_read(offset, byte, new_buf);
			memcpy(buf, new_buf, ret);
			free(new_buf);
			return ret;
		} else if ((ulong)(buf + byte) >= 0x980000 && (ulong)(buf + byte) <= 0x98ffff) {
			u32 over_size = (ulong)(buf + byte) - 0x97ffff;
			int pagesize = spl_get_bl_len(load);
			over_size = (over_size + pagesize - 1) / pagesize * pagesize;

			ret = spl_romapi_raw_seekable_read(offset, byte - over_size, buf);
			new_buf = memalign(ARCH_DMA_MINALIGN, over_size);
			if (!new_buf) {
				printf("Fail to allocate read buffer\n");
				return 0;
			}

			ret += spl_romapi_raw_seekable_read(offset + byte - over_size, over_size, new_buf);
			memcpy(buf + byte - over_size, new_buf, ret);
			free(new_buf);
			return ret;
		}
	}

	return spl_romapi_raw_seekable_read(offset, byte, buf);
}

static int spl_romapi_load_image_seekable(struct spl_image_info *spl_image,
					  struct spl_boot_device *bootdev,
					  u32 rom_bt_dev)
{
	int ret;
	u32 offset;
	u32 pagesize, size;
	struct legacy_img_hdr *header;
	u32 image_offset;

	ret = rom_api_query_boot_infor(QUERY_IVT_OFF, &offset);
	if (ret != ROM_API_OKAY)
		goto err;

	ret = rom_api_query_boot_infor(QUERY_PAGE_SZ, &pagesize);
	if (ret != ROM_API_OKAY)
		goto err;

	ret = rom_api_query_boot_infor(QUERY_IMG_OFF, &image_offset);
	if (ret != ROM_API_OKAY)
		goto err;

	header = (struct legacy_img_hdr *)(CONFIG_SPL_IMX_ROMAPI_LOADADDR);

	printf("image offset 0x%x, pagesize 0x%x, ivt offset 0x%x\n",
	       image_offset, pagesize, offset);

	offset = spl_romapi_get_uboot_base(image_offset, rom_bt_dev, pagesize);

	size = ALIGN(sizeof(struct legacy_img_hdr), pagesize);
	ret = rom_api_download_image((u8 *)header, offset, size);

	if (ret != ROM_API_OKAY) {
		printf("ROMAPI: download failure offset 0x%x size 0x%x\n",
		       offset, size);
		return -1;
	}

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) && image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		memset(&load, 0, sizeof(load));
		spl_set_bl_len(&load, pagesize);
		load.read = spl_romapi_read_seekable;
		return spl_load_simple_fit(spl_image, &load, offset, header);
	} else if (IS_ENABLED(CONFIG_SPL_LOAD_IMX_CONTAINER) &&
		   valid_container_hdr((void *)header)) {
		struct spl_load_info load;

		memset(&load, 0, sizeof(load));
		spl_set_bl_len(&load, pagesize);
		load.read = spl_romapi_read_seekable;

		ret = spl_load_imx_container(spl_image, &load, offset);
	} else {
		/* TODO */
		puts("Can't support legacy image\n");
		return -1;
	}

	return 0;

err:
	puts("ROMAPI: Failure query boot infor pagesize/offset\n");
	return -1;
}

struct stream_state {
	u8 *base;
	u8 *end;
	u32 pagesize;
};

static ulong spl_romapi_read_stream(struct spl_load_info *load, ulong sector,
			       ulong count, void *buf)
{
	struct stream_state *ss = load->priv;
	u8 *end = (u8*)(sector + count);
	u32 bytes;
	int ret;

	if (end > ss->end) {
		bytes = end - ss->end;
		bytes += ss->pagesize - 1;
		bytes /= ss->pagesize;
		bytes *= ss->pagesize;

		debug("downloading another 0x%x bytes\n", bytes);
		ret = rom_api_download_image(ss->end, 0, bytes);

		if (ret != ROM_API_OKAY) {
			printf("Failure download %d\n", bytes);
			return 0;
		}

		ss->end += bytes;
	}

	memcpy(buf, (void *)(sector), count);
	return count;
}

static ulong spl_ram_load_read(struct spl_load_info *load, ulong sector,
			       ulong count, void *buf)
{
	memcpy(buf, (void *)(sector), count);

	if (load->priv) {
		ulong *p = (ulong *)load->priv;
		ulong total = sector + count;

		if (total > *p)
			*p = total;
	}

	return count;
}

static u8 *search_fit_header(u8 *p, int size)
{
	int i;

	for (i = 0; i < size; i += 4)
		if (genimg_get_format(p + i) == IMAGE_FORMAT_FIT)
			return p + i;

	return NULL;
}

static u8 *search_container_header(u8 *p, int size)
{
	int i = 0;
	u8 *hdr;

	for (i = 0; i < size; i += 4) {
		hdr = p + i;
		if (valid_container_hdr((void *)hdr) &&
		    (*(hdr + 1) != 0 || *(hdr + 2) != 0))
			return p + i;
	}

	return NULL;
}

static u8 *search_img_header(u8 *p, int size)
{
	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT))
		return search_fit_header(p, size);
	else if (IS_ENABLED(CONFIG_SPL_LOAD_IMX_CONTAINER))
		return search_container_header(p, size);

	return NULL;
}

static u32 img_header_size(void)
{
	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT))
		return sizeof(struct fdt_header);
	else if (IS_ENABLED(CONFIG_SPL_LOAD_IMX_CONTAINER))
		return sizeof(struct container_hdr);

	return 0;
}

static int img_info_size(void *img_hdr)
{
#ifdef CONFIG_SPL_LOAD_FIT
	return board_spl_fit_size_align(fit_get_size(img_hdr));
#elif defined CONFIG_SPL_LOAD_IMX_CONTAINER
	struct container_hdr *container = img_hdr;

	return (container->length_lsb + (container->length_msb << 8));
#else
	return 0;
#endif
}

static int img_total_size(void *img_hdr)
{
	if (IS_ENABLED(CONFIG_SPL_LOAD_IMX_CONTAINER)) {
		int total = get_container_size((ulong)img_hdr, NULL);

		if (total < 0) {
			printf("invalid container image\n");
			return 0;
		}

		return total;
	}

	return 0;
}

static int spl_romapi_load_image_stream(struct spl_image_info *spl_image,
					struct spl_boot_device *bootdev)
{
	struct spl_load_info load;
	u32 pagesize, pg;
	int ret;
	int i = 0;
	u8 *p = (u8 *)CONFIG_SPL_IMX_ROMAPI_LOADADDR;
	u8 *phdr = NULL;
	int imagesize;
	int total;

	ret = rom_api_query_boot_infor(QUERY_PAGE_SZ, &pagesize);

	if (ret != ROM_API_OKAY)
		puts("failure at query_boot_info\n");

	pg = pagesize;
	if (pg < 1024)
		pg = 1024;

	for (i = 0; i < 640; i++) {
		ret = rom_api_download_image(p, 0, pg);

		if (ret != ROM_API_OKAY) {
			puts("Stream(USB) download failure\n");
			return -1;
		}

		phdr = search_img_header(p, pg);
		p += pg;

		if (phdr)
			break;
	}

	if (!phdr) {
		puts("Can't found uboot image in 640K range\n");
		return -1;
	}

	if (p - phdr < img_header_size()) {
		ret = rom_api_download_image(p, 0, pg);

		if (ret != ROM_API_OKAY) {
			puts("Stream(USB) download failure\n");
			return -1;
		}

		p += pg;
	}

	imagesize = img_info_size(phdr);
	printf("Find img info 0x%p, size %d\n", phdr, imagesize);

	if (p - phdr < imagesize) {
		imagesize -= p - phdr;
		/*need pagesize hear after ROM fix USB problme*/
		imagesize += pg - 1;
		imagesize /= pg;
		imagesize *= pg;

		printf("Need continue download %d\n", imagesize);

		ret = rom_api_download_image(p, 0, imagesize);

		p += imagesize;

		if (ret != ROM_API_OKAY) {
			printf("Failure download %d\n", imagesize);
			return -1;
		}
	}

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT)) {
		struct stream_state ss;

		ss.base = phdr;
		ss.end = p;
		ss.pagesize = pagesize;

		memset(&load, 0, sizeof(load));
		spl_set_bl_len(&load, 1);
		load.read = spl_romapi_read_stream;
		load.priv = &ss;

		return spl_load_simple_fit(spl_image, &load, (ulong)phdr, phdr);
	}

	total = img_total_size(phdr);
	total += 3;
	total &= ~0x3;

	imagesize = total - (p - phdr);

	imagesize += pagesize - 1;
	imagesize /= pagesize;
	imagesize *= pagesize;

	printf("Download %d, Total size %d\n", imagesize, total);

	ret = rom_api_download_image(p, 0, imagesize);
	if (ret != ROM_API_OKAY)
		printf("ROM download failure %d\n", imagesize);

	memset(&load, 0, sizeof(load));
	spl_set_bl_len(&load, 1);
	load.read = spl_ram_load_read;

	if (IS_ENABLED(CONFIG_SPL_LOAD_IMX_CONTAINER))
		return spl_load_imx_container(spl_image, &load, (ulong)phdr);

	return -1;
}

int board_return_to_bootrom(struct spl_image_info *spl_image,
			    struct spl_boot_device *bootdev)
{
	int ret;
	u32 boot, bstage;

	ret = rom_api_query_boot_infor(QUERY_BT_DEV, &boot);
	if (ret != ROM_API_OKAY)
		goto err;

	ret = rom_api_query_boot_infor(QUERY_BT_STAGE, &bstage);
	if (ret != ROM_API_OKAY)
		goto err;

	printf("Boot Stage: ");

	switch (bstage) {
	case BT_STAGE_PRIMARY:
		printf("Primary boot\n");
		break;
	case BT_STAGE_SECONDARY:
		printf("Secondary boot\n");
		break;
	case BT_STAGE_RECOVERY:
		printf("Recovery boot\n");
		break;
	case BT_STAGE_USB:
		printf("USB boot\n");
		break;
	default:
		printf("Unknown (0x%x)\n", bstage);
	}

	if (is_boot_from_stream_device(boot))
		return spl_romapi_load_image_stream(spl_image, bootdev);

	return spl_romapi_load_image_seekable(spl_image, bootdev, boot);
err:
	puts("ROMAPI: failure at query_boot_info\n");
	return -1;
}
