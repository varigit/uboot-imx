/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * Copyright (C) 2016 Variscite Ltd. All Rights Reserved.
 *
 * Author: Eran Matityahu <eran.m@variscite.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <malloc.h>
#include <asm/arch/mx6-pins.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/video.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <micrel.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch/mxc_hdmi.h>
#include <asm/arch/crm_regs.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include <usb.h>
#include <usb/ehci-fsl.h>

#ifdef CONFIG_FSL_FASTBOOT
#include <fsl_fastboot.h>
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif
#endif /*CONFIG_FSL_FASTBOOT*/

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PUS_47K_UP |			\
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define GPMI_PAD_CTRL0 (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP)
#define GPMI_PAD_CTRL1 (PAD_CTL_DSE_40ohm | PAD_CTL_SPEED_MED | \
			PAD_CTL_SRE_FAST)
#define GPMI_PAD_CTRL2 (GPMI_PAD_CTRL0 | GPMI_PAD_CTRL1)

#define PER_VCC_EN_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |	\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |	\
	PAD_CTL_DSE_40ohm   | PAD_CTL_HYS)

#define I2C_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#define OTG_ID_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)


#define VAR_SOM_BACKLIGHT_EN	IMX_GPIO_NR(4, 30)

bool lvds_enabled=false;

#ifdef CONFIG_SYS_USE_NAND
static int var_load_file_from_nand(u32 addr, char *filename)
{
	extern int ubi_part(char *part_name, const char *vid_header_offset);
	extern int ubifs_init(void);
	extern int uboot_ubifs_mount(char *vol_name);
	extern int ubifs_load(char *filename, u32 addr, u32 size);
	extern void cmd_ubifs_umount(void);

	char *ubi_part_name = "rootfs";
	char *ubifs_vol_name = "ubi0:rootfs";

	if (ubi_part(ubi_part_name, NULL))
		return -1;
	if (ubifs_init())
		return -1;
	if (uboot_ubifs_mount(ubifs_vol_name))
		return -1;

	/* Load the file to memory */
	if (ubifs_load(filename, addr, 0)) {
		printf("Error: splash file not found %s\n", filename);
		cmd_ubifs_umount();
		return -1;
	}
	cmd_ubifs_umount();
	return 0;
}
#endif

static int var_load_file_from_mmc(u32 addr, char *filename)
{
#define FS_TYPE_EXT 2
	extern int fs_set_blk_dev(const char *ifname, const char *dev_part_str, int fstype);
	extern int fs_read(const char *filename, ulong addr, loff_t offset, loff_t len,
			loff_t *actread);

	loff_t len_read;

#ifdef CONFIG_SYS_USE_NAND
	if (fs_set_blk_dev("mmc", "1:1", FS_TYPE_EXT))
		return -1;
#else
	if (fs_set_blk_dev("mmc", "0:2", FS_TYPE_EXT))
		return -1;
#endif

	if (fs_read(filename, addr, 0, 0, &len_read)) {
		printf("Error: splash file not found %s\n", filename);
		return -1;
	}

	printf("%llu bytes read\n", len_read);
	return 0;
#undef FS_TYPE_EXT
}

#ifdef CONFIG_SPLASH_SCREEN
int splash_screen_prepare(void)
{
	char *filename;
	const char *addr_str;
	u32 addr = 0;
#ifdef CONFIG_SYS_USE_NAND
	char *s;
#endif

	/* Get filename and load address from env */
	addr_str = getenv("splashimage");
	if (!addr_str) {
		return -1;
	}
	addr = simple_strtoul(addr_str, 0, 16);
	if (addr == 0) {
		printf("Error: bad splashimage value %s\n", addr_str);
		return -1;
	}
	filename = getenv("splash_filename");
	if (!filename) {
		printf("Error: splashimage defined, but splash_filename isn't\n");
		return -1;
	}

#ifdef CONFIG_SYS_USE_NAND
	s = getenv("chosen_rootfs");
	if ((s != NULL) && (!strcmp(s, "emmc"))) {
		if (var_load_file_from_mmc(addr, filename))
			return -1;
	} else {
		if (var_load_file_from_nand(addr, filename))
			return -1;
	}
#else /* MMC */
	if (var_load_file_from_mmc(addr, filename))
		return -1;
#endif

	/* Turn on backlight */
	if (lvds_enabled)
		gpio_set_value(VAR_SOM_BACKLIGHT_EN, 1);

	return 0;
}
#endif

static bool is_som_solo(void)
{
	bool ret;
	int oldbus = i2c_get_bus_num();

	i2c_set_bus_num(PMIC_I2C_BUS);
	/* Probing for PMIC which is not preset only on som solo */
	ret = (0 != i2c_probe(CONFIG_POWER_PFUZE100_I2C_ADDR));

	i2c_set_bus_num(oldbus);
	return ret;
}

static bool is_solo_custom_board(void)
{
	bool ret;
	int oldbus = i2c_get_bus_num();

	i2c_set_bus_num(1);
	/* Probing for extra EEPROM present only on solo custom board */
	ret = (0 == i2c_probe(0x51));

	i2c_set_bus_num(oldbus);
	return ret;
}

static bool is_cpu_pop_package(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	u32 reg;
	u32 type;

	reg = readl(&anatop->digprog);
	type = ((reg >> 16) & 0xff);
	if (type != MXC_CPU_MX6DL)
		return ((soc_sbmr & 0x200000) != 0);
	return false;
}

static inline bool is_dart_board(void)
{
	return is_cpu_pop_package();
}

static inline bool is_mx6_custom_board(void)
{
	return (!is_dart_board() && !is_solo_custom_board());
}

enum mmc_boot_device {
	SD_BOOT,
	MMC_BOOT,
	OTHER_BOOT,
};

static unsigned get_mmc_boot_device(void)
{
	struct src *psrc = (struct src *)SRC_BASE_ADDR;
	unsigned reg = (readl(&psrc->sbmr1) >> 5) & 0x7;

	switch(reg) {
	case 0x2:
		return SD_BOOT;
	case 0x3:
		return MMC_BOOT;
	default:
		return OTHER_BOOT;
	}
}

static bool is_mmc_present(struct mmc *mmc)
{
	int err;
	struct mmc_cmd cmd;

	if (mmc->has_init)
		return true;

	mdelay(1);
	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;

	err = mmc->cfg->ops->send_cmd(mmc, &cmd, NULL);
	if (err)
		return false;

	mdelay(2);

	cmd.cmdidx = MMC_CMD_SEND_OP_COND;
	cmd.resp_type = MMC_RSP_R3;
	cmd.cmdarg = 0;

	err = mmc->cfg->ops->send_cmd(mmc, &cmd, NULL);
	return (!err);
}

static void print_emmc_size(void)
{
	struct mmc *mmc;
	int device;

	if (is_dart_board() && (MMC_BOOT == get_mmc_boot_device()))
		device=0;
	else
		device=1;

	mmc = find_mmc_device(device);
	if (!mmc || !is_mmc_present(mmc) || mmc_init(mmc) || IS_SD(mmc)) {
		puts("No eMMC\n");
		return;
	}
	puts("eMMC:  ");
	print_size(mmc->capacity, "\n");
}

