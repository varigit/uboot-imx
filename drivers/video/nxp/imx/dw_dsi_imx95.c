// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
 *
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <dsi_host.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <reset.h>
#include <video.h>
#include <video_bridge.h>
#include <video_link.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <dm/device-internal.h>
#include <linux/iopoll.h>
#include <linux/err.h>
#include <linux/bitfield.h>
#include <phy-mipi-dphy.h>
#include <mux-internal.h>
#include <mux.h>
#include <regmap.h>
#include <syscon.h>
#include <div64.h>

#define DSI_DPI_CFG_POL			0x14
#define  COLORM_ACTIVE_LOW		BIT(4)
#define  SHUTD_ACTIVE_LOW		BIT(3)
#define  HSYNC_ACTIVE_LOW		BIT(2)
#define  VSYNC_ACTIVE_LOW		BIT(1)
#define  DATAEN_ACTIVE_LOW		BIT(0)

#define DSI_PHY_TST_CTRL0		0xb4
#define  PHY_TESTCLK			BIT(1)
#define  PHY_UNTESTCLK			0
#define  PHY_TESTCLR			BIT(0)
#define  PHY_UNTESTCLR			0

#define DSI_PHY_TST_CTRL1		0xb8
#define  PHY_TESTEN			BIT(16)
#define  PHY_UNTESTEN			0
#define  PHY_TESTDOUT_MASK		GENMASK(15, 8)
#define  PHY_TESTDOUT(n)		FIELD_PREP(PHY_TESTDOUT_MASK, (n))
#define  PHY_TESTDIN(n)			FIELD_PREP(GENMASK(7, 0), (n))

/* master CSR */
#define DSI_CLOCK_GATING_CONTROL	0x0
#define  DISPLAY_ASYNC_FIFO(n)		BIT(0 + (n))
#define  DPHY_PLL_CLKIN			BIT(2)
#define  DPHY_PLL_CLKOUT		BIT(3)
#define  DPHY_PLL_CLKEXT		BIT(4)

#define DSI_CLOCK_SETTING		0x8
#define  DPHY_REF_CLOCK_DIV(n)		FIELD_PREP(GENMASK(3, 0), (n))
#define  DPHY_BYPASS_CLOCK		BIT(6)
#define  DPHY_REF_CLOCK_SOURCE		BIT(7)
#define  DPHY_PLL_DIV(n)		FIELD_PREP(GENMASK(11, 8), (n))
#define  PL_SLV_DSI_INGRESS_CLK_SEL(n)	BIT(16 + (n))

/* stream CSR */
#define DSI_HOST_CONFIGURATION		0x0
#define  PIXEL_LINK_FORMAT_MASK		GENMASK(2, 0)
#define  SHUTDOWN			BIT(4)
#define  COLORMODE			BIT(5)

#define DSI_PARITY_ERROR_STATUS		0x4
#define DSI_PARITY_ERROR_THRESHOLD	0x8

/* DPHY CSR */
#define PHY_MODE_CONTROL		0x0
#define  PHY_ENABLE_EXT(n)		FIELD_PREP(GENMASK(7, 4), (n))
#define  PLL_GP_CLK_EN			BIT(3)
#define  PLL_CLKSEL_MASK		GENMASK(2, 1)
#define  PLL_CLKSEL_STOP		FIELD_PREP(PLL_CLKSEL_MASK, 0)
#define  PLL_CLKSEL_GEN			FIELD_PREP(PLL_CLKSEL_MASK, 1)
#define  PLL_CLKSEL_EXT			FIELD_PREP(PLL_CLKSEL_MASK, 2)
#define  TX_RXZ_DSI_MODE		BIT(0)

#define PHY_FREQ_CONTROL		0x4
#define  PHY_HSFREQRANGE(n)		FIELD_PREP(GENMASK(22, 16), (n))
#define  PHY_CFGCLKFREQRANGE(n)		FIELD_PREP(GENMASK(7, 0), (n))

#define PHY_TEST_MODE_CONTROL		0x8
#define	 EXT_SIGNAL_ALL			GENMASK(19, 14)
#define  TURNDISABLE_0			BIT(13)
#define  ENABLECLK_EXT			BIT(12)

#define PHY_TEST_MODE_STATUS		0xc
#define  PHY_LOCK			BIT(11)

