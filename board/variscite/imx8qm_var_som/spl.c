/*
 * Copyright 2017-2018 NXP
 * Copyright 2020 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <spl.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <fsl_ifc.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <environment.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include "pca953x.h"

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <dm.h>
#include <imx8_hsio.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/video.h>
#include <asm/arch/video_common.h>
#include <power-domain.h>

#include "../common/imx8_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

#define ESDHC_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ESDHC_CLK_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#ifdef CONFIG_FSL_ESDHC

enum {
	SPEAR_MX8,
	VAR_SOM_MX8,
	UNKNOWN_BOARD,
};

static int get_board_id(void)
{
	struct var_eeprom eeprom;
	static int board_id = UNKNOWN_BOARD;

	if (board_id == UNKNOWN_BOARD) {
		if (!var_scu_eeprom_read_header(&eeprom) &&
		    var_eeprom_is_valid(&eeprom) && (eeprom.somrev & 0x40))
			board_id = SPEAR_MX8;
		else
			board_id = VAR_SOM_MX8;
	}

	return board_id;
}

static struct fsl_esdhc_cfg usdhc_cfg[CONFIG_SYS_FSL_USDHC_NUM] = {
	{USDHC1_BASE_ADDR, 0, 8},
	{USDHC2_BASE_ADDR, 0, 4},
};

#define USDHC1_CD_GPIO_SOM	IMX_GPIO_NR(0, 14)
#define USDHC1_CD_GPIO_SPEAR	IMX_GPIO_NR(5, 22)
#define USDHC1_PWR_GPIO_SPEAR	IMX_GPIO_NR(4, 7)

static iomux_cfg_t emmc0[] = {
	SC_P_EMMC0_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_EMMC0_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA4 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA5 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA6 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA7 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_RESET_B | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_STROBE | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};
static iomux_cfg_t usdhc1_sd[] = {
	SC_P_USDHC1_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_USDHC1_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_VSELECT | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};

static iomux_cfg_t const usdhc1_pwr_pads_spear[] = {
	SC_P_USDHC1_RESET_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static iomux_cfg_t const usdhc1_cd_pads_som[] = {
	SC_P_GPT0_CLK | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static iomux_cfg_t const usdhc1_cd_pads_spear[] = {
	SC_P_USDHC1_DATA7 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

int board_mmc_init(bd_t *bis)
{
	int i, ret;
	sc_ipc_t ipcHndl = 0;
	iomux_cfg_t const *usdhc1_cd_pads;
	int usdhc1_cd_gpio;
	ipcHndl = gd->arch.ipc_channel_handle;

	if (get_board_id() == SPEAR_MX8) {
		usdhc1_cd_pads = usdhc1_cd_pads_spear;
		usdhc1_cd_gpio = USDHC1_CD_GPIO_SPEAR;
	}
	else {
		usdhc1_cd_pads = usdhc1_cd_pads_som;
		usdhc1_cd_gpio = USDHC1_CD_GPIO_SOM;
	}

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1                    USDHC2
	 * mmc2                    USDHC3
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_SDHC_0, SC_PM_PW_MODE_ON);
			if (ret != SC_ERR_NONE)
				return ret;

			imx8_iomux_setup_multiple_pads(emmc0, ARRAY_SIZE(emmc0));
			init_clk_usdhc(0);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			break;
		case 1:
			ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_SDHC_1, SC_PM_PW_MODE_ON);
			if (ret != SC_ERR_NONE)
				return ret;
			ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_GPIO_0, SC_PM_PW_MODE_ON);
			if (ret != SC_ERR_NONE)
				return ret;
			ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_GPIO_4, SC_PM_PW_MODE_ON);
			if (ret != SC_ERR_NONE)
				return ret;
			ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_GPIO_5, SC_PM_PW_MODE_ON);
			if (ret != SC_ERR_NONE)
				return ret;

			imx8_iomux_setup_multiple_pads(usdhc1_sd, ARRAY_SIZE(usdhc1_sd));
			imx8_iomux_setup_multiple_pads(usdhc1_cd_pads, ARRAY_SIZE(usdhc1_cd_pads));
			if (get_board_id() == SPEAR_MX8)
				imx8_iomux_setup_multiple_pads(usdhc1_pwr_pads_spear,
					ARRAY_SIZE(usdhc1_pwr_pads_spear));

			init_clk_usdhc(1);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);

			gpio_request(usdhc1_cd_gpio, "sd1_cd");
			gpio_direction_input(usdhc1_cd_gpio);

			if (get_board_id() == SPEAR_MX8) {
				gpio_request(USDHC1_PWR_GPIO_SPEAR, "sd1_pwr");
				gpio_direction_output(USDHC1_PWR_GPIO_SPEAR, 0);
				mdelay(10);
				gpio_direction_output(USDHC1_PWR_GPIO_SPEAR, 1);
			}
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return 0;
		}
		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret) {
			printf("Warning: failed to initialize mmc dev %d\n", i);
			return ret;
		}
	}

	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;
	int usdhc1_cd_gpio;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = 1; /* eMMC */
		break;
	case USDHC2_BASE_ADDR:
		if (get_board_id() == SPEAR_MX8)
			usdhc1_cd_gpio = USDHC1_CD_GPIO_SPEAR;
		else
			usdhc1_cd_gpio = USDHC1_CD_GPIO_SOM;
		ret = !gpio_get_value(usdhc1_cd_gpio);
		break;
	}

	return ret;
}

#endif /* CONFIG_FSL_ESDHC */

void spl_board_init(void)
{
#if defined(CONFIG_SPL_SPI_SUPPORT)
	sc_ipc_t ipcHndl = 0;

	ipcHndl = gd->arch.ipc_channel_handle;
	if (sc_rm_is_resource_owned(ipcHndl, SC_R_FSPI_0)) {
		if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_FSPI_0, SC_PM_PW_MODE_ON)) {
			puts("Warning: failed to initialize FSPI0\n");
		}
	}
#endif

	puts("Normal Boot\n");
}

void spl_board_prepare_for_boot(void)
{
#if defined(CONFIG_SPL_SPI_SUPPORT)
	sc_ipc_t ipcHndl = 0;

	ipcHndl = gd->arch.ipc_channel_handle;
	if (sc_rm_is_resource_owned(ipcHndl, SC_R_FSPI_0)) {
		if (sc_pm_set_resource_power_mode(ipcHndl, SC_R_FSPI_0, SC_PM_PW_MODE_OFF)) {
			puts("Warning: failed to turn off FSPI0\n");
		}
	}
#endif
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	return 0;
}
#endif

void board_init_f(ulong dummy)
{
	/* Clear global data */
	memset((void *)gd, 0, sizeof(gd_t));

	arch_cpu_init();

	board_early_init_f();

	timer_init();

	preloader_console_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	board_init_r(NULL, 0);
}