static u32 var_ram_size(void)
{
	u32 *p_ram_size = (u32 *)RAM_SIZE_ADDR;
	return *p_ram_size;
}

int dram_init(void)
{
	gd->ram_size = var_ram_size() * 1024 * 1024;
	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	IOMUX_PADS(PAD_CSI0_DAT10__UART1_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL)),
	IOMUX_PADS(PAD_CSI0_DAT11__UART1_RX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL)),
};

static iomux_v3_cfg_t const enet_pads1[] = {
	IOMUX_PADS(PAD_ENET_MDIO__ENET_MDIO	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_ENET_MDC__ENET_MDC	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TXC__RGMII_TXC	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD0__RGMII_TD0	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD1__RGMII_TD1	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD2__RGMII_TD2	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD3__RGMII_TD3	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TX_CTL__RGMII_TX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_ENET_REF_CLK__ENET_TX_CLK	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	/* pin 35 - 1 (PHY_AD2) on reset */
	IOMUX_PADS(PAD_RGMII_RXC__GPIO6_IO30		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* pin 32 - 1 - (MODE0) all */
	IOMUX_PADS(PAD_RGMII_RD0__GPIO6_IO25		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* pin 31 - 1 - (MODE1) all */
	IOMUX_PADS(PAD_RGMII_RD1__GPIO6_IO27		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* pin 28 - 1 - (MODE2) all */
	IOMUX_PADS(PAD_RGMII_RD2__GPIO6_IO28		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* pin 27 - 1 - (MODE3) all */
	IOMUX_PADS(PAD_RGMII_RD3__GPIO6_IO29		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* pin 33 - 1 - (CLK125_EN) 125Mhz clockout enabled */
	IOMUX_PADS(PAD_RGMII_RX_CTL__GPIO6_IO24	| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* AR8031 PHY Reset */
	IOMUX_PADS(PAD_ENET_CRS_DV__GPIO1_IO25		| MUX_PAD_CTRL(NO_PAD_CTRL)),
};

static iomux_v3_cfg_t const enet_pads2[] = {
	IOMUX_PADS(PAD_RGMII_RXC__RGMII_RXC	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD0__RGMII_RD0	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD1__RGMII_RD1	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD2__RGMII_RD2	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD3__RGMII_RD3	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RX_CTL__RGMII_RX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
};

static void setup_iomux_enet(void)
{
	gpio_direction_output(IMX_GPIO_NR(1, 25), 0);   /* Variscite SOM PHY reset */
	gpio_direction_output(IMX_GPIO_NR(6, 30), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 25), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 27), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 28), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 29), 1);

	SETUP_IOMUX_PADS(enet_pads1);

	gpio_direction_output(IMX_GPIO_NR(6, 24), 1);

	mdelay(10);
	gpio_set_value(IMX_GPIO_NR(1, 25), 1);

	SETUP_IOMUX_PADS(enet_pads2);
}

static iomux_v3_cfg_t const usdhc1_pads[] = {
	IOMUX_PADS(PAD_SD1_CLK__SD1_CLK	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_CMD__SD1_CMD	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT0__SD1_DATA0	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT1__SD1_DATA1	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT2__SD1_DATA2	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT3__SD1_DATA3	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
};


static iomux_v3_cfg_t const usdhc2_pads[] = {
	IOMUX_PADS(PAD_SD2_CLK__SD2_CLK	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_CMD__SD2_CMD	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT0__SD2_DATA0	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT1__SD2_DATA1	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT2__SD2_DATA2	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT3__SD2_DATA3	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
};

static iomux_v3_cfg_t const usdhc3_pads[] = {
	IOMUX_PADS(PAD_SD3_CLK__SD3_CLK   | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_CMD__SD3_CMD   | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT0__SD3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT1__SD3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT2__SD3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT3__SD3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
};

#ifdef CONFIG_SYS_I2C_MXC
I2C_PADS(i2c_pad_info1,
	PAD_CSI0_DAT9__I2C1_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_CSI0_DAT9__GPIO5_IO27 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(5, 27),
	PAD_CSI0_DAT8__I2C1_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_CSI0_DAT8__GPIO5_IO26 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(5, 26));

I2C_PADS(i2c_pad_info2,
	PAD_KEY_COL3__I2C2_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_KEY_COL3__GPIO4_IO12 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(4, 12),
	PAD_KEY_ROW3__I2C2_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_KEY_ROW3__GPIO4_IO13 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(4, 13));

I2C_PADS(i2c_pad_info3,
	PAD_GPIO_5__I2C3_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_GPIO_5__GPIO1_IO05 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(1, 5),
	PAD_GPIO_16__I2C3_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_GPIO_16__GPIO7_IO11 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(7, 11));
#endif

void setup_local_i2c(void) {
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info1));
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info2));
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info3));
}

static void setup_iomux_uart(void)
{
	SETUP_IOMUX_PADS(uart1_pads);
}

#ifdef CONFIG_FSL_ESDHC
struct fsl_esdhc_cfg usdhc_cfg[2] = {
	/*
	 * This is incorrect for DART board but it's overwritten
	 * in board_mmc_init() according to board setup
	 */
	{USDHC2_BASE_ADDR},
	{USDHC1_BASE_ADDR},
};

int board_mmc_getcd(struct mmc *mmc)
{
	return 1;
}

int board_mmc_init(bd_t *bis)
{
#ifndef CONFIG_SPL_BUILD
	int ret;
	int i;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * On non DART boards:
	 * mmc0                    SD2 (SD)
	 * mmc1                    SD1 (eMMC)
	 *
	 * On DART board when booting from SD:
	 * mmc0                    SD2 (SD)
	 * mmc1                    SD3 (eMMC)
	 *
	 * On DART board when booting from eMMC:
	 * mmc0                    SD3 (eMMC)
	 * mmc1                    SD2 (SD)
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			if (is_dart_board() && (MMC_BOOT == get_mmc_boot_device())) {
				SETUP_IOMUX_PADS(usdhc3_pads);
				usdhc_cfg[0].esdhc_base = USDHC3_BASE_ADDR;
				usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			} else {
				SETUP_IOMUX_PADS(usdhc2_pads);
				usdhc_cfg[0].esdhc_base = USDHC2_BASE_ADDR;
				usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			}
			gd->arch.sdhc_clk = usdhc_cfg[0].sdhc_clk;
			usdhc_cfg[0].max_bus_width = 4;
			break;
		case 1:
			if (is_dart_board() && (MMC_BOOT == get_mmc_boot_device())) {
				SETUP_IOMUX_PADS(usdhc2_pads);
				usdhc_cfg[1].esdhc_base = USDHC2_BASE_ADDR;
				usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			} else if (is_dart_board()) {
				SETUP_IOMUX_PADS(usdhc3_pads);
				usdhc_cfg[1].esdhc_base = USDHC3_BASE_ADDR;
				usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			} else {
				SETUP_IOMUX_PADS(usdhc1_pads);
				usdhc_cfg[1].esdhc_base = USDHC1_BASE_ADDR;
				usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			}
			gd->arch.sdhc_clk = usdhc_cfg[1].sdhc_clk;
			usdhc_cfg[1].max_bus_width = 4;
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
			       "(%d) then supported by the board (%d)\n",
			       i + 1, CONFIG_SYS_FSL_USDHC_NUM);
			return -EINVAL;
		}

		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret)
			return ret;
	}
	return 0;
