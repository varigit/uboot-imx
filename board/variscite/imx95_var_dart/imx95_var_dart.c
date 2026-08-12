// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 Variscite Ltd.
 */

#include <env.h>
#include <efi_loader.h>
#include <init.h>
#include <asm/arch/clock.h>
#include <usb.h>
#include <miiphy.h>
#include <netdev.h>
#include <dwc3-uboot.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <power/regulator.h>
#include <scmi_agent.h>
#include "../dts/upstream/src/arm64/freescale/imx95-power.h"
#include <asm/arch/sys_proto.h>
#include <i2c.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>
#include <dt-bindings/gpio/gpio.h>

#include "../common/imx9_eeprom.h"
#include "../common/extcon-ptn5150.h"

DECLARE_GLOBAL_DATA_PTR;

extern int var_setup_mac(struct var_eeprom *eeprom);
extern int board_fix_fdt_fuse(void *fdt);
static void board_sm_cfg_info(void);

/* Carrier board EEPROM */
#define CARRIER_EEPROM_I2C_NAME		"i2c@42530000"
#define CARRIER_EEPROM_ADDR		0x54

static struct var_eeprom eeprom = {0};
static bool m7_is_powered = false;

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0x2c4db6b3, 0x0b15, 0x4a36, 0xbe, 0xae, \
		 0x1e, 0xa1, 0x35, 0x46, 0x4f, 0x5b)

struct efi_fw_image fw_images[] = {
	{
		.image_type_id = IMX_BOOT_IMAGE_GUID,
		.fw_name = u"IMX95-EVK-RAW",
		.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "mmc 0=flash-bin raw 0 0x2000 mmcpart 1",
	.num_images = ARRAY_SIZE(fw_images),
	.images = fw_images,
};
#endif /* EFI_HAVE_CAPSULE_SUPPORT */

int board_early_init_f(void)
{
	/* UART1: A55, UART3: M7 */
	init_uart_clk(0);

	return 0;
}

#ifdef CONFIG_USB_DWC3

#define PHY_CTRL0			0xF0040
#define PHY_CTRL0_REF_SSP_EN		BIT(2)
#define PHY_CTRL0_FSEL_MASK		GENMASK(10, 5)
#define PHY_CTRL0_FSEL_24M		0x2a
#define PHY_CTRL0_FSEL_100M		0x27
#define PHY_CTRL0_SSC_RANGE_MASK	GENMASK(23, 21)
#define PHY_CTRL0_SSC_RANGE_4003PPM	(0x2 << 21)

#define PHY_CTRL1			0xF0044
#define PHY_CTRL1_RESET			BIT(0)
#define PHY_CTRL1_COMMONONN		BIT(1)
#define PHY_CTRL1_ATERESET		BIT(3)
#define PHY_CTRL1_DCDENB		BIT(17)
#define PHY_CTRL1_CHRGSEL		BIT(18)
#define PHY_CTRL1_VDATSRCENB0		BIT(19)
#define PHY_CTRL1_VDATDETENB0		BIT(20)

#define PHY_CTRL2			0xF0048
#define PHY_CTRL2_TXENABLEN0		BIT(8)
#define PHY_CTRL2_OTG_DISABLE		BIT(9)

#define PHY_CTRL6			0xF0058
#define PHY_CTRL6_RXTERM_OVERRIDE_SEL	BIT(29)
#define PHY_CTRL6_ALT_CLK_EN		BIT(1)
#define PHY_CTRL6_ALT_CLK_SEL		BIT(0)

static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_HIGH,
	.base = USB1_BASE_ADDR,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.power_down_scale = 2,
};