/* DPHY test control register */
#define DIG_RDWR_TX_PLL_1		0x15e
#define  PLL_CPBIAS_CNTRL_RW_MASK	GENMASK(6, 0)
#define  PLL_CPBIAS_CNTRL_RW(n)		FIELD_PREP(PLL_CPBIAS_CNTRL_RW_MASK, (n))
#define DIG_RDWR_TX_PLL_5		0x162
#define  PLL_INT_CNTRL_RW(n)		FIELD_PREP(GENMASK(7, 2), (n))
#define  PLL_GMP_CNTRL_RW(n)		FIELD_PREP(GENMASK(1, 0), (n))
#define DIG_RDWR_TX_PLL_9		0x166
#define  PLL_LOCK_SEL_RW		BIT(2)
#define DIG_RDWR_TX_PLL_13		0x16a
#define DIG_RDWR_TX_PLL_17		0x16e
#define  PLL_PWRON_OVR_RW		BIT(7)
#define  PLL_PWRON_OVR_EN_RW		BIT(6)
#define  PLL_PROP_CNTRL_RW_MASK		GENMASK(5, 0)
#define  PLL_PROP_CNTRL_RW(n)		FIELD_PREP(PLL_PROP_CNTRL_RW_MASK, (n))
#define DIG_RDWR_TX_PLL_22		0x173
#define DIG_RDWR_TX_PLL_23		0x174
#define DIG_RDWR_TX_PLL_24		0x175
#define  PLL_TH2_RW(n)			FIELD_PREP(GENMASK(7, 0), (n))
#define DIG_RDWR_TX_PLL_25		0x176
#define  PLL_TH3_RW(n)			FIELD_PREP(GENMASK(7, 0), (n))
#define DIG_RDWR_TX_PLL_27		0x178
#define  PLL_N_OVR_EN_RW		BIT(7)
#define  PLL_N_OVR_RW_MASK		GENMASK(6, 3)
#define  PLL_N_OVR_RW(n)		FIELD_PREP(PLL_N_OVR_RW_MASK, ((n) - 1))
#define DIG_RDWR_TX_PLL_28		0x179
#define  PLL_M_LOW(n)			(((n) - 2) & 0xff)
#define DIG_RDWR_TX_PLL_29		0x17a
#define  PLL_M_HIGH(n)			((((n) - 2) >> 8) & 0xff)
#define DIG_RDWR_TX_PLL_30		0x17b
#define  PLL_VCO_CNTRL_OVR_EN_RW	BIT(7)
#define  PLL_VCO_CNTRL_OVR_RW_MASK	GENMASK(6, 1)
#define  PLL_VCO_CNTRL_OVR_RW(n)	FIELD_PREP(PLL_VCO_CNTRL_OVR_RW_MASK, (n))
#define  PLL_M_OVR_EN_RW		BIT(0)
#define DIG_RDWR_TX_PLL_31		0x17c
#define  PLL_CLKOUTEN_RIGHT_RW		BIT(1)
#define  PLL_CLKOUTEN_LEFT_RW		BIT(0)
#define DIG_RDWR_TX_CB_0		0x1aa
#define DIG_RDWR_TX_CB_1		0x1ab
#define DIG_RDWR_TX_TX_SLEW_0		0x26b
#define DIG_RDWR_TX_TX_SLEW_5		0x270
#define DIG_RDWR_TX_TX_SLEW_6		0x271
#define DIG_RDWR_TX_TX_SLEW_7		0x272
#define DIG_RDWR_TX_CLK_TERMLOWCAP	0x402

#define IMX95_DSI_ENDPOINT_PL0		0
#define IMX95_DSI_ENDPOINT_PL1		1

#define MHZ(x)				((x) * 1000000UL)

#define FOUT_MAX			MHZ(1250)
#define FOUT_MIN			MHZ(40)
#define FVCO_DIV_FACTOR			MHZ(80)

#define MBPS(x)				((x) * 1000000UL)

#define DATA_RATE_MAX_SPEED		MBPS(2500)
#define DATA_RATE_MIN_SPEED		MBPS(80)

#define M_MAX				625UL
#define M_MIN				64UL

#define N_MAX				16U
#define N_MIN				1U

#define MSEC_PER_SEC			1000

enum dsi_pixel_link_format {
	RGB_24BIT,
	RGB_30BIT,
	RGB_18BIT,
	RGB_16BIT,
	YCBCR_20BIT_422,
	YCBCR_16BIT_422,
};

struct imx95_dsi_priv {
	struct mipi_dsi_device device;
	struct udevice *panel;
	struct udevice *dsi_host;

	void __iomem *base;
	struct regmap *mst;
	struct regmap *str;
	struct regmap *phy;
	struct clk clk_pixel;
	struct clk clk_cfg;
	struct clk clk_ref;
	struct mux_control *mux;
	bool use_pl0;

	unsigned long ref_clk_rate;

	struct phy_configure_opts_mipi_dphy phy_cfg;

	unsigned int lane_mbps; /* per lane */
	u32 lanes;
	u32 format;
	struct display_timing adj;
};

struct imx95_dsi_phy_pll_cfg {
	u32 m;	/* PLL Feedback Multiplication Ratio */
	u32 n;	/* PLL Input Frequency Division Ratio */
};

struct imx95_dsi_phy_pll_vco_prop {
	unsigned long max_fout;
	u8 vco_cntl;
	u8 prop_cntl;
};

struct imx95_dsi_phy_pll_hsfreqrange {
	unsigned long max_mbps;
	u8 hsfreqrange;
};

/* DPHY Databook Table 3-12 Charge-pump Programmability */
static const struct imx95_dsi_phy_pll_vco_prop vco_prop_map[] = {
	{   55, 0x3f, 0x0d },
	{   82, 0x39, 0x0d },
	{  110, 0x2f, 0x0d },
	{  165, 0x29, 0x0d },
	{  220, 0x1f, 0x0d },
	{  330, 0x19, 0x0d },
	{  440, 0x0f, 0x0d },
	{  660, 0x09, 0x0d },
	{ 1149, 0x03, 0x0d },
	{ 1152, 0x01, 0x0d },
	{ 1250, 0x01, 0x0e },
};

