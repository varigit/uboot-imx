/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:    GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/hab.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>

/* -------- start of HAB API updates ------------*/
#define hab_rvt_report_event_p						\
(									\
	((hab_rvt_report_event_t *)HAB_RVT_REPORT_EVENT)		\
)

#define hab_rvt_report_status_p						\
(									\
	((hab_rvt_report_status_t *)HAB_RVT_REPORT_STATUS)		\
)

#define hab_rvt_authenticate_image_p					\
(									\
	((hab_rvt_authenticate_image_t *)HAB_RVT_AUTHENTICATE_IMAGE)	\
)

#define hab_rvt_entry_p							\
(									\
	((hab_rvt_entry_t *)HAB_RVT_ENTRY)				\
)

#define hab_rvt_exit_p							\
(									\
	((hab_rvt_exit_t *)HAB_RVT_EXIT)				\
)

#define IVT_SIZE		0x20
#define ALIGN_SIZE		0x1000
#define CSF_PAD_SIZE		0x2000

/*
 * +------------+  0x0 (DDR_UIMAGE_START) -
 * |   Header   |                          |
 * +------------+  0x40                    |
 * |            |                          |
 * |            |                          |
 * |            |                          |
 * |            |                          |
 * | Image Data |                          |
 * .            |                          |
 * .            |                           > Stuff to be authenticated ----+
 * .            |                          |                                |
 * |            |                          |                                |
 * |            |                          |                                |
 * +------------+                          |                                |
 * |            |                          |                                |
 * | Fill Data  |                          |                                |
 * |            |                          |                                |
 * +------------+ Align to ALIGN_SIZE      |                                |
 * |    IVT     |                          |                                |
 * +------------+ + IVT_SIZE              -                                 |
 * |            |                                                           |
 * |  CSF DATA  | <---------------------------------------------------------+
 * |            |
 * +------------+
 * |            |
 * | Fill Data  |
 * |            |
 * +------------+ + CSF_PAD_SIZE
 */

bool is_hab_enabled(void)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse =
		(struct fuse_bank1_regs *)bank->fuse_regs;
	uint32_t reg = readl(&fuse->cfg0);

	return (reg & 0x2000000) == 0x2000000;
}

/*
 * Check whether addr lies between start and end and is within
 * the length of the image
 */
static inline int chk_bounds(const uint8_t *addr, size_t bytes,
				const uint8_t *start, const uint8_t *end)
{
	return (addr && (addr >= start) && (addr <= end) &&
		((size_t)((end + 1) - addr) >= bytes))
		? 1 : 0;
}

/* Get Length of each command in CSF */
static inline int get_csf_cmd_hdr_len(const uint8_t *csf_hdr)
{
	if (*csf_hdr == HAB_CMD_HDR)
		return sizeof(struct hab_hdr);

	return HAB_HDR_LEN(*(const struct hab_hdr *)csf_hdr);
}

/*
 * Check if CSF has Write data command
 *
 * If WRITE DATA command exists, then return failure
 */
static int csf_is_valid(int ivt_offset, ulong start_addr, size_t bytes)
{
	size_t offset = 0;
	size_t cmd_hdr_len = 0;
	size_t csf_hdr_len = 0;

	const struct hab_ivt *ivt_initial = NULL;
	const uint8_t *csf_hdr = NULL;
	const uint8_t *end = NULL;
	const uint8_t *start = (const uint8_t *)start_addr;

	ivt_initial = (const struct hab_ivt *)(start + ivt_offset);

	if (bytes != 0)
		end = start + bytes - 1;
	else
		end = start;

	/* Check that the CSF lies within the image bounds */
	if ((start == 0) || (ivt_initial == NULL) ||
	    (ivt_initial->csf == 0) ||
	    !chk_bounds((const uint8_t *)ivt_initial->csf,
			HAB_HDR_LEN(*(const struct hab_hdr *)ivt_initial->csf),
			start, end)) {
		puts("Error - CSF lies outside the image bounds\n");
		return 0;
	}

	csf_hdr = (const uint8_t *)ivt_initial->csf;

	if (*csf_hdr == HAB_CMD_HDR) {
		csf_hdr_len = HAB_HDR_LEN(*(const struct hab_hdr *)csf_hdr);
	} else {
		puts("Error - CSF header command not found\n");
		return 0;
	}

	/* Check for Write data command in CSF */
	do {
		switch (csf_hdr[offset]) {
		case (HAB_CMD_WRT_DAT):
			puts("Error - WRITE Data command found\n");
			return 0;
		default:
			break;
		}

		cmd_hdr_len = get_csf_cmd_hdr_len(&csf_hdr[offset]);
		if (!cmd_hdr_len) {
			puts("Error - Invalid command length\n");
			return 0;
		}
		offset += cmd_hdr_len;

	} while (offset < csf_hdr_len);

	/* Write Data command not found */
	return 1;
}

void display_event(uint8_t *event_data, size_t bytes)
{
	uint32_t i;

	if (!(event_data && bytes > 0))
		return;

	for (i = 0; i < bytes; i++) {
		if (i == 0)
			printf("\t0x%02x", event_data[i]);
		else if ((i % 8) == 0)
			printf("\n\t0x%02x", event_data[i]);
		else
			printf(" 0x%02x", event_data[i]);
	}
}