static void dwc3_nxp_usb_phy_init(struct dwc3_device *dwc3)
{
	u32 value;

	/* USB3.0 PHY signal fsel for 24M ref */
	value = readl(dwc3->base + PHY_CTRL0);
	value &= ~PHY_CTRL0_FSEL_MASK;
	value |= FIELD_PREP(PHY_CTRL0_FSEL_MASK, PHY_CTRL0_FSEL_24M);
	writel(value, dwc3->base + PHY_CTRL0);

	/* Disable alt_clk_en and use internal MPLL clocks */
	value = readl(dwc3->base + PHY_CTRL6);
	value &= ~(PHY_CTRL6_ALT_CLK_SEL | PHY_CTRL6_ALT_CLK_EN);
	writel(value, dwc3->base + PHY_CTRL6);

	value = readl(dwc3->base + PHY_CTRL1);
	value &= ~(PHY_CTRL1_VDATSRCENB0 | PHY_CTRL1_VDATDETENB0);
	value |= PHY_CTRL1_RESET | PHY_CTRL1_ATERESET;
	writel(value, dwc3->base + PHY_CTRL1);

	value = readl(dwc3->base + PHY_CTRL0);
	value |= PHY_CTRL0_REF_SSP_EN;
	writel(value, dwc3->base + PHY_CTRL0);

	value = readl(dwc3->base + PHY_CTRL2);
	value |= PHY_CTRL2_TXENABLEN0 | PHY_CTRL2_OTG_DISABLE;
	writel(value, dwc3->base + PHY_CTRL2);

	udelay(10);

	value = readl(dwc3->base + PHY_CTRL1);
	value &= ~(PHY_CTRL1_RESET | PHY_CTRL1_ATERESET);
	writel(value, dwc3->base + PHY_CTRL1);
}
#endif

static int imx9_scmi_power_domain_enable(u32 domain, bool enable)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_CLK, "protocol@14", &dev);
	if (ret)
		return ret;

	return scmi_pwd_state_set(dev, 0, domain, enable ? 0 : BIT(30));
}

#ifdef CONFIG_EXTCON_PTN5150
static struct extcon_ptn5150 usb_ptn5150;
#endif

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

#if (!defined(CONFIG_SPL_BUILD) && defined(CONFIG_EXTCON_PTN5150))
	if (index == 0) {
		/* Verify port is in proper mode */
		int phy_mode = extcon_ptn5150_phy_mode(&usb_ptn5150);

		/* Only verify phy_mode if ptn5150 is initialized */
		if (phy_mode >= 0 && phy_mode != init)
			return -ENODEV;
	}
#endif

	if (index == 0 && init == USB_INIT_DEVICE) {
		ret = imx9_scmi_power_domain_enable(IMX95_PD_HSIO_TOP, true);
		if (ret) {
			printf("SCMI_POWWER_STATE_SET Failed for USB\n");
			return ret;
		}

#ifdef CONFIG_USB_DWC3
		dwc3_nxp_usb_phy_init(&dwc3_device_data);
		return dwc3_uboot_init(&dwc3_device_data);
#endif
	} else if (index == 0 && init == USB_INIT_HOST) {
		return ret;
	}

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;
	if (index == 0 && init == USB_INIT_DEVICE) {
#ifdef CONFIG_USB_DWC3
		dwc3_uboot_exit(index);
#endif
	}

	return ret;
}

static void netc_phy_rst(const char *gpio_name, const char *label)
{
	int ret;
	struct gpio_desc desc;

	/* ENET_RST_B */
	ret = dm_gpio_lookup_name(gpio_name, &desc);
	if (ret) {
		printf("%s lookup %s failed ret = %d\n", __func__, gpio_name, ret);
		return;
	}

	ret = dm_gpio_request(&desc, label);
	if (ret) {
		printf("%s request %s failed ret = %d\n", __func__, label, ret);
		return;
	}

	/* assert the ENET_RST_B */
	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE | GPIOD_ACTIVE_LOW);
	udelay(10000);
	dm_gpio_set_value(&desc, 0); /* deassert the ENET_RST_B */
	udelay(100000);

}