/* DPHY Databook Table A-4 High-Speed Transition Times */
static const struct imx95_dsi_phy_pll_hsfreqrange hsfreqrange_map[] = {
	{   79, 0x00 },
	{   89, 0x10 },
	{   99, 0x20 },
	{  109, 0x30 },
	{  119, 0x01 },
	{  129, 0x11 },
	{  139, 0x21 },
	{  149, 0x31 },
	{  159, 0x02 },
	{  169, 0x12 },
	{  179, 0x22 },
	{  189, 0x32 },
	{  204, 0x03 },
	{  219, 0x13 },
	{  234, 0x23 },
	{  249, 0x33 },
	{  274, 0x04 },
	{  299, 0x14 },
	{  324, 0x25 },
	{  349, 0x35 },
	{  399, 0x05 },
	{  449, 0x16 },
	{  499, 0x26 },
	{  549, 0x37 },
	{  599, 0x07 },
	{  649, 0x18 },
	{  699, 0x28 },
	{  749, 0x39 },
	{  799, 0x09 },
	{  849, 0x19 },
	{  899, 0x29 },
	{  949, 0x3a },
	{  999, 0x0a },
	{ 1049, 0x1a },
	{ 1099, 0x2a },
	{ 1149, 0x3b },
	{ 1199, 0x0b },
	{ 1249, 0x1b },
	{ 1299, 0x2b },
	{ 1349, 0x3c },
	{ 1399, 0x0c },
	{ 1449, 0x1c },
	{ 1499, 0x2c },
	{ 1549, 0x3d },
	{ 1599, 0x0d },
	{ 1649, 0x1d },
	{ 1699, 0x2e },
	{ 1749, 0x3e },
	{ 1779, 0x0e },
	{ 1849, 0x1e },
	{ 1899, 0x2f },
	{ 1949, 0x3f },
	{ 1999, 0x0f },
	{ 2049, 0x40 },
	{ 2099, 0x41 },
	{ 2149, 0x42 },
	{ 2199, 0x43 },
	{ 2249, 0x44 },
	{ 2299, 0x45 },
	{ 2349, 0x46 },
	{ 2399, 0x47 },
	{ 2449, 0x48 },
	{ 2500, 0x49 },
};

static void
imx95_dsi_phy_csr_write(struct imx95_dsi_priv *dsi, unsigned int reg, u32 val)
{
	int ret;
	struct mipi_dsi_device *device = &dsi->device;
	struct udevice *dev = device->dev;

	ret = regmap_write(dsi->phy, reg, val);
	if (ret < 0)
		dev_err(dev, "failed to write 0x%08x to phy reg 0x%x: %d\n",
			val, reg, ret);
}

static inline void imx95_dsi_write(struct imx95_dsi_priv *dsi, u32 reg, u32 val)
{
	writel(val, dsi->base + reg);
}

static inline u32 imx95_dsi_read(struct imx95_dsi_priv *dsi, u32 reg)
{
	return readl(dsi->base + reg);
}

static void imx95_dsi_phy_write_testcode(struct imx95_dsi_priv *dsi, u16 test_code)
{
	/*
	 * Each control register address is composed by a 4-bit word(testcode
	 * MSBs) and an 8-bit word(testcode LSBs).
	 */

	/* 1. For writing the 4-bit testcode MSBs: */
	/* a. Ensure that TESTCLK and TESTEN are set to low. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_UNTESTCLK);
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL1, PHY_UNTESTEN);
	/* b. Set TESTEN to high. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL1, PHY_TESTEN);
	/* c. Set TESTCLK to high. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_TESTCLK);
	/* d. Place 0x00 in TESTDIN. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL1, PHY_TESTEN | PHY_TESTDIN(0));
	/*
	 * e. Set TESTCLK to low(with the falling edge on TESTCLK, the TESTDIN
	 * signal content is latched internally).
	 */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_UNTESTCLK);
	/* f. Set TESTEN to low. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL1, PHY_UNTESTEN);
	/* g. Place the MSB 8-bit word of testcode in TESTDIN. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL1, PHY_TESTDIN(test_code >> 8));
	/* h. Set TESTCLK to high. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_TESTCLK);

	/* 2. For writing the 8-bit testcode LSBs: */
	/* a. Set TESTCLK to low. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_UNTESTCLK);
	/* b. Set TESTEN to high. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL1, PHY_TESTEN);
	/* c. Set TESTCLK to high. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_TESTCLK);
	/* d. Place the LSB 8-bit word of testcode in TESTDIN. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL1,
			PHY_TESTEN | PHY_TESTDIN(test_code & 0xff));
	/*
	 * e. Set TESTCLK to low(with the falling edge on TESTCLK, the TESTDIN
	 * signal content is latched internally).
	 */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_UNTESTCLK);
	/* f. Set TESTEN to low. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL1,
			PHY_UNTESTEN | PHY_TESTDIN(test_code & 0xff));
}

static void
imx95_dsi_phy_tst_ctrl_write(struct imx95_dsi_priv *dsi, u16 test_code, u8 test_data)
{
	imx95_dsi_phy_write_testcode(dsi, test_code);

	/* For writing the data: */
	/* a. Place the 8-bit word in TESTDIN. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL1, PHY_TESTDIN(test_data));
	/* b. Set TESTCLK to high. */
	imx95_dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_TESTCLK);
}

static u8 imx95_dsi_phy_tst_ctrl_read(struct imx95_dsi_priv *dsi, u16 test_code)
{
	u32 val;

	imx95_dsi_phy_write_testcode(dsi, test_code);

	/* For reading the data: */
	val = imx95_dsi_read(dsi, DSI_PHY_TST_CTRL1);
	return FIELD_GET(PHY_TESTDOUT_MASK, val);
}

static void
imx95_dsi_phy_tst_ctrl_update(struct imx95_dsi_priv *dsi, u16 test_code, u8 mask, u8 val)
{
	u8 tmp;

	tmp = imx95_dsi_phy_tst_ctrl_read(dsi, test_code);
	tmp &= ~mask;
	imx95_dsi_phy_tst_ctrl_write(dsi, test_code, tmp | val);
}

static inline unsigned long data_rate_to_fout(unsigned long data_rate)
{
	/* Fout is half of data rate */
	return data_rate / 2;
}