#else
	struct src *psrc = (struct src *)SRC_BASE_ADDR;
	unsigned reg = readl(&psrc->sbmr1) >> 11;
	/*
	 * Upon reading BOOT_CFG register the following map is done:
	 * Bit 11 and 12 of BOOT_CFG register can determine the current
	 * mmc port
	 * 0x1                  SD2 (SD)
	 * 0x2                  SD3 (DART eMMC)
	 */

	switch (reg & 0x3) {
	case 0x1:
		SETUP_IOMUX_PADS(usdhc2_pads);
		usdhc_cfg[0].esdhc_base = USDHC2_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
		gd->arch.sdhc_clk = usdhc_cfg[0].sdhc_clk;
		usdhc_cfg[0].max_bus_width = 4;
		break;
	case 0x2:
		SETUP_IOMUX_PADS(usdhc3_pads);
		usdhc_cfg[0].esdhc_base = USDHC3_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
		gd->arch.sdhc_clk = usdhc_cfg[0].sdhc_clk;
		usdhc_cfg[0].max_bus_width = 4;
		break;
	}

	return fsl_esdhc_initialize(bis, &usdhc_cfg[0]);
#endif
}
#endif

#ifdef CONFIG_SYS_USE_NAND
static iomux_v3_cfg_t const gpmi_pads[] = {
	IOMUX_PADS(PAD_NANDF_CLE__NAND_CLE	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_ALE__NAND_ALE	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_WP_B__NAND_WP_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_RB0__NAND_READY_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL0)),
	IOMUX_PADS(PAD_NANDF_CS0__NAND_CE0_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_SD4_CMD__NAND_RE_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_SD4_CLK__NAND_WE_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D0__NAND_DATA00	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D1__NAND_DATA01	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D2__NAND_DATA02	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D3__NAND_DATA03	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D4__NAND_DATA04	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D5__NAND_DATA05	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D6__NAND_DATA06	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D7__NAND_DATA07	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_SD4_DAT0__NAND_DQS	| MUX_PAD_CTRL(GPMI_PAD_CTRL1)),
};