void netc_init(void)
{
	int ret;

	/* Power up the NETC MIX. */
	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for NETC MIX\n");
		return;
	}

	set_clk_netc(ENET_125MHZ);

	netc_phy_rst("GPIO5_16", "ENET1_RST_B");
	/* Reset the ethernet phy only if exists */
	if (eeprom.features & VAR_EEPROM_F_ETH)
		netc_phy_rst("i2c8_io@21_0", "ENET2_RST_B");
	netc_phy_rst("i2c3_io@22_5", "ETH10G_SEL");

	pci_init();
}

#if CONFIG_IS_ENABLED(NET)
int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

int board_init(void)
{
	int ret;
	struct var_eeprom *ep = &eeprom;
	ret = imx9_scmi_power_domain_enable(IMX95_PD_HSIO_TOP, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for USB\n");
		return ret;
	}

	imx9_scmi_power_domain_enable(IMX95_PD_DISPLAY, false);
	imx9_scmi_power_domain_enable(IMX95_PD_CAMERA, false);

	/* Read EEPROM data */
	var_eeprom_read_header(ep);

	netc_init();

	m7_is_powered = false;
	if (power_on_m7("dart-mx95-m7") == 0) {
		m7_is_powered = true;
	} else if (power_on_m7("dart-mx95-m7deb") == 0) {
		m7_is_powered = true;
	} else {
		printf("Cortex M7 core not powered ON\n");
	}

#ifdef CONFIG_EXTCON_PTN5150
	extcon_ptn5150_setup(&usb_ptn5150);
#endif
	return 0;
}

int board_late_init(void)
{
	struct var_eeprom *ep = &eeprom;
	struct var_carrier_eeprom carrier_eeprom;
	char som_rev[CARRIER_REV_LEN] = {0};
	char carrier_rev[CARRIER_REV_LEN] = {0};
	char carrier_name[CARRIER_REV_LEN] = {0};

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

	/* SoM Features ENV */
	env_set("som_has_wbe", (ep->features & VAR_EEPROM_F_WBE) ? "1" : "0");

	/* SoM Rev and Board name ENV */
	snprintf(som_rev, CARRIER_REV_LEN, "%ld.%ld", SOMREV_MAJOR(ep->somrev), SOMREV_MINOR(ep->somrev));
	env_set("som_rev", som_rev);
	if (SOMREV_MAJOR(ep->somrev) == 2)
		env_set("board_name", "DART-MX95_V2");
	else
		env_set("board_name", "DART-MX95");

	/* Carrier Rev ENV */
	var_carrier_eeprom_read(CARRIER_EEPROM_I2C_NAME, CARRIER_EEPROM_ADDR, &carrier_eeprom);
	var_carrier_eeprom_get_revision(&carrier_eeprom, carrier_rev, sizeof(carrier_rev), VAR_DART);
	env_set("carrier_rev", carrier_rev);

	if (var_carrier_eeprom_get_name(&carrier_eeprom, carrier_name, VAR_DART) > 0)
		env_set("carrier_name", carrier_name);

	/* To avoid U-Boot crash running Cortex M7 demos */
	if (m7_is_powered == false) {
		printf ("Force use_m7=no because Cortex-M7 is not powered");
		env_set("use_m7", "no");
		env_set("m7_dtb_suffix", "");
	}
	else
		env_set("m7_dtb_suffix", "-m7");

	var_setup_mac(ep);

	var_eeprom_print_prod_info(ep);

	board_sm_cfg_info();

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, struct bd_info *bd)
{
	return 0;
}
#endif

void board_quiesce_devices(void)
{
	int ret;

	ret = imx9_scmi_power_domain_enable(IMX95_PD_HSIO_TOP, false);
	if (ret) {
		printf("%s: Failed for HSIO MIX: %d\n", __func__, ret);
		return;
	}

	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, false);
	if (ret) {
		printf("%s: Failed for NETC MIX: %d\n", __func__, ret);
		return;
	}
}