static int
imx95_dsi_phy_pll_get_configure_from_opts(struct imx95_dsi_priv *dsi,
					  struct phy_configure_opts_mipi_dphy *dphy_opts,
					  struct imx95_dsi_phy_pll_cfg *cfg)
{
	struct mipi_dsi_device *device = &dsi->device;
	struct udevice *dev = device->dev;
	unsigned long fin = dsi->ref_clk_rate;
	unsigned long fout;
	unsigned long best_fout = 0;
	unsigned int fvco_div;
	unsigned int min_n, max_n, n, best_n;
	unsigned long m, best_m;
	unsigned long min_delta = ULONG_MAX;
	unsigned long delta;
	u64 tmp;

	if (dphy_opts->hs_clk_rate < DATA_RATE_MIN_SPEED ||
	    dphy_opts->hs_clk_rate > DATA_RATE_MAX_SPEED) {
		dev_dbg(dev, "invalid data rate per lane: %lu\n",
			dphy_opts->hs_clk_rate);
		return -EINVAL;
	}

	dev_dbg(dev, "data rate per lane: %lu\n",
			dphy_opts->hs_clk_rate);

	fout = data_rate_to_fout(dphy_opts->hs_clk_rate);

	/* DPHY Databook 3.3.6.1 Output Frequency */
	/* Fout = Fvco / Fvco_div = (Fin * M) / (Fvco_div * N) */
	/* Fvco_div could be 1/2/4/8 according to Fout range. */
	fvco_div = 8UL / min(DIV_ROUND_UP(fout, FVCO_DIV_FACTOR), 8UL);

	/* limitation: 2MHz <= Fin / N <= 8MHz */
	min_n = DIV_ROUND_UP_ULL((u64)fin, MHZ(8));
	max_n = DIV_ROUND_DOWN_ULL((u64)fin, MHZ(2));

	/* clamp possible N(s) */
	min_n = clamp(min_n, N_MIN, N_MAX);
	max_n = clamp(max_n, N_MIN, N_MAX);

	dev_dbg(dev, "Fout = %lu, Fvco_div = %u, n_range = [%u, %u]\n",
		fout, fvco_div, min_n, max_n);

	for (n = min_n; n <= max_n; n++) {
		/* M = (Fout * N * Fvco_div) / Fin */
		m = DIV_ROUND_CLOSEST(fout * n * fvco_div, fin);

		/* check M range */
		if (m < M_MIN || m > M_MAX)
			continue;

		/* calculate temporary Fout */
		tmp = m * fin;
		do_div(tmp, n * fvco_div);
		if (tmp < FOUT_MIN || tmp > FOUT_MAX)
			continue;

		delta = abs(fout - tmp);
		if (delta < min_delta) {
			best_n = n;
			best_m = m;
			min_delta = delta;
			best_fout = tmp;
		}
	}

	if (best_fout) {
		cfg->m = best_m;
		cfg->n = best_n;
		dev_dbg(dev, "best Fout = %lu, m = %u, n = %u\n",
			best_fout, cfg->m, cfg->n);
	} else {
		dev_dbg(dev, "failed to find best Fout\n");
		return -EINVAL;
	}

	return 0;
}

static u8
imx95_dsi_phy_pll_get_hsfreqrange(struct phy_configure_opts_mipi_dphy *dphy_opts)
{
	unsigned long mbps = dphy_opts->hs_clk_rate / MHZ(1);
	int i;

	for (i = 0; i < ARRAY_SIZE(hsfreqrange_map); i++)
		if (mbps <= hsfreqrange_map[i].max_mbps)
			return hsfreqrange_map[i].hsfreqrange;

	return 0;
}

static unsigned long imx95_dsi_phy_pll_get_cfgclkrange(struct imx95_dsi_priv *dsi)
{
	/*
	 * DPHY Databook Table 4-5 System Control Signals mentions an equation
	 * for cfgclkfreqrange[5:0].
	 */
	return (clk_get_rate(&dsi->clk_cfg) / MHZ(1) - 17) * 4;
}

static u8
imx95_dsi_phy_pll_get_vco(struct phy_configure_opts_mipi_dphy *dphy_opts)
{
	unsigned long fout = data_rate_to_fout(dphy_opts->hs_clk_rate) / MHZ(1);
	int i;

	for (i = 0; i < ARRAY_SIZE(vco_prop_map); i++)
		if (fout <= vco_prop_map[i].max_fout)
			return vco_prop_map[i].vco_cntl;

	return 0;
}

static u8
imx95_dsi_phy_pll_get_prop(struct phy_configure_opts_mipi_dphy *dphy_opts)
{
	unsigned long fout = data_rate_to_fout(dphy_opts->hs_clk_rate) / MHZ(1);
	int i;

	for (i = 0; i < ARRAY_SIZE(vco_prop_map); i++)
		if (fout <= vco_prop_map[i].max_fout)
			return vco_prop_map[i].prop_cntl;

	return 0;
}

/* Dump PHY PLL observability, could check after dw_mipi_dsi_dphy_enable */
void imx95_dsi_phy_pll_observability(struct imx95_dsi_priv *dsi)
{
	int i;
	u8 test;
	for (i = 0; i < 5; i++){
		test = imx95_dsi_phy_tst_ctrl_read(dsi, 0x191 + i);
		debug("0x%x 0x%x\n", 0x191 + i, test);
	}
}