int get_hab_status(void)
{
	uint32_t index = 0; /* Loop index */
	uint8_t event_data[128]; /* Event data buffer */
	size_t bytes = sizeof(event_data); /* Event size in bytes */
	enum hab_config config = 0;
	enum hab_state state = 0;
	hab_rvt_report_event_t *hab_rvt_report_event;
	hab_rvt_report_status_t *hab_rvt_report_status;

	if (is_hab_enabled())
		puts("\nSecure boot enabled\n");
	else
		puts("\nSecure boot disabled\n");

	hab_rvt_report_event = hab_rvt_report_event_p;
	hab_rvt_report_status = hab_rvt_report_status_p;

	/* Check HAB status */
	if (hab_rvt_report_status(&config, &state) != HAB_SUCCESS) {
		printf("\nHAB Configuration: 0x%02x, HAB State: 0x%02x\n",
		       config, state);

		/* Display HAB Error events */
		while (hab_rvt_report_event(HAB_FAILURE, index, event_data,
					&bytes) == HAB_SUCCESS) {
			puts("\n");
			printf("--------- HAB Event %d -----------------\n",
			       index + 1);
			puts("event data:\n");
			display_event(event_data, bytes);
			puts("\n");
			bytes = sizeof(event_data);
			index++;
		}
	}
	/* Display message if no HAB events are found */
	else {
		printf("\nHAB Configuration: 0x%02x, HAB State: 0x%02x\n",
		       config, state);
		puts("No HAB Events Found!\n\n");
	}
	return 0;
}

#ifdef DEBUG_AUTHENTICATE_IMAGE
void dump_mem(uint32_t addr, int size)
{
	int i;

	for (i = 0; i < size; i += 4) {
		if (i != 0) {
			if (i % 16 == 0)
				printf("\n");
			else
				printf(" ");
		}

		printf("0x%08x", *(uint32_t *)addr);
		addr += 4;
	}

	printf("\n");

	return;
}
#endif

uint32_t authenticate_image(uint32_t ddr_start, uint32_t image_size)
{
	uint32_t load_addr = 0;
	size_t bytes;
	ptrdiff_t ivt_offset = 0;
	int result = 0;
	ulong start;
	hab_rvt_authenticate_image_t *hab_rvt_authenticate_image;
	hab_rvt_entry_t *hab_rvt_entry;
	hab_rvt_exit_t *hab_rvt_exit;

	hab_rvt_authenticate_image = hab_rvt_authenticate_image_p;
	hab_rvt_entry = hab_rvt_entry_p;
	hab_rvt_exit = hab_rvt_exit_p;

	if (is_hab_enabled()) {
		printf("\nAuthenticate uImage from DDR location 0x%x...\n",
			ddr_start);

		hab_caam_clock_enable(1);

		if (hab_rvt_entry() == HAB_SUCCESS) {
			/* If not already aligned, Align to ALIGN_SIZE */
			ivt_offset = (image_size + ALIGN_SIZE - 1) &
					~(ALIGN_SIZE - 1);

			start = ddr_start;
			bytes = ivt_offset + IVT_SIZE + CSF_PAD_SIZE;

			puts("Check CSF for Write Data command before ");
			puts("authenticating image\n");
			if (!csf_is_valid(ivt_offset, start, bytes))
				return result;

#ifdef DEBUG_AUTHENTICATE_IMAGE
			printf("\nivt_offset = 0x%x, ivt addr = 0x%x\n",
			       ivt_offset, ddr_start + ivt_offset);
			printf("Dumping IVT\n");
			dump_mem(ddr_start + ivt_offset, 0x20);

			printf("Dumping CSF Header\n");
			dump_mem(ddr_start + ivt_offset + 0x20, 0x40);

			get_hab_status();

			printf("\nCalling authenticate_image in ROM\n");
			printf("\tivt_offset = 0x%x\n\tstart = 0x%08x"
			       "\n\tbytes = 0x%x\n", ivt_offset, start, bytes);
#endif
			load_addr = (uint32_t)hab_rvt_authenticate_image(
					HAB_CID_UBOOT,
					ivt_offset, (void **)&start,
					(size_t *)&bytes, NULL);
			if (hab_rvt_exit() != HAB_SUCCESS) {
				printf("hab exit function fail\n");
				load_addr = 0;
			}
		} else
			printf("hab entry function fail\n");

		hab_caam_clock_enable(0);

		get_hab_status();
	}

	if ((!is_hab_enabled()) || (load_addr != 0))
		result = 1;

	return result;
}

int do_hab_status(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if ((argc != 1)) {
		cmd_usage(cmdtp);
		return 1;
	}

	get_hab_status();

	return 0;
}

static int do_authenticate_image(cmd_tbl_t *cmdtp, int flag, int argc,
				char * const argv[])
{
	ulong	addr, ivt_offset;
	int	rcode = 0;

	if (argc < 3)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[1], NULL, 16);
	ivt_offset = simple_strtoul(argv[2], NULL, 16);

	rcode = authenticate_image(addr, ivt_offset);

	return rcode;
}

U_BOOT_CMD(
		hab_status, CONFIG_SYS_MAXARGS, 1, do_hab_status,
		"display HAB status",
		""
	  );

U_BOOT_CMD(
		hab_auth_img, 3, 1, do_authenticate_image,
		"authenticate image via HAB",
		"addr ivt_offset\n"
		"addr - image hex address\n"
		"ivt_offset - hex offset of IVT in the image"
	  );