static void setup_gpmi_nand(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	SETUP_IOMUX_PADS(gpmi_pads);

	/* gate ENFC_CLK_ROOT clock first,before clk source switch */
	clrbits_le32(&mxc_ccm->CCGR2, MXC_CCM_CCGR2_IOMUX_IPT_CLK_IO_MASK);

	/* config gpmi and bch clock to 100 MHz */
	clrsetbits_le32(&mxc_ccm->cs2cdr,
			MXC_CCM_CS2CDR_ENFC_CLK_PODF_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL_MASK,
			MXC_CCM_CS2CDR_ENFC_CLK_PODF(0) |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED(3) |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL(3));

	/* enable ENFC_CLK_ROOT clock */
	setbits_le32(&mxc_ccm->CCGR2, MXC_CCM_CCGR2_IOMUX_IPT_CLK_IO_MASK);

	/* enable gpmi and bch clock gating */
	setbits_le32(&mxc_ccm->CCGR4,
			MXC_CCM_CCGR4_RAWNAND_U_BCH_INPUT_APB_MASK |
			MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_BCH_MASK |
			MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK |
			MXC_CCM_CCGR4_RAWNAND_U_GPMI_INPUT_APB_MASK |
			MXC_CCM_CCGR4_PL301_MX6QPER1_BCH_OFFSET);

	/* enable apbh clock gating */
	setbits_le32(&mxc_ccm->CCGR0, MXC_CCM_CCGR0_APBHDMA_MASK);
}
#endif

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x9, 0x1c00);

	/* control data pad skew */
	ksz9031_phy_extended_write(phydev, 0x02,
			MII_KSZ9031_EXT_RGMII_CTRL_SIG_SKEW,
			MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* rx data pad skew */
	ksz9031_phy_extended_write(phydev, 0x02,
			MII_KSZ9031_EXT_RGMII_RX_DATA_SKEW,
			MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* tx data pad skew */
	ksz9031_phy_extended_write(phydev, 0x02,
			MII_KSZ9031_EXT_RGMII_TX_DATA_SKEW,
			MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* gtx and rx clock pad skew */
	ksz9031_phy_extended_write(phydev, 0x02,
			MII_KSZ9031_EXT_RGMII_CLOCK_SKEW,
			MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x03FF);

	return 0;
}

#if defined(CONFIG_VIDEO_IPUV3)
static void disable_lvds(struct display_info_t const *dev)
{
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;

	int reg = readl(&iomux->gpr[2]);

	reg &= ~(IOMUXC_GPR2_LVDS_CH0_MODE_MASK |
		 IOMUXC_GPR2_LVDS_CH1_MODE_MASK);

	writel(reg, &iomux->gpr[2]);
}

static void do_enable_hdmi(struct display_info_t const *dev)
{
	disable_lvds(dev);
	/*
	 * imx_enable_hdmi_phy(); should be called here.
	 * It is ommitted to avoid a current known bug where
	 * the boot sometimes hangs if an HDMI cable is attached
	 * - at least on some DART SOMs.
	 */
}

static void enable_lvds(struct display_info_t const *dev)
{
	struct iomuxc *iomux = (struct iomuxc *)
				IOMUXC_BASE_ADDR;
	u32 reg = readl(&iomux->gpr[2]);
	reg |= IOMUXC_GPR2_DATA_WIDTH_CH0_24BIT |
	       IOMUXC_GPR2_DATA_WIDTH_CH1_18BIT;
	writel(reg, &iomux->gpr[2]);

	lvds_enabled=true;
}

static void lvds_enable_disable(struct display_info_t const *dev)
{
	if (getenv("splashimage") != NULL)
		enable_lvds(dev);
	else
		disable_lvds(dev);
}

static int detect_dart_vsc_display(struct display_info_t const *dev)
{
	return (!is_mx6_custom_board());
}

static int detect_mx6cb_cdisplay(struct display_info_t const *dev)
{
	if (!is_mx6_custom_board())
		return 0;

	i2c_set_bus_num(dev->bus);
	return (0 == i2c_probe(dev->addr));
}

static int detect_mx6cb_rdisplay(struct display_info_t const *dev)
{
	if (!is_mx6_custom_board())
		return 0;

	/* i2c probe the *c*display */
	i2c_set_bus_num(MX6CB_CDISPLAY_I2C_BUS);
	return (0 != i2c_probe(MX6CB_CDISPLAY_I2C_ADDR));
}

#define MHZ2PS(f)	(1000000/(f))

struct display_info_t const displays[] = {{
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= detect_hdmi,
	.enable	= do_enable_hdmi,
	.mode	= {
		.name           = "HDMI",
		.refresh        = 60,
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 31777,
		.left_margin    = 48,
		.right_margin   = 16,
		.upper_margin   = 33,
		.lower_margin   = 10,
		.hsync_len      = 96,
		.vsync_len      = 2,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB666,
	.detect	= detect_dart_vsc_display,
	.enable	= lvds_enable_disable,
	.mode	= {
		.name           = "VAR-WVGA",
		.refresh        = 60, /* optional */
		.xres           = 800,
		.yres           = 480,
		.pixclock       = MHZ2PS(50),
		.left_margin    = 40,
		.right_margin   = 40,
		.upper_margin   = 29,
		.lower_margin   = 13,
		.hsync_len      = 48,
		.vsync_len      = 3,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= MX6CB_CDISPLAY_I2C_BUS,
	.addr	= MX6CB_CDISPLAY_I2C_ADDR,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= detect_mx6cb_cdisplay,
	.enable	= lvds_enable_disable,
	.mode	= {
		.name           = "VAR-WVGA MX6CB-C",
		.refresh        = 60, /* optional */
		.xres           = 800,
		.yres           = 480,
		.pixclock       = MHZ2PS(50),
		.left_margin    = 39,
		.right_margin   = 39,
		.upper_margin   = 29,
		.lower_margin   = 13,
		.hsync_len      = 128,
		.vsync_len      = 2,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= detect_mx6cb_rdisplay,
	.enable	= lvds_enable_disable,
	.mode	= {
		.name           = "VAR-WVGA MX6CB-R",
		.refresh        = 60, /* optional */
		.xres           = 800,
		.yres           = 480,
		.pixclock       = MHZ2PS(50),
		.left_margin    = 0,
		.right_margin   = 40,
		.upper_margin   = 20,
		.lower_margin   = 13,
		.hsync_len      = 48,
		.vsync_len      = 3,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} } };

size_t display_count = ARRAY_SIZE(displays);

static void setup_display(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int reg;

	/* Setup backlight */
	SETUP_IOMUX_PAD(PAD_DISP0_DAT9__GPIO4_IO30 | MUX_PAD_CTRL(NO_PAD_CTRL));

	/* Turn off backlight until display is ready */
	gpio_direction_output(VAR_SOM_BACKLIGHT_EN , 0);

	enable_ipu_clock();
	imx_setup_hdmi();

	/* Turn on LDB0, LDB1, IPU,IPU DI0 clocks */
	reg = readl(&mxc_ccm->CCGR3);
	reg |=  MXC_CCM_CCGR3_LDB_DI0_MASK | MXC_CCM_CCGR3_LDB_DI1_MASK;
	writel(reg, &mxc_ccm->CCGR3);

	/* set LDB0, LDB1 clk select to 011/011 */
	reg = readl(&mxc_ccm->cs2cdr);
	reg &= ~(MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_MASK
		 | MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_MASK);
	/* 1 -> ~50MHz , 2 -> ~56MHz, 3 -> ~75MHz, 4 -> ~68MHz */
	reg |= (1 << MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_OFFSET)
	      | (1 << MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->cs2cdr);

	reg = readl(&mxc_ccm->cscmr2);
	reg |= MXC_CCM_CSCMR2_LDB_DI0_IPU_DIV | MXC_CCM_CSCMR2_LDB_DI1_IPU_DIV;
	writel(reg, &mxc_ccm->cscmr2);

	reg = readl(&mxc_ccm->chsccdr);
	reg |= (CHSCCDR_CLK_SEL_LDB_DI0
		<< MXC_CCM_CHSCCDR_IPU1_DI0_CLK_SEL_OFFSET);
	reg |= (CHSCCDR_CLK_SEL_LDB_DI0
		<< MXC_CCM_CHSCCDR_IPU1_DI1_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->chsccdr);

	reg = IOMUXC_GPR2_BGREF_RRMODE_EXTERNAL_RES
	     | IOMUXC_GPR2_DI1_VS_POLARITY_ACTIVE_LOW
	     | IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_LOW
	     | IOMUXC_GPR2_BIT_MAPPING_CH1_SPWG
	     | IOMUXC_GPR2_DATA_WIDTH_CH1_18BIT
	     | IOMUXC_GPR2_BIT_MAPPING_CH0_SPWG
	     | IOMUXC_GPR2_DATA_WIDTH_CH0_24BIT
	     | IOMUXC_GPR2_LVDS_CH0_MODE_ENABLED_DI0
	     | IOMUXC_GPR2_LVDS_CH1_MODE_ENABLED_DI0;
	writel(reg, &iomux->gpr[2]);

	reg = readl(&iomux->gpr[3]);
	reg = (reg & ~(IOMUXC_GPR3_LVDS1_MUX_CTL_MASK
			| IOMUXC_GPR3_HDMI_MUX_CTL_MASK))
	    | (IOMUXC_GPR3_MUX_SRC_IPU1_DI0
	       << IOMUXC_GPR3_LVDS1_MUX_CTL_OFFSET);
	writel(reg, &iomux->gpr[3]);
}
#endif /* CONFIG_VIDEO_IPUV3 */

/*
 * Do not overwrite the console
 * Use always serial for U-Boot console
 */
int overwrite_console(void)
{
	return 1;
}

int board_eth_init(bd_t *bis)
{
	uint32_t base = IMX_FEC_BASE;
	struct mii_dev *bus = NULL;
	struct phy_device *phydev = NULL;
	int ret;

	setup_iomux_enet();

#ifdef CONFIG_FEC_MXC
	bus = fec_get_miibus(base, -1);
	if (!bus) {
		printf("FEC MXC bus: %s:failed\n", __func__);
		return 0;
	}

	/* scan phy 3,4,5,6,7 */
	phydev = phy_find_by_mask(bus, (0x1f << 3), PHY_INTERFACE_MODE_RGMII);
	if (!phydev) {
		printf("FEC MXC phy find: %s:failed\n", __func__);
		free(bus);
		return 0;
	}

	printf("using phy at %d\n", phydev->addr);
	ret  = fec_probe(bis, -1, base, bus, phydev);
	if (ret) {
		printf("FEC MXC probe: %s:failed\n", __func__);
		free(phydev);
		free(bus);
	}
#endif

#if defined(CONFIG_CI_UDC) && defined(CONFIG_USB_ETHER)
	/* USB Ethernet Gadget */
	usb_eth_initialize(bis);
#endif

	return 0;
}

#ifdef CONFIG_USB_EHCI_MX6
#define VSC_USB_H1_PWR_EN	IMX_GPIO_NR(4, 15)
#define VSC_USB_OTG_PWR_EN	IMX_GPIO_NR(3, 22)
#define DART_USB_H1_PWR_EN	IMX_GPIO_NR(1, 28)
#define DART_USB_OTG_PWR_EN	IMX_GPIO_NR(4, 15)

static void setup_usb(void)
{
	if (is_dart_board()) {
		/* config OTG ID iomux */
		SETUP_IOMUX_PAD(PAD_ENET_RX_ER__USB_OTG_ID | MUX_PAD_CTRL(OTG_ID_PAD_CTRL));
		imx_iomux_set_gpr_register(1, 13, 1, 0);

		SETUP_IOMUX_PAD(PAD_KEY_ROW4__GPIO4_IO15 | MUX_PAD_CTRL(NO_PAD_CTRL));
		gpio_direction_output(DART_USB_OTG_PWR_EN, 0);

		SETUP_IOMUX_PAD(PAD_ENET_TX_EN__GPIO1_IO28  | MUX_PAD_CTRL(NO_PAD_CTRL));
		gpio_direction_output(DART_USB_H1_PWR_EN, 0);
	} else if (is_solo_custom_board()) {
		/* config OTG ID iomux */
		SETUP_IOMUX_PAD(PAD_GPIO_1__USB_OTG_ID | MUX_PAD_CTRL(OTG_ID_PAD_CTRL));
		imx_iomux_set_gpr_register(1, 13, 1, 1);

		SETUP_IOMUX_PAD(PAD_EIM_D22__GPIO3_IO22  | MUX_PAD_CTRL(NO_PAD_CTRL));
		gpio_direction_output(VSC_USB_OTG_PWR_EN, 0);

		SETUP_IOMUX_PAD(PAD_KEY_ROW4__GPIO4_IO15 | MUX_PAD_CTRL(NO_PAD_CTRL));
		gpio_direction_output(VSC_USB_H1_PWR_EN, 0);
	}
}

int board_usb_phy_mode(int port)
{
	char *s;
	if (is_mx6_custom_board() && port == 0) {
		s = getenv("usbmode");
		if ((s != NULL) && (!strcmp(s, "host")))
			return USB_INIT_HOST;
		else
			return USB_INIT_DEVICE;
	}
	return usb_phy_mode(port);
}

int board_ehci_power(int port, int on)
{
	if (is_mx6_custom_board())
		return 0; /* no power enable needed */

	if (port > 1)
		return -EINVAL;

	if (port) {
		if (is_dart_board())
			gpio_set_value(DART_USB_H1_PWR_EN, on);
		else if (is_solo_custom_board())
			gpio_set_value(VSC_USB_H1_PWR_EN, on);
	} else {
		if (is_dart_board())
			gpio_set_value(DART_USB_OTG_PWR_EN, on);
		else if (is_solo_custom_board())
			gpio_set_value(VSC_USB_OTG_PWR_EN, on);
	}

	return 0;
}
#endif /* CONFIG_USB_EHCI_MX6 */

int board_early_init_f(void)
{
	setup_iomux_uart();
#if defined(CONFIG_VIDEO_IPUV3)
	setup_display();
#endif
#ifdef CONFIG_SYS_I2C_MXC
	setup_local_i2c();
#endif
#ifdef CONFIG_SYS_USE_NAND
	setup_gpmi_nand();
#endif

	return 0;
}

int board_init(void)
{
	int ret = 0;

	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
	gd->bd->bi_arch_number = CONFIG_MACH_VAR_SOM_MX6;

#ifdef CONFIG_USB_EHCI_MX6
	setup_usb();
#endif
	return ret;
}

static struct pmic *pfuze;

#ifdef CONFIG_LDO_BYPASS_CHECK
void ldo_mode_set(int ldo_bypass)
{
	if (!is_som_solo()) {
		unsigned int reg;
		struct pmic *p = pfuze;
		int retval = 0;

		if (!p) {
			printf("No PMIC found!\n");
			return;
		}

		/* Set SW1AB to 1.325V */
		retval += pmic_reg_read(p, PFUZE100_SW1ABVOL, &reg);
		reg &= ~SW1x_NORMAL_MASK;
		reg |= SW1x_1_325V;
		retval += pmic_reg_write(p, PFUZE100_SW1ABVOL, reg);

		/* Set SW1C to 1.325V */
		retval += pmic_reg_read(p, PFUZE100_SW1CVOL, &reg);
		reg &= ~SW1x_NORMAL_MASK;
		reg |= SW1x_1_325V;
		retval += pmic_reg_write(p, PFUZE100_SW1CVOL, reg);

		if (retval) {
			printf("PMIC write voltages error!\n");
			return;
		}

		set_anatop_bypass(0);
		printf("switch to ldo_bypass mode!\n");
	}
}
#endif

int power_init_board(void)
{
	if (!is_som_solo()) {
		unsigned int reg;
		int retval;

		retval = power_pfuze100_init(PMIC_I2C_BUS);
		if (retval)
			return -ENODEV;
		pfuze = pmic_get("PFUZE100");
		retval = pmic_probe(pfuze);
		if (retval)
			return -ENODEV;
		pmic_reg_read(pfuze, PFUZE100_DEVICEID, &reg);
		printf("PMIC:  PFUZE100 ID=0x%02x\n", reg);

		if (!is_dart_board()) {

			/* Set Gigbit Ethernet voltage (SOM v1.1/1.0) */
			retval += pmic_reg_read(pfuze, PFUZE100_SW4VOL, &reg);
			reg &= ~0x7f;
			reg |= 0x60; /* 2.4V */
			retval += pmic_reg_write(pfuze, PFUZE100_SW4VOL, reg);

			/* Increase VGEN5 from 2.8 to 3V */
			retval += pmic_reg_read(pfuze, PFUZE100_VGEN5VOL, &reg);
			reg &= ~LDO_VOL_MASK;
			reg |= LDOB_3_00V;
			retval += pmic_reg_write(pfuze, PFUZE100_VGEN5VOL, reg);

			/* Set SW1AB standby volage to 0.975V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW1ABSTBY, &reg);
			reg &= ~SW1x_STBY_MASK;
			reg |= SW1x_0_975V;
			retval += pmic_reg_write(pfuze, PFUZE100_SW1ABSTBY, reg);

			/* Set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
			retval += pmic_reg_read(pfuze, PFUZE100_SW1ABCONF, &reg);
			reg &= ~SW1xCONF_DVSSPEED_MASK;
			reg |= SW1xCONF_DVSSPEED_4US;
			retval += pmic_reg_write(pfuze, PFUZE100_SW1ABCONF, reg);

			/* Set SW1C standby voltage to 0.975V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW1CSTBY, &reg);
			reg &= ~SW1x_STBY_MASK;
			reg |= SW1x_0_975V;
			retval += pmic_reg_write(pfuze, PFUZE100_SW1CSTBY, reg);

#ifdef LOW_POWER_MODE_ENABLE
			/* Set low power mode voltages to disable */

			/* Set SW3A standby voltage to 1.275V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW3ASTBY, &reg);
			reg &= ~0x7f; /* SW3x STBY MASK */
			reg = 0x23; /* SW3x 1.275V */
			retval += pmic_reg_write(pfuze, PFUZE100_SW3ASTBY, reg);

			/* Set SW3A off voltage to 1.275V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW3AOFF, &reg);
			reg &= ~0x7f; /* SW3x OFF MASK */
			reg = 0x23; /* SW3x 1.275V */
			retval += pmic_reg_write(pfuze, PFUZE100_SW3AOFF, reg);

			/* SW4MODE = OFF in standby */
			reg = PWM_OFF;
			retval += pmic_reg_write(pfuze, PFUZE100_SW4MODE, reg);

			/* Set VGEN3 to 2.5V, VGEN3CTL = OFF in standby */
			retval += pmic_reg_read(pfuze, PFUZE100_VGEN3VOL, &reg);
			reg &= ~0x7f;
			reg |= 0x30; /* VGEN3EN, VGEN3STBY, ~VGEN3LPWR */
			reg |= LDOB_2_50V;
			retval += pmic_reg_write(pfuze, PFUZE100_VGEN3VOL, reg);
#else
			/* Set VGEN3 to 2.5V, VGEN3CTL = low power in standby */
			retval += pmic_reg_read(pfuze, PFUZE100_VGEN3VOL, &reg);
			reg &= ~0x7f;
			reg |= 0x70; /* VGEN3EN, VGEN3STBY, VGEN3LPWR */
			reg |= LDOB_2_50V;
			retval += pmic_reg_write(pfuze, PFUZE100_VGEN3VOL, reg);
#endif
		} else {

			/* Set SW1AB standby volage to 0.9V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW1ABSTBY, &reg);
			reg &= ~SW1x_STBY_MASK;
			reg |= SW1x_0_900V;
			retval += pmic_reg_write(pfuze, PFUZE100_SW1ABSTBY, reg);

			/* Set SW1AB off volage to 0.9V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW1ABOFF, &reg);
			reg &= ~SW1x_OFF_MASK;
			reg |= SW1x_0_900V;
			retval += pmic_reg_write(pfuze, PFUZE100_SW1ABOFF, reg);

			/* Set SW1C standby voltage to 0.9V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW1CSTBY, &reg);
			reg &= ~SW1x_STBY_MASK;
			reg |= SW1x_0_900V;
			retval += pmic_reg_write(pfuze, PFUZE100_SW1CSTBY, reg);

			/* Set SW1C off volage to 0.9V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW1COFF, &reg);
			reg &= ~SW1x_OFF_MASK;
			reg |= SW1x_0_900V;
			retval += pmic_reg_write(pfuze, PFUZE100_SW1COFF, reg);

			/* Set SW2 to 3.3V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW2VOL, &reg);
			reg &= ~0x7f; /* SW2x NORMAL MASK */
			reg |= 0x72; /* SW2x 3.3V */
			retval += pmic_reg_write(pfuze, PFUZE100_SW2VOL, reg);

			/* Set SW2 standby voltage to 3.2V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW2STBY, &reg);
			reg &= ~0x7f; /* SW2x STBY MASK */
			reg |= 0x70; /* SW2x 3.2V */
			retval += pmic_reg_write(pfuze, PFUZE100_SW2STBY, reg);

			/* Set SW2 off voltage to 3.2V */
			retval += pmic_reg_read(pfuze, PFUZE100_SW2OFF, &reg);
			reg &= ~0x7f; /* SW2x OFF MASK */
			reg |= 0x70; /* SW2x 3.2V */
			retval += pmic_reg_write(pfuze, PFUZE100_SW2OFF, reg);

			reg = PWM_PFM;
			retval += pmic_reg_write(pfuze, PFUZE100_SW1ABMODE, reg);

			/* Set SW1AB/VDDARM step ramp up time 2us */
			retval += pmic_reg_read(pfuze, PFUZE100_SW1ABCONF, &reg);
			reg &= ~SW1xCONF_DVSSPEED_MASK;
			reg |= SW1xCONF_DVSSPEED_2US;
			retval += pmic_reg_write(pfuze, PFUZE100_SW1ABCONF, reg);

			reg = PWM_PFM;
			retval += pmic_reg_write(pfuze, PFUZE100_SW1CMODE, reg);

			reg = PWM_PFM;
			retval += pmic_reg_write(pfuze, PFUZE100_SW2MODE, reg);

			reg = LDOB_3_30V;
			retval += pmic_reg_write(pfuze, PFUZE100_VGEN6VOL, reg);
		}

		/* Set SW1C/VDDSOC step ramp up time from 16us to 4us/25mV */
		retval += pmic_reg_read(pfuze, PFUZE100_SW1CCONF, &reg);
		reg &= ~SW1xCONF_DVSSPEED_MASK;
		reg |= SW1xCONF_DVSSPEED_4US;
		retval += pmic_reg_write(pfuze, PFUZE100_SW1CCONF, reg);

		if (retval) {
			printf("PMIC write voltages error!\n");
			return -1;
		}
	}

	return 0;
}

static void update_env(void)
{
	setenv("mmcroot" , "/dev/mmcblk0p2 rootwait rw");
	switch (get_cpu_type()) {
	case MXC_CPU_MX6Q:
	case MXC_CPU_MX6D:
		if (is_dart_board()) {
			setenv("fdt_file", "imx6q-var-dart.dtb");
			if (MMC_BOOT == get_mmc_boot_device())
				setenv("mmcroot" , "/dev/mmcblk2p2 rootwait rw");
			else
				setenv("mmcroot" , "/dev/mmcblk1p2 rootwait rw");
		} else {
			if (is_solo_custom_board())
				setenv("fdt_file", "imx6q-var-som-vsc.dtb");
			else
				setenv("fdt_file", "imx6q-var-som.dtb");
		}
		break;
	case MXC_CPU_MX6DL:
	case MXC_CPU_MX6SOLO:
		if (is_som_solo())
			if (is_solo_custom_board())
				setenv("fdt_file", "imx6dl-var-som-solo-vsc.dtb");
			else
				setenv("fdt_file", "imx6dl-var-som-solo.dtb");
		else
			setenv("fdt_file", "imx6dl-var-som.dtb");
		break;
	default:
		printf("Error: Could not auto-match correct fdt due to CPU type.\n");
		printf("CPU type num is 0x%x\n", get_cpu_type());
		break;
	}
}

int board_late_init(void)
{
	char *s;

	print_emmc_size();

	s = getenv("var_auto_env");
	if (s[0] == 'Y')
		update_env();

	return 0;
}

int checkboard(void)
{
	puts("Board: Variscite VAR-SOM-MX6 ");

	switch (get_cpu_type()) {
	case MXC_CPU_MX6Q:
		puts("Quad");
		if (is_cpu_pop_package())
			puts("-POP");
		break;
	case MXC_CPU_MX6D:
		puts("Dual");
		if (is_cpu_pop_package())
			puts("-POP");
		break;
	case MXC_CPU_MX6DL:
		if (is_som_solo())
			puts("SOM-Dual");
		else
			puts("Dual Lite");
		break;
	case MXC_CPU_MX6SOLO:
		if (is_som_solo())
			puts("SOM-Solo");
		else
			puts("Solo");
		break;
	default:
		puts("????");
		break;
	}

	puts("\n");
	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT

void board_fastboot_setup(void)
{
	switch (get_boot_device()) {
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case SD2_BOOT:
	case MMC2_BOOT:
	    if (!getenv("fastboot_dev"))
			setenv("fastboot_dev", "mmc0");
	    if (!getenv("bootcmd"))
			setenv("bootcmd", "boota mmc0");
	    break;
	case SD3_BOOT:
	case MMC3_BOOT:
	    if (!getenv("fastboot_dev"))
			setenv("fastboot_dev", "mmc1");
	    if (!getenv("bootcmd"))
			setenv("bootcmd", "boota mmc1");
	    break;
#endif /*CONFIG_FASTBOOT_STORAGE_MMC*/
	default:
		printf("unsupported boot devices\n");
		break;
	}

}

#ifdef CONFIG_ANDROID_RECOVERY

#define GPIO_VOL_DN_KEY IMX_GPIO_NR(1, 5)

int check_recovery_cmd_file(void)
{
	int button_pressed = 0;
	int recovery_mode = 0;

	recovery_mode = recovery_check_and_clean_flag();

	/* Check Recovery Combo Button press or not. */
	SETUP_IOMUX_PAD(PAD_GPIO_5__GPIO1_IO05 | MUX_PAD_CTRL(NO_PAD_CTRL));

	gpio_direction_input(GPIO_VOL_DN_KEY);

	if (gpio_get_value(GPIO_VOL_DN_KEY) == 0) { /* VOL_DN key is low assert */
		button_pressed = 1;
		printf("Recovery key pressed\n");
	}

	return recovery_mode || button_pressed;
}

void board_recovery_setup(void)
{
	int bootdev = get_boot_device();

	switch (bootdev) {
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case SD2_BOOT:
	case MMC2_BOOT:
		if (!getenv("bootcmd_android_recovery"))
			setenv("bootcmd_android_recovery",
				"boota mmc0 recovery");
		break;
	case SD3_BOOT:
	case MMC3_BOOT:
		if (!getenv("bootcmd_android_recovery"))
			setenv("bootcmd_android_recovery",
				"boota mmc1 recovery");
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_MMC*/
	default:
		printf("Unsupported bootup device for recovery: dev: %d\n",
			bootdev);
		return;
	}

	printf("setup env for recovery..\n");
	setenv("bootcmd", "run bootcmd_android_recovery");
}

#endif /*CONFIG_ANDROID_RECOVERY*/

#endif /*CONFIG_FSL_FASTBOOT*/


#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#include <libfdt.h>
#include <asm/arch/mx6-ddr.h>
#include "mx6var_eeprom.h"
#include "mx6var_eeprom_v2.h"
#include "mx6var_legacy.h"

#include "mx6var_legacy_dart_auto.c"

static void ccgr_init(void)
{
	struct mxc_ccm_reg *ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	writel(0x00C03F3F, &ccm->CCGR0);
	writel(0x0030FC03, &ccm->CCGR1);
	writel(0x0FFFC000, &ccm->CCGR2);
	writel(0x3FF00000, &ccm->CCGR3);
	writel(0x00FFF300, &ccm->CCGR4);
	writel(0x0F0000C3, &ccm->CCGR5);
	writel(0x000003FF, &ccm->CCGR6);
}

static void gpr_init(void)
{
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;

	/* enable AXI cache for VDOA/VPU/IPU */
	writel(0xF00000CF, &iomux->gpr[4]);
	/* set IPU AXI-id0 Qos=0xf(bypass) AXI-id1 Qos=0x7 */
	writel(0x007F007F, &iomux->gpr[6]);
	writel(0x007F007F, &iomux->gpr[7]);
}

static int power_init_pmic_sw2(void)
{
	if (!is_som_solo() && !is_dart_board()) {
		unsigned char reg;

		i2c_set_bus_num(PMIC_I2C_BUS);

		if (i2c_probe(CONFIG_POWER_PFUZE100_I2C_ADDR))
			return -1;

		/* Set SW2 to 3.2V */
		if (i2c_read(CONFIG_POWER_PFUZE100_I2C_ADDR, PFUZE100_SW2VOL, 1, &reg, 1))
			return -1;

		reg &= ~0x7f;
		reg |= 0x70; /* SW2x 3.2V */

		if (i2c_write(CONFIG_POWER_PFUZE100_I2C_ADDR, PFUZE100_SW2VOL, 1, &reg, 1))
			return -1;
	}

	return 0;
}

static void var_setup_iomux_per_vcc_en(void)
{
	SETUP_IOMUX_PAD(PAD_EIM_D31__GPIO3_IO31 | MUX_PAD_CTRL(PER_VCC_EN_PAD_CTRL));
	gpio_direction_output(IMX_GPIO_NR(3, 31), 1);
}

/*
 * Writes RAM size to RAM_SIZE_ADDR so U-Boot can read it
 */
static void var_set_ram_size(u32 ram_size)
{
	u32 *p_ram_size = (u32 *)RAM_SIZE_ADDR;
	if (ram_size > 3840)
		ram_size = 3840;
	*p_ram_size = ram_size;
}

/*
 * OCOTP_CFG3[17:16] (see Fusemap Description Table offset 0x440)
 * defines a 2-bit SPEED_GRADING
 */
#define OCOTP_CFG3_SPEED_SHIFT  16
#define OCOTP_CFG3_SPEED_800MHZ 0
#define OCOTP_CFG3_SPEED_850MHZ 1
#define OCOTP_CFG3_SPEED_1GHZ   2
#define OCOTP_CFG3_SPEED_1P2GHZ 3

u32 get_cpu_speed_grade_hz(void)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[0];
	struct fuse_bank0_regs *fuse =
		(struct fuse_bank0_regs *)bank->fuse_regs;
	uint32_t val;

	val = readl(&fuse->cfg3);
	val >>= OCOTP_CFG3_SPEED_SHIFT;
	val &= 0x3;

	switch (val) {
	/* Valid for IMX6DQ */
	case OCOTP_CFG3_SPEED_1P2GHZ:
		if (is_cpu_type(MXC_CPU_MX6Q) || is_cpu_type(MXC_CPU_MX6D))
			return 1200000000;
	/* Valid for IMX6SX/IMX6SDL/IMX6DQ */
	case OCOTP_CFG3_SPEED_1GHZ:
		return 996000000;
	/* Valid for IMX6DQ */
	case OCOTP_CFG3_SPEED_850MHZ:
		if (is_cpu_type(MXC_CPU_MX6Q) || is_cpu_type(MXC_CPU_MX6D))
			return 852000000;
	/* Valid for IMX6SX/IMX6SDL/IMX6DQ */
	case OCOTP_CFG3_SPEED_800MHZ:
		return 792000000;
	}
	return 0;
}

static void print_board_info(int eeprom_rev, void* var_eeprom, u32 ram_size)
{
	u32 boot_device;
	u32 max_freq;

	printf("\ni.MX%s SOC P-%s\n", get_imx_type(get_cpu_type()), \
			is_cpu_pop_package() ? "POP" : "STD");

	max_freq = get_cpu_speed_grade_hz();
	if (!max_freq)
		printf("CPU running at %dMHz\n", mxc_get_clock(MXC_ARM_CLK) / 1000000);
	else
		printf("%d MHz CPU (running at %d MHz)\n", max_freq / 1000000,
				mxc_get_clock(MXC_ARM_CLK) / 1000000);

	if (eeprom_rev == 1)
		var_eeprom_strings_print((struct var_eeprom_cfg *) var_eeprom);
	else if (eeprom_rev == 2)
		var_eeprom_v2_strings_print((struct var_eeprom_v2_cfg *) var_eeprom);
	else
		printf("DDR LEGACY configuration\n");

	printf("RAM: ");
	print_size(ram_size * 1024 * 1024, "\n");

	printf("Boot Device: ");
	boot_device = spl_boot_device();
	switch (boot_device) {
	case BOOT_DEVICE_MMC1:
		switch (get_mmc_boot_device()) {
		case SD_BOOT:
			printf("MMC0 (SD)\n");
			break;
		case MMC_BOOT:
			printf("MMC1 (DART eMMC)\n");
			break;
		default:
			printf("MMC?\n");
			break;
		}
		break;

	case BOOT_DEVICE_NAND:
		printf("NAND\n");
		break;

	default:
		printf("UNKNOWN (%d)\n", boot_device);
		break;
	}
}

static int spl_dram_init_by_eeprom(void)
{
	struct var_eeprom_cfg var_eeprom_cfg = {{0}};
	int ret;

	ret = var_eeprom_read_struct(&var_eeprom_cfg);
	if (ret)
		return SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM;

	/* is valid Variscite EEPROM? */
	if (!var_eeprom_is_valid(&var_eeprom_cfg))
		return SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM_STRUCT_DETECTED;

	switch (get_cpu_type()) {
	case MXC_CPU_MX6DL:
	case MXC_CPU_MX6SOLO:
		var_eeprom_mx6dlsl_dram_setup_iomux_from_struct(&var_eeprom_cfg.pinmux_group);
		break;
	case MXC_CPU_MX6Q:
	case MXC_CPU_MX6D:
	default:
		var_eeprom_mx6qd_dram_setup_iomux_from_struct(&var_eeprom_cfg.pinmux_group);
		break;
	}

	var_eeprom_dram_init_from_struct(&var_eeprom_cfg);

	var_set_ram_size(var_eeprom_cfg.header.ddr_size);
	print_board_info(1, (void*) &var_eeprom_cfg, var_eeprom_cfg.header.ddr_size);

	return SPL_DRAM_INIT_STATUS_OK;
}

static int spl_dram_init_by_eeprom_v2(void)
{
	u32 ram_addresses[MAXIMUM_RAM_ADDRESSES];
	u32 ram_values[MAXIMUM_RAM_VALUES];
	struct var_eeprom_v2_cfg var_eeprom_v2_cfg = {0};
	int ret, ram_size;

	ret = var_eeprom_v2_read_struct(&var_eeprom_v2_cfg, \
			is_dart_board() ? VAR_DART_EEPROM_CHIP : VAR_MX6_EEPROM_CHIP);

	if (ret)
		return SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM;

	if (!var_eeprom_v2_is_valid(&var_eeprom_v2_cfg))
		return SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM_STRUCT_DETECTED;

	/*
	 * The MX6Q_MMDC_LPDDR2_register_programming_aid_v0_Micron_InterL_commands
	 * revision is incorrect.
	 * If the data is equal to it, use mt128x64mx32_Step3_commands revision instead
	 */
	if (!memcmp(var_eeprom_v2_cfg.eeprom_commands, \
				MX6Q_MMDC_LPDDR2_register_programming_aid_v0_Micron_InterL_commands, 254)) {

		load_custom_data(mt128x64mx32_Step3_RamValues, ram_addresses, ram_values);
		setup_ddr_parameters((struct eeprom_command *) mt128x64mx32_Step3_commands, \
				ram_addresses, ram_values);
	} else {
		handle_eeprom_data(&var_eeprom_v2_cfg, ram_addresses, ram_values);
	}

	ram_size = var_eeprom_v2_cfg.ddr_size * 128;
	var_set_ram_size(ram_size);
	print_board_info(2, (void*) &var_eeprom_v2_cfg, ram_size);
	return SPL_DRAM_INIT_STATUS_OK;
}

static void legacy_spl_dram_init(void)
{
	u32 ram_size;
	switch (get_cpu_type()) {
	case MXC_CPU_MX6SOLO:
		spl_mx6dlsl_dram_setup_iomux();
		spl_dram_init_mx6solo_1gb();
		ram_size = get_actual_ram_size(1024);
		if (ram_size == 512) {
			udelay(1000);
			reset_ddr_solo();
			spl_mx6dlsl_dram_setup_iomux();
			spl_dram_init_mx6solo_512mb();
			udelay(1000);
		}
		break;
	case MXC_CPU_MX6Q:
	case MXC_CPU_MX6D:
		if (is_dart_board()) {
			u32 ram_addresses[MAXIMUM_RAM_ADDRESSES];
			u32 ram_values[MAXIMUM_RAM_VALUES];
			load_custom_data(mt128x64mx32_Step3_RamValues, ram_addresses, ram_values);
			setup_ddr_parameters((struct eeprom_command *) mt128x64mx32_Step3_commands, \
					ram_addresses, ram_values);
			ram_size = 1024;
		} else {
			spl_mx6qd_dram_setup_iomux();
#ifdef CONFIG_DDR_2GB
			spl_dram_init_mx6q_2g();
			ram_size = get_actual_ram_size(2048);
#else
			if (get_cpu_speed_grade_hz() == 1200000000) {
				spl_dram_init_mx6q_2g();
				ram_size = get_actual_ram_size(2048);
			} else {
				spl_dram_init_mx6q_1g();
				ram_size = get_actual_ram_size(1024);
			}
#endif
		}
		break;
	case MXC_CPU_MX6DL:
	default:
		spl_mx6dlsl_dram_setup_iomux();
		if (is_som_solo())
			spl_dram_init_mx6solo_1gb();
		else
			spl_dram_init_mx6dl_1g();
		ram_size = get_actual_ram_size(1024);
		break;
	}

	var_set_ram_size(ram_size);
	print_board_info(0, NULL, ram_size);
}

/*
 * Bugfix: Fix Freescale wrong processor documentation.
 */
static void spl_mx6qd_dram_setup_iomux_check_reset(void)
{
	volatile struct mx6dq_iomux_ddr_regs *mx6q_ddr_iomux;
	u32 cputype = get_cpu_type();

	if ((cputype == MXC_CPU_MX6D) || (cputype == MXC_CPU_MX6Q)) {
		mx6q_ddr_iomux = (struct mx6dq_iomux_ddr_regs *) MX6DQ_IOM_DDR_BASE;

		if (mx6q_ddr_iomux->dram_reset == (u32)0x000C0030)
			mx6q_ddr_iomux->dram_reset = (u32)0x00000030;
	}
}

static void spl_dram_init(void)
{
	int status;

	status = spl_dram_init_by_eeprom();
	if (status != SPL_DRAM_INIT_STATUS_OK) {
		status = spl_dram_init_by_eeprom_v2();
		if (status != SPL_DRAM_INIT_STATUS_OK)
			legacy_spl_dram_init();
	}
	spl_mx6qd_dram_setup_iomux_check_reset();
}

void board_init_f(ulong dummy)
{
	/* setup AIPS and disable watchdog */
	arch_cpu_init();

	ccgr_init();
	gpr_init();

	/* iomux and setup of i2c */
	board_early_init_f();

	/* setup GP timer */
	timer_init();

#ifdef LOW_POWER_MODE_ENABLE
	power_init_pmic_sw2();
#endif
	var_setup_iomux_per_vcc_en();

	/* Wait 330ms before starting to print to console */
	mdelay(330);

	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();

	/* DDR initialization */
	spl_dram_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* load/boot image from boot device */
	board_init_r(NULL, 0);
}

void reset_cpu(ulong addr)
{
}
#endif