static int imx95_dsi_phy_pll_configure(struct imx95_dsi_priv *dsi)
{
	struct imx95_dsi_phy_pll_cfg cfg = { 0 };
	struct mipi_dsi_device *device = &dsi->device;
	struct udevice *dev = device->dev;
	u32 val;
	int ret;

	ret = imx95_dsi_phy_pll_get_configure_from_opts(dsi, &dsi->phy_cfg,
							&cfg);
	if (ret) {
		dev_err(dev, "failed to get phy pll cfg %d\n", ret);
		return ret;
	}

	val = PHY_CFGCLKFREQRANGE(imx95_dsi_phy_pll_get_cfgclkrange(dsi)) |
	      PHY_HSFREQRANGE(imx95_dsi_phy_pll_get_hsfreqrange(&dsi->phy_cfg));
	imx95_dsi_phy_csr_write(dsi, PHY_FREQ_CONTROL, val);

	/* set pll_mpll_prog_rw (bits1:0) to 2'b11 */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_PLL_13, 0x03);
	/* set cb_sel_vref_lprx_rw (bits 1:0) to 2'b10 */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_CB_1, 0x06);
	/* set cb_sel_vrefcd_lprx_rw (bits 6:5) to 2'b10 */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_CB_0, 0x53);
	/*
	 * set txclk_term_lowcap_lp00_en_ovr_en and
	 * txclk_term_lowcap_lp00_en_ovr(bits 1:0) to 2'b10
	 */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_CLK_TERMLOWCAP, 0x02);
	/* bit [5:4] of TX control register with address 0x272 to 2'b00 */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_TX_SLEW_7, 0x00);
	/* sr_osc_freq_target */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_TX_SLEW_6, 0x07);
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_TX_SLEW_5, 0xd0);
	/* enable slew rate calibration */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_TX_SLEW_7, 0x10);

	ret = clk_prepare_enable(&dsi->clk_cfg);
	if (ret < 0) {
		dev_err(dev, "failed to enable cfg clock: %d\n", ret);
		return ret;
	}

	/* pll m */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_PLL_28,
				     PLL_M_LOW(cfg.m));
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_PLL_29,
				     PLL_M_HIGH(cfg.m));
	imx95_dsi_phy_tst_ctrl_update(dsi, DIG_RDWR_TX_PLL_30,
				      PLL_M_OVR_EN_RW, PLL_M_OVR_EN_RW);

	/* pll n */
	imx95_dsi_phy_tst_ctrl_update(dsi, DIG_RDWR_TX_PLL_27,
				      PLL_N_OVR_RW_MASK | PLL_N_OVR_EN_RW,
				      PLL_N_OVR_RW(cfg.n) | PLL_N_OVR_EN_RW);

	/* vco ctrl */
	val = imx95_dsi_phy_pll_get_vco(&dsi->phy_cfg);
	dev_dbg(dev, "vco ctrl value: 0x%x\n", val);
	imx95_dsi_phy_tst_ctrl_update(dsi, DIG_RDWR_TX_PLL_30,
				      PLL_VCO_CNTRL_OVR_RW_MASK |
				      PLL_VCO_CNTRL_OVR_EN_RW,
				      PLL_VCO_CNTRL_OVR_RW(val) |
				      PLL_VCO_CNTRL_OVR_EN_RW);

	/* cpbias ctrl */
	imx95_dsi_phy_tst_ctrl_update(dsi, DIG_RDWR_TX_PLL_1,
				      PLL_CPBIAS_CNTRL_RW_MASK,
				      PLL_CPBIAS_CNTRL_RW(0x10));

	/* int ctrl & gmp ctrl */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_PLL_5,
				     PLL_INT_CNTRL_RW(0x0) |
				     PLL_GMP_CNTRL_RW(0x1));

	/* prop ctrl */
	val = imx95_dsi_phy_pll_get_prop(&dsi->phy_cfg);
	dev_dbg(dev, "prop ctrl value: 0x%x\n", val);
	imx95_dsi_phy_tst_ctrl_update(dsi, DIG_RDWR_TX_PLL_17,
				      PLL_PROP_CNTRL_RW_MASK,
				      PLL_PROP_CNTRL_RW(val));

	/* pll lock configurations */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_PLL_22, 0x2);	/* TH1 */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_PLL_23, 0x0);	/* TH1 */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_PLL_24,
				     PLL_TH2_RW(0x60));
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_PLL_25,
				     PLL_TH3_RW(0x3));
	imx95_dsi_phy_tst_ctrl_update(dsi, DIG_RDWR_TX_PLL_9,
				      PLL_LOCK_SEL_RW, PLL_LOCK_SEL_RW);

	/* pll power on */
	imx95_dsi_phy_tst_ctrl_update(dsi, DIG_RDWR_TX_PLL_17,
				      PLL_PWRON_OVR_RW | PLL_PWRON_OVR_EN_RW,
				      PLL_PWRON_OVR_RW | PLL_PWRON_OVR_EN_RW);

	/* pll clkouten right/left */
	imx95_dsi_phy_tst_ctrl_write(dsi, DIG_RDWR_TX_PLL_31,
				     PLL_CLKOUTEN_RIGHT_RW |
				     PLL_CLKOUTEN_LEFT_RW);
	return 0;
}