static void board_sm_cfg_info(void)
{
	char cfgname[SCMI_MISC_MAX_CFGNAME];
	u32 msel = 0;
	int ret;

	ret = scmi_misc_cfginfo(&msel, cfgname);
	if (ret)
		snprintf(cfgname, sizeof(cfgname), "Unknown");

	printf("SM: cfg: %s, msel: %u\n", cfgname, msel);
}

#if !defined(CONFIG_SPL_BUILD) && IS_ENABLED(CONFIG_OF_BOARD_FIXUP)
static void board_v2_fdt_fixup(void *fdt_blob)
{
	const char *node_path;
	int node_offset, ret;
	u32 phandle, phandle_with_args[3], fsl_pins[6];

	/* Find the existing ethernet-phy@0 node via its path */
	node_path = "/soc/netc-blk-ctrl@4cde0000/pcie@4cb00000/mdio@0,0/ethernet-phy@0";
	node_offset = fdt_path_offset(fdt_blob, node_path);
	if (node_offset < 0) {
		printf("%s: Couldn't find %s: %s\n", __func__, node_path,
			   fdt_strerror(node_offset));
		return;
	}

	/* Delete the existing ethernet-phy@0 node */
	ret = fdt_del_node(fdt_blob, node_offset);
	if (ret < 0) {
		printf("%s: Couldn't delete node %s: %s\n", __func__, node_path,
			   fdt_strerror(ret));
		return;
	}

	/* Increase the size of the device tree to accommodate new node and properties */
	ret = fdt_increase_size(fdt_blob, 16);
	if (ret < 0) {
		printf("%s: Couldn't increase size of device tree: %s\n", __func__,
			   fdt_strerror(ret));
		return;
	}

	/* Prepare <gpio2 18 GPIO_ACTIVE_LOW> for the PHY reset GPIO */
	node_path = "/soc/gpio@43810000";
	node_offset = fdt_path_offset(fdt_blob, node_path);
	if (node_offset < 0) {
		printf("%s: Couldn't find %s: %s\n", __func__, node_path,
			   fdt_strerror(node_offset));
		return;
	}

	phandle = fdt_get_phandle(fdt_blob, node_offset);
	phandle_with_args[0] = cpu_to_fdt32(phandle);
	phandle_with_args[1] = cpu_to_fdt32(18);
	phandle_with_args[2] = cpu_to_fdt32(GPIO_ACTIVE_LOW);

	/* Find the mdio@0,0 node where the new PHY node will be created */
	node_path = "/soc/netc-blk-ctrl@4cde0000/pcie@4cb00000/mdio@0,0";
	node_offset = fdt_path_offset(fdt_blob, node_path);
	if (node_offset < 0) {
		printf("%s: Couldn't find %s: %s\n", __func__, node_path,
			   fdt_strerror(node_offset));
		return;
	}

	/* Add the DART-MX95_V2-specific ethernet-phy@4 node under mdio@0,0 */
	node_path = "ethernet-phy@4";
	node_offset = fdt_add_subnode(fdt_blob, node_offset, node_path);
	if (node_offset < 0) {
		printf("%s: Couldn't create node %s: %s\n", __func__, node_path,
			   fdt_strerror(node_offset));
		return;
	}

	/* Set the required properties for the new ethernet-phy@4 node */
	ret = fdt_setprop_u32(fdt_blob, node_offset, "reg", 4);
	if (ret < 0) {
		printf("%s: Couldn't set reg for %s: %s\n", __func__, node_path,
			   fdt_strerror(ret));
		return;
	}
	ret = fdt_setprop_u32(fdt_blob, node_offset, "reset-assert-us", 10000);
	if (ret < 0) {
		printf("%s: Couldn't set reset-assert-us for %s: %s\n", __func__, node_path,
			   fdt_strerror(ret));
		return;
	}
	ret = fdt_setprop_u32(fdt_blob, node_offset, "reset-deassert-us", 100000);
	if (ret < 0) {
		printf("%s: Couldn't set reset-deassert-us for %s: %s\n", __func__, node_path,
			   fdt_strerror(ret));
		return;
	}
	ret = fdt_setprop(fdt_blob, node_offset, "reset-gpios",
		phandle_with_args, sizeof(phandle_with_args));
	if (ret < 0) {
		printf("%s: Couldn't set reset-gpios for %s: %s\n", __func__, node_path,
			   fdt_strerror(ret));
		return;
	}

	ret = fdt_setprop_empty(fdt_blob, node_offset, "ti,min-output-impedance");
	if (ret < 0) {
		printf("%s: Couldn't set ti,min-output-impedance for %s: %s\n", __func__, node_path,
			   fdt_strerror(ret));
		return;
	}

	/* Create a phandle for ethernet-phy@4 to be referenced by ethernet@0,0 */
	phandle = fdt_create_phandle(fdt_blob, node_offset);
	if (!phandle) {
		printf("%s: Couldn't create phandle for %s\n", __func__, node_path);
		return;
	}

	/* Find the ethernet@0,0 node that will reference the new PHY */
	node_path = "/soc/netc-blk-ctrl@4cde0000/pcie@4ca00000/ethernet@0,0";
	node_offset = fdt_path_offset(fdt_blob, node_path);
	if (node_offset < 0) {
		printf("%s: Couldn't find %s: %s\n", __func__, node_path,
			   fdt_strerror(node_offset));
		return;
	}

	/* Update phy-handle to reference the newly created ethernet-phy@4 node */
	ret = fdt_setprop_u32(fdt_blob, node_offset, "phy-handle", phandle);
	if (ret < 0) {
		printf("%s: Couldn't set phy-handle for %s: %s\n", __func__, node_path,
			   fdt_strerror(ret));
		return;
	}

	/* Find the phy0resgrp node used for the PHY reset GPIO configuration */
	node_path = "/firmware/scmi/protocol@19/phy0resgrp";
	node_offset = fdt_path_offset(fdt_blob, node_path);
	if (node_offset < 0) {
		printf("%s: Couldn't find %s: %s\n", __func__, node_path,
			   fdt_strerror(node_offset));
		return;
	}

	/* Configure GPIO2_IO18 as the PHY reset GPIO */
	/* IMX95_PAD_GPIO_IO18__GPIO2_IO_BIT18    0x0058 0x025C 0x0000 0x00 0x00 0x31e*/
	fsl_pins[0] = cpu_to_fdt32(0x0058);
	fsl_pins[1] = cpu_to_fdt32(0x025C);
	fsl_pins[2] = cpu_to_fdt32(0x0000);
	fsl_pins[3] = cpu_to_fdt32(0x00);
	fsl_pins[4] = cpu_to_fdt32(0x00);
	fsl_pins[5] = cpu_to_fdt32(0x31e);
	ret = fdt_setprop(fdt_blob, node_offset, "fsl,pins", fsl_pins, sizeof(fsl_pins));
	if (ret < 0) {
		printf("%s: Couldn't set fsl,pins for %s: %s\n", __func__, node_path,
			   fdt_strerror(ret));
		return;
	}
}

int board_fix_fdt(void *fdt)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	if (htons(ep->magic) == VAR_DART_EEPROM_MAGIC) {
		if (SOMREV_MAJOR(ep->somrev) == 2)
			board_v2_fdt_fixup(fdt);
	} else {
		printf("%s: Invalid EEPROM magic 0x%hx, expected 0x%hx\n", __func__,
			   htons(ep->magic), VAR_DART_EEPROM_MAGIC);
	}
	/* Remove nodes based on fuses. */
	return board_fix_fdt_fuse(fdt);
}
#endif
#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0;
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