static int imx95_dsi_phy_init(void *priv_data)
{
	struct imx95_dsi_priv *dsi = priv_data;
	struct mipi_dsi_device *device = &dsi->device;
	struct udevice *dev = device->dev;
	u64 lane_mask;
	int bpp, ret;

	ret = mux_control_try_select(dsi->mux, !dsi->use_pl0);
	if (ret < 0) {
		dev_err(dev, "failed to select input: %d\n", ret);
		return ret;
	}

	bpp = mipi_dsi_pixel_format_to_bpp(dsi->format);
	if (bpp < 0) {
		dev_err(dev, "failed to get dsi format bpp\n");
		return bpp;
	}

	imx95_dsi_write(dsi, DSI_DPI_CFG_POL, VSYNC_ACTIVE_LOW | HSYNC_ACTIVE_LOW);

	regmap_write(dsi->mst, DSI_CLOCK_SETTING, 0);

	debug("dsi bpp %u, lane %u\n", bpp, dsi->lanes);

	switch (bpp) {
	case 24:
		regmap_write(dsi->str, DSI_HOST_CONFIGURATION, RGB_24BIT);
		break;
	case 18:
		regmap_write(dsi->str, DSI_HOST_CONFIGURATION, RGB_18BIT);
		break;
	case 16:
		regmap_write(dsi->str, DSI_HOST_CONFIGURATION, RGB_16BIT);
		break;
	default:
		dev_err(dev, "invalid dsi format bpp %d\n", bpp);
		return -EINVAL;
	}

	lane_mask = (1 << dsi->lanes) - 1;
	imx95_dsi_phy_csr_write(dsi, PHY_MODE_CONTROL,
				PHY_ENABLE_EXT(lane_mask) | PLL_CLKSEL_GEN |
				TX_RXZ_DSI_MODE);

	ret = imx95_dsi_phy_pll_configure(dsi);
	if (ret < 0) {
		dev_err(dev, "failed to configure phy pll: %d\n", ret);
		return ret;
	}

	imx95_dsi_phy_csr_write(dsi, PHY_TEST_MODE_CONTROL, TURNDISABLE_0);

	regmap_write(dsi->mst, DSI_CLOCK_GATING_CONTROL,
		     DISPLAY_ASYNC_FIFO(dsi->use_pl0));

	return 0;
}

static void imx95_dsi_phy_power_off(void *priv_data)
{
	struct imx95_dsi_priv *dsi = priv_data;

	/* set 1 to disable */
	regmap_write(dsi->mst, DSI_CLOCK_GATING_CONTROL,
		     DPHY_PLL_CLKEXT | DPHY_PLL_CLKOUT | DPHY_PLL_CLKIN |
		     DISPLAY_ASYNC_FIFO(0) | DISPLAY_ASYNC_FIFO(1));

	imx95_dsi_phy_csr_write(dsi, PHY_MODE_CONTROL, TX_RXZ_DSI_MODE);

	clk_disable_unprepare(&dsi->clk_cfg);
}

static int
imx95_dsi_phy_get_lane_mbps(void *priv_data, struct display_timing *timings,
								 u32 lanes, u32 format, unsigned int *lane_mbps)
{
	struct imx95_dsi_priv *dsi = priv_data;
	int bpp;
	int ret;

	bpp = mipi_dsi_pixel_format_to_bpp(format);
	if (bpp < 0) {
		dev_dbg(dsi->device.dev,
			      "failed to get bpp for pixel format %d\n",
			      format);
		return bpp;
	}

	dsi->lane_mbps = DIV_ROUND_UP((timings->pixelclock.typ / 1000) * (bpp / lanes), MSEC_PER_SEC);
	*lane_mbps = dsi->lane_mbps;

	debug("lane_mbps %u, bpp %d\n", *lane_mbps, bpp);

	ret = phy_mipi_dphy_get_default_config(timings->pixelclock.typ,
					       bpp, lanes,
					       &dsi->phy_cfg);
	if (ret < 0) {
		dev_dbg(dsi->device.dev, "failed to get default phy cfg %d\n", ret);
		return ret;
	}

	return 0;
}

struct hstt {
	unsigned int maxfreq;
	struct mipi_dsi_phy_timing timing;
};

#define HSTT(_maxfreq, _c_lp2hs, _c_hs2lp, _d_lp2hs, _d_hs2lp)	\
{								\
	.maxfreq = (_maxfreq),					\
	.timing = {						\
		.clk_lp2hs = (_c_lp2hs),			\
		.clk_hs2lp = (_c_hs2lp),			\
		.data_lp2hs = (_d_lp2hs),			\
		.data_hs2lp = (_d_hs2lp),			\
	}							\
}

/* Table A-4 High-Speed Transition Times */
struct hstt hstt_table[] = {
	HSTT(80,   21,  17,  15, 10),
	HSTT(90,   23,  17,  16, 10),
	HSTT(100,  22,  17,  16, 10),
	HSTT(110,  25,  18,  17, 11),
	HSTT(120,  26,  20,  18, 11),
	HSTT(130,  27,  19,  19, 11),
	HSTT(140,  27,  19,  19, 11),
	HSTT(150,  28,  20,  20, 12),
	HSTT(160,  30,  21,  22, 13),
	HSTT(170,  30,  21,  23, 13),
	HSTT(180,  31,  21,  23, 13),
	HSTT(190,  32,  22,  24, 13),
	HSTT(205,  35,  22,  25, 13),
	HSTT(220,  37,  26,  27, 15),
	HSTT(235,  38,  28,  27, 16),
	HSTT(250,  41,  29,  30, 17),
	HSTT(275,  43,  29,  32, 18),
	HSTT(300,  45,  32,  35, 19),
	HSTT(325,  48,  33,  36, 18),
	HSTT(350,  51,  35,  40, 20),
	HSTT(400,  59,  37,  44, 21),
	HSTT(450,  65,  40,  49, 23),
	HSTT(500,  71,  41,  54, 24),
	HSTT(550,  77,  44,  57, 26),
	HSTT(600,  82,  46,  64, 27),
	HSTT(650,  87,  48,  67, 28),
	HSTT(700,  94,  52,  71, 29),
	HSTT(750,  99,  52,  75, 31),
	HSTT(800, 105,  55,  82, 32),
	HSTT(850, 110,  58,  85, 32),
	HSTT(900, 115,  58,  88, 35),
	HSTT(950, 120,  62,  93, 36),
	HSTT(1000, 128,  63,  99, 38),
	HSTT(1050, 132,  65, 102, 38),
	HSTT(1100, 138,  67, 106, 39),
	HSTT(1150, 146,  69, 112, 42),
	HSTT(1200, 151,  71, 117, 43),
	HSTT(1250, 153,  74, 120, 45),
	HSTT(1300, 160,  73, 124, 46),
	HSTT(1350, 165,  76, 130, 47),
	HSTT(1400, 172,  78, 134, 49),
	HSTT(1450, 177,  80, 138, 49),
	HSTT(1500, 183,  81, 143, 52),
	HSTT(1550, 191,  84, 147, 52),
	HSTT(1600, 194,  85, 152, 52),
	HSTT(1650, 201,  86, 155, 53),
	HSTT(1700, 208,  88, 161, 53),
	HSTT(1750, 212,  89, 165, 53),
	HSTT(1800, 220,  90, 171, 54),
	HSTT(1850, 223,  92, 175, 54),
	HSTT(1900, 231,  91, 180, 55),
	HSTT(1950, 236,  95, 185, 56),
	HSTT(2000, 243,  97, 190, 56),
	HSTT(2050, 248,  99, 194, 58),
	HSTT(2100, 252, 100, 199, 59),
	HSTT(2150, 259, 102, 204, 61),
	HSTT(2200, 266, 105, 210, 62),
	HSTT(2250, 269, 109, 213, 63),
	HSTT(2300, 272, 109, 217, 65),
	HSTT(2350, 281, 112, 225, 66),
	HSTT(2400, 283, 115, 226, 66),
	HSTT(2450, 282, 115, 226, 67),
	HSTT(2500, 281, 118, 227, 67),
};

static int
imx95_dsi_phy_get_timing(void *priv_data, unsigned int lane_mbps,
			   struct mipi_dsi_phy_timing *timing)
{
	struct imx95_dsi_priv *dsi = priv_data;
	int i;

	for (i = 0; i < ARRAY_SIZE(hstt_table); i++)
		if (lane_mbps <= hstt_table[i].maxfreq)
			break;

	if (i == ARRAY_SIZE(hstt_table))
		i--;

	*timing = hstt_table[i].timing;

	dev_dbg(dsi->device.dev, "get phy timing for %u <= %u (lane_mbps)\n",
		      lane_mbps, hstt_table[i].maxfreq);

	return 0;
}

static const struct mipi_dsi_phy_ops imx95_dsi_phy_ops = {
	.init = imx95_dsi_phy_init,
	.get_lane_mbps = imx95_dsi_phy_get_lane_mbps,
	.get_timing = imx95_dsi_phy_get_timing,
};

static int imx95_dsi_attach(struct udevice *dev)
{
	struct imx95_dsi_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	struct mipi_dsi_panel_plat *mplat;
	struct display_timing timings;
	int ret, bpp;

	debug("%s\n", __func__);

	priv->panel = video_link_get_next_device(dev);
	if (!priv->panel ||
		device_get_uclass_id(priv->panel) != UCLASS_PANEL) {
		dev_err(dev, "get panel device error\n");
		return -ENODEV;
	}

	mplat = dev_get_plat(priv->panel);
	mplat->device = &priv->device;

	ret = video_link_get_display_timings(&timings);
	if (ret) {
		dev_err(dev, "decode display timing error %d\n", ret);
		return ret;
	}

	bpp = mipi_dsi_pixel_format_to_bpp(device->format);
	if (bpp < 0) {
		dev_err(dev, "failed to get bpp for pixel format %d\n", device->format);
		return bpp;
	}

	priv->lanes = device->lanes;
	priv->format = device->format;

	priv->adj = timings;
	ret = uclass_get_device(UCLASS_DSI_HOST, 0, &priv->dsi_host);
	if (ret) {
		dev_err(dev, "No video dsi host detected %d\n", ret);
		return ret;
	}

	ret = dsi_host_init(priv->dsi_host, device, &priv->adj,
			4,
			&imx95_dsi_phy_ops);
	if (ret) {
		dev_err(dev, "failed to initialize mipi dsi host\n");
		return ret;
	}

	return 0;
}

static int imx95_dsi_set_backlight(struct udevice *dev, int percent)
{
	struct imx95_dsi_priv *priv = dev_get_priv(dev);
	int ret;

	ret = panel_enable_backlight(priv->panel);
	if (ret) {
		dev_err(dev, "panel %s enable backlight error %d\n",
			priv->panel->name, ret);
		return ret;
	}

	ret = dsi_host_enable(priv->dsi_host);
	if (ret) {
		dev_err(dev, "failed to enable mipi dsi host\n");
		return ret;
	}

	return 0;
}

static int imx95_dsi_check_timing(struct udevice *dev, struct display_timing *timing)
{
	struct imx95_dsi_priv *priv = dev_get_priv(dev);

	/* Ensure the bridge device attached to panel */
	if (!priv->panel) {
		dev_err(dev, "%s No panel device attached\n", __func__);
		return -ENOTCONN;
	}

	/* DSI force the Polarities as high */
	priv->adj.flags &= ~(DISPLAY_FLAGS_HSYNC_HIGH | DISPLAY_FLAGS_VSYNC_HIGH);
	priv->adj.flags |= DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW;

	*timing = priv->adj;

	return 0;
}

static int imx95_dsi_get_clk(struct imx95_dsi_priv *dsi)
{
	struct mipi_dsi_device *device = &dsi->device;
	struct udevice *dev = device->dev;
	int ret;

	ret = clk_get_by_name(dev, "pix", &dsi->clk_pixel);
	if (ret) {
		dev_err(dev, "failed to get pixel clk %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "cfg", &dsi->clk_cfg);
	if (ret) {
		dev_err(dev, "failed to get cfg clk %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "ref", &dsi->clk_ref);
	if (ret) {
		dev_err(dev, "failed to get ref clk %d\n", ret);
		return ret;
	}

	dsi->ref_clk_rate = clk_get_rate(&dsi->clk_ref);
	dev_dbg(dev, "phy ref clock rate: %lu\n", dsi->ref_clk_rate);

	return 0;
}

static int imx95_dsi_get_regmap(struct imx95_dsi_priv *dsi)
{
	struct mipi_dsi_device *device = &dsi->device;
	struct udevice *dev = device->dev;

	dsi->mst = syscon_regmap_lookup_by_phandle(dev, "nxp,disp-master-csr");
	if (IS_ERR(dsi->mst)) {
		dev_err(dev, "failed to get master CSR\n");
		return PTR_ERR(dsi->mst);
	}

	dsi->str = syscon_regmap_lookup_by_phandle(dev, "nxp,disp-stream-csr");
	if (IS_ERR(dsi->str)) {
		dev_err(dev, "failed to get stream CSR\n");
		return PTR_ERR(dsi->str);
	}

	dsi->phy = syscon_regmap_lookup_by_phandle(dev, "nxp,mipi-combo-phy-csr");
	if (IS_ERR(dsi->phy)) {
		dev_err(dev, "failed to get phy CSR\n");
		return PTR_ERR(dsi->phy);
	}

	return 0;
}

static int imx95_dsi_get_mux(struct imx95_dsi_priv *dsi)
{
	struct mipi_dsi_device *device = &dsi->device;
	struct udevice *dev = device->dev;
	int ret;

	ret = mux_get_by_index(dev, 0, &dsi->mux);
	if (ret) {
		dev_err(dev, "failed to get mux %d\n", ret);
		return ret;
	}

	return 0;
}

static int imx95_dsi_select_input(struct imx95_dsi_priv *dsi)
{
	struct mipi_dsi_device *device = &dsi->device;
	struct udevice *dev = device->dev;
	int ret;
	ofnode pl_dev_ep, dsi_ep;
	u32 phandle, reg;

	pl_dev_ep = video_link_get_ep_to_nextdev(dev);
	if (!ofnode_valid(pl_dev_ep)) {
		dev_err(dev, "failed to get pixel-link endpoint\n");
		return -ENODEV;
	}

	ret = ofnode_read_u32(pl_dev_ep, "remote-endpoint", &phandle);
	if (ret) {
		dev_err(dev, "required remote-endpoint property isn't provided\n");
		return -ENODEV;
	}

	dsi_ep = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(dsi_ep)) {
		dev_err(dev, "failed to find remote-endpoint\n");
		return -ENODEV;
	}

	reg = (u32)ofnode_get_addr_size_index_notrans(dsi_ep, 0, NULL);
	dsi->use_pl0 = reg ? false: true;

	return ret;
}

static void imx95_dsi_deselect_input(struct imx95_dsi_priv *dsi)
{
	struct mipi_dsi_device *device = &dsi->device;
	struct udevice *dev = device->dev;
	int ret;

	ret = mux_control_deselect(dsi->mux);
	if (ret < 0)
		dev_err(dev, "failed to deselect input: %d\n", ret);
}

static int imx95_dsi_probe(struct udevice *dev)
{
	struct imx95_dsi_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	int ret;

	device->dev = dev;

	priv->base = (void *)dev_read_addr(dev);
	if ((fdt_addr_t)priv->base == FDT_ADDR_T_NONE) {
		dev_err(dev, "dsi dt register address error\n");
		return -EINVAL;
	}

	ret = imx95_dsi_get_clk(priv);
	if (ret)
		return ret;

	ret = imx95_dsi_get_regmap(priv);
	if (ret)
		return ret;

	ret = imx95_dsi_get_mux(priv);
	if (ret)
		return ret;

	ret = imx95_dsi_select_input(priv);
	if (ret)
		return ret;

	return ret;
}

static int imx95_dsi_remove(struct udevice *dev)
{
	struct imx95_dsi_priv *priv = dev_get_priv(dev);
	int ret;

	if (priv->panel)
		device_remove(priv->panel, DM_REMOVE_NORMAL);

	ret = dsi_host_disable(priv->dsi_host);
	if (ret < 0 && ret != -ENOSYS)
		dev_err(dev, "failed to disable mipi dsi host\n");

	imx95_dsi_phy_power_off(priv);

	imx95_dsi_deselect_input(priv);

	return 0;
}

struct video_bridge_ops imx95_dsi_ops = {
	.attach = imx95_dsi_attach,
	.set_backlight = imx95_dsi_set_backlight,
	.check_timing = imx95_dsi_check_timing,
};

static const struct udevice_id imx95_dsi_ids[] = {
	{ .compatible = "nxp,imx95-mipi-dsi" },
	{ }
};

U_BOOT_DRIVER(dw_dsi_imx95) = {
	.name				= "dw_dsi_imx95",
	.id				= UCLASS_VIDEO_BRIDGE,
	.of_match			= imx95_dsi_ids,
	.bind				= dm_scan_fdt_dev,
	.remove				= imx95_dsi_remove,
	.probe				= imx95_dsi_probe,
	.ops				= &imx95_dsi_ops,
	.priv_auto			= sizeof(struct imx95_dsi_priv),
};
