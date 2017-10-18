/*
 * Copyright (C) 2016 Variscite Ltd.
 *
 * Setup "Legacy" DRAM parametes
 *
 * For non-DART boards
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifdef CONFIG_SPL_BUILD
#include <common.h>
#include <asm/arch/mx6-ddr.h>
#include <asm/arch/sys_proto.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/io.h>

u32 get_cpu_speed_grade_hz(void);
void var_set_ram_size(u32 ram_size);


static int wait_for_bit(volatile void *reg, const uint32_t mask, bool set)
{
	unsigned int timeout = 1000;
	u32 val;

	while (--timeout) {
		val = readl(reg);
		if (!set)
			val = ~val;

		if ((val & mask) == mask)
			return 0;

		udelay(1);
	}

	printf("%s: Timeout (reg=%p mask=%08x wait_set=%i)\n",
			__func__, reg, mask, set);
	printf("DRAM couldn't be calibrated\n");
	hang();
}

static void reset_ddr_solo(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;

	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;

	/* Reset data FIFOs twice */
	setbits_le32(&mmdc_p0->mpdgctrl0, 1 << 31);
	wait_for_bit(&mmdc_p0->mpdgctrl0, 1 << 31, 0);

	setbits_le32(&mmdc_p0->mpdgctrl0, 1 << 31);
	wait_for_bit(&mmdc_p0->mpdgctrl0, 1 << 31, 0);

	mmdc_p0->mdmisc = 2;
	mmdc_p0->mdscr = (u32)0x00008000;

	wait_for_bit(&mmdc_p0->mdscr, 1 << 14, 1);
}

static void spl_dram_init_mx6solo_512mb(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;

	/* ZQ */
	mmdc_p0->mpzqhwctrl     = (u32)0xa1390003;
	/* Write leveling */
	mmdc_p0->mpwldectrl0    = (u32)0x001F001F;
	mmdc_p0->mpwldectrl1    = (u32)0x001F001F;

	mmdc_p0->mpdgctrl0      = (u32)0x421C0216;
	mmdc_p0->mpdgctrl1      = (u32)0x017B017A;

	mmdc_p0->mprddlctl      = (u32)0x4B4A4E4C;
	mmdc_p0->mpwrdlctl      = (u32)0x38363236;
	/* Read data bit delay */
	mmdc_p0->mprddqby0dl    = (u32)0x33333333;
	mmdc_p0->mprddqby1dl    = (u32)0x33333333;
	mmdc_p0->mprddqby2dl    = (u32)0x33333333;
	mmdc_p0->mprddqby3dl    = (u32)0x33333333;

	/* Complete calibration by forced measurement */
	mmdc_p0->mpmur0         = (u32)0x00000800;
	mmdc_p0->mdpdc          = (u32)0x00020025;
	mmdc_p0->mdotc          = (u32)0x00333030;
	mmdc_p0->mdcfg0         = (u32)0x676B5313;
	mmdc_p0->mdcfg1         = (u32)0xB66E8B63;
	mmdc_p0->mdcfg2         = (u32)0x01ff00db;
	mmdc_p0->mdmisc         = (u32)0x00001740;

	mmdc_p0->mdscr          = (u32)0x00008000;
	mmdc_p0->mdrwd          = (u32)0x000026d2;
	mmdc_p0->mdor           = (u32)0x006B1023;
	mmdc_p0->mdasp          = (u32)0x00000017;

	mmdc_p0->mdctl          = (u32)0x84190000;

	mmdc_p0->mdscr          = (u32)0x04008032;
	mmdc_p0->mdscr          = (u32)0x00008033;
	mmdc_p0->mdscr          = (u32)0x00048031;
	mmdc_p0->mdscr          = (u32)0x07208030;
	mmdc_p0->mdscr          = (u32)0x04008040;

	mmdc_p0->mdref          = (u32)0x00005800;
	mmdc_p0->mpodtctrl      = (u32)0x00011117;

	mmdc_p0->mdpdc          = (u32)0x00025565;
	mmdc_p0->mapsr          = (u32)0x00011006;
	mmdc_p0->mdscr          = (u32)0x00000000;
}

static void spl_mx6dqp_dram_setup_iomux(void)
{
	volatile struct mx6dq_iomux_ddr_regs *mx6q_ddr_iomux;
	volatile struct mx6dq_iomux_grp_regs *mx6q_grp_iomux;

	mx6q_ddr_iomux = (struct mx6dq_iomux_ddr_regs *) MX6DQ_IOM_DDR_BASE;
	mx6q_grp_iomux = (struct mx6dq_iomux_grp_regs *) MX6DQ_IOM_GRP_BASE;

	mx6q_grp_iomux->grp_ddr_type	= (u32)0x000c0000;
	mx6q_grp_iomux->grp_ddrpke	= (u32)0x00000000;
	mx6q_ddr_iomux->dram_sdclk_0	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdclk_1	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_cas	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_ras	= (u32)0x00000030;
	mx6q_grp_iomux->grp_addds	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_reset	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdcke0	= (u32)0x00003000;
	mx6q_ddr_iomux->dram_sdcke1	= (u32)0x00003000;
	mx6q_ddr_iomux->dram_sdba2	= (u32)0x00000000;
	mx6q_ddr_iomux->dram_sdodt0	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdodt1	= (u32)0x00000030;
	mx6q_grp_iomux->grp_ctlds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_ddrmode_ctl = (u32)0x00020000;
	mx6q_ddr_iomux->dram_sdqs0	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs1	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs2	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs3	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs4	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs5	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs6	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs7	= (u32)0x00000030;
	mx6q_grp_iomux->grp_ddrmode	= (u32)0x00020000;
	mx6q_grp_iomux->grp_b0ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b1ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b2ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b3ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b4ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b5ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b6ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b7ds	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm0	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm1	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm2	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm3	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm4	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm5	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm6	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm7	= (u32)0x00000030;
}

static void spl_dram_init_mx6dqp_2g(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	volatile struct mmdc_p_regs *mmdc_p1;
	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;
	mmdc_p1 = (struct mmdc_p_regs *) MMDC_P1_BASE_ADDR;
	u32 *nocddrc = (u32 *) 0x00bb0000; /* undocumented */

	/* Calibrations */
	/* ZQ */
	mmdc_p0->mpzqhwctrl 	= (u32)0xa1390003;
	while (mmdc_p0->mpzqhwctrl & 0x00010000);

	/* write leveling */
	mmdc_p0->mpwldectrl0 	= (u32)0x001b001e;
	mmdc_p0->mpwldectrl1 	= (u32)0x002e0029;
	mmdc_p1->mpwldectrl0 	= (u32)0x001b002a;
	mmdc_p1->mpwldectrl1 	= (u32)0x0019002c;
	/*
	 * DQS gating, read delay, write delay calibration values
	 * based on calibration compare of 0x00ffff00
	 */
	mmdc_p0->mpdgctrl0	= (u32)0x43240334;
	mmdc_p0->mpdgctrl1	= (u32)0x0324031a;
	mmdc_p1->mpdgctrl0	= (u32)0x43340344;
	mmdc_p1->mpdgctrl1	= (u32)0x03280276;

	mmdc_p0->mprddlctl	= (u32)0x44383A3E;
	mmdc_p1->mprddlctl	= (u32)0x3C3C3846;

	mmdc_p0->mpwrdlctl	= (u32)0x2e303230;
	mmdc_p1->mpwrdlctl	= (u32)0x38283E34;

	mmdc_p0->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby3dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby3dl 	= (u32)0x33333333;

	mmdc_p0->mpdccr 	= (u32)0x24912249; /* new */
	mmdc_p1->mpdccr 	= (u32)0x24914289; /* new */

	mmdc_p0->mpmur0 	= (u32)0x00000800;
	mmdc_p1->mpmur0 	= (u32)0x00000800;
	/* MMDC init: in DDR3, 64-bit mode, only MMDC0 is initiated: */
	mmdc_p0->mdpdc 		= (u32)0x00020036;
	mmdc_p0->mdotc 		= (u32)0x24444040;

	mmdc_p0->mdcfg0 	= (u32)0x898E7955;
	mmdc_p0->mdcfg1 	= (u32)0xFF320F64;
	mmdc_p0->mdcfg2 	= (u32)0x01FF00DB;

	mmdc_p0->mdmisc 	= (u32)0x00001740;
	mmdc_p0->mdscr 		= (u32)0x00008000;

	mmdc_p0->mdrwd 		= (u32)0x000026d2;

	mmdc_p0->mdor 		= (u32)0x008E1023;

	/* 2G */
	mmdc_p0->mdasp 		= (u32)0x00000047;
	mmdc_p0->maarcr		= (u32)0x14420000; /* new */
	mmdc_p0->mdctl 		= (u32)0x841a0000;
	mmdc_p0->mppdcmpr2	= (u32)0x00400C58; /* new */

	/* undocumented */
	nocddrc[2]		= (u32)0x00000000;
	nocddrc[3]		= (u32)0x2891E41A;
	nocddrc[14]		= (u32)0x00000564;
	nocddrc[5]		= (u32)0x00000040;
	nocddrc[10]		= (u32)0x00000020;
	nocddrc[11]		= (u32)0x00000020;

	mmdc_p0->mdscr 		= (u32)0x04088032;
	mmdc_p0->mdscr 		= (u32)0x00008033;
	mmdc_p0->mdscr 		= (u32)0x00048031;
	mmdc_p0->mdscr 		= (u32)0x09408030;
	mmdc_p0->mdscr 		= (u32)0x04008040;

	mmdc_p0->mdref 		= (u32)0x00005800;

	mmdc_p0->mpodtctrl 	= (u32)0x00011117;
	mmdc_p1->mpodtctrl 	= (u32)0x00011117;

	mmdc_p0->mdpdc 		= (u32)0x00025576;
	mmdc_p0->mapsr 		= (u32)0x00011006;
	mmdc_p0->mdscr 		= (u32)0x00000000;
}

static void spl_mx6qd_dram_setup_iomux(void)
{
	volatile struct mx6dq_iomux_ddr_regs *mx6q_ddr_iomux;
	volatile struct mx6dq_iomux_grp_regs *mx6q_grp_iomux;

	mx6q_ddr_iomux = (struct mx6dq_iomux_ddr_regs *) MX6DQ_IOM_DDR_BASE;
	mx6q_grp_iomux = (struct mx6dq_iomux_grp_regs *) MX6DQ_IOM_GRP_BASE;

	mx6q_grp_iomux->grp_ddr_type	= (u32)0x000c0000;
	mx6q_grp_iomux->grp_ddrpke	= (u32)0x00000000;
	mx6q_ddr_iomux->dram_sdclk_0	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdclk_1	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_cas	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_ras	= (u32)0x00000030;
	mx6q_grp_iomux->grp_addds	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_reset	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdcke0	= (u32)0x00003000;
	mx6q_ddr_iomux->dram_sdcke1	= (u32)0x00003000;
	mx6q_ddr_iomux->dram_sdba2	= (u32)0x00000000;
	mx6q_ddr_iomux->dram_sdodt0	= (u32)0x00003030;
	mx6q_ddr_iomux->dram_sdodt1	= (u32)0x00003030;
	mx6q_grp_iomux->grp_ctlds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_ddrmode_ctl = (u32)0x00020000;
	mx6q_ddr_iomux->dram_sdqs0	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs1	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs2	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs3	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs4	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs5	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs6	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs7	= (u32)0x00000030;
	mx6q_grp_iomux->grp_ddrmode	= (u32)0x00020000;
	mx6q_grp_iomux->grp_b0ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b1ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b2ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b3ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b4ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b5ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b6ds	= (u32)0x00000030;
	mx6q_grp_iomux->grp_b7ds	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm0	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm1	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm2	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm3	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm4	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm5	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm6	= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm7	= (u32)0x00000030;
}

static void spl_dram_init_mx6dl_1g(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	volatile struct mmdc_p_regs *mmdc_p1;
	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;
	mmdc_p1 = (struct mmdc_p_regs *) MMDC_P1_BASE_ADDR;

	/* Calibrations */
	/* ZQ */
	mmdc_p0->mpzqhwctrl 	= (u32)0xa1390003;
	mmdc_p1->mpzqhwctrl 	= (u32)0xa1390003;
	/* write leveling */
	mmdc_p0->mpwldectrl0 	= (u32)0x001f001f;
	mmdc_p0->mpwldectrl1 	= (u32)0x001f001f;
	mmdc_p1->mpwldectrl0 	= (u32)0x001f001f;
	mmdc_p1->mpwldectrl1 	= (u32)0x001f001f;
	/*
	 * DQS gating, read delay, write delay calibration values
	 * based on calibration compare of 0x00ffff00
	 */
	mmdc_p0->mpdgctrl0 	= (u32)0x42440244;
	mmdc_p0->mpdgctrl1 	= (u32)0x02300238;
	mmdc_p1->mpdgctrl0 	= (u32)0x421C0228;
	mmdc_p1->mpdgctrl1 	= (u32)0x0214021C;

	mmdc_p0->mprddlctl 	= (u32)0x38362E32;
	mmdc_p1->mprddlctl 	= (u32)0x3234342C;

	mmdc_p0->mpwrdlctl 	= (u32)0x38362E32;
	mmdc_p1->mpwrdlctl 	= (u32)0x2B35382B;

	/* read data bit delay */
	mmdc_p0->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby3dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby3dl 	= (u32)0x33333333;
	/* Complete calibration by forced measurment */
	mmdc_p0->mpmur0 	= (u32)0x00000800;
	mmdc_p1->mpmur0 	= (u32)0x00000800;
	/* MMDC init: in DDR3, 64-bit mode, only MMDC0 is initiated: */
	mmdc_p0->mdpdc 		= (u32)0x0002002d;
	mmdc_p0->mdotc 		= (u32)0x00333030;
	mmdc_p0->mdcfg0 	= (u32)0x3F435313;
	mmdc_p0->mdcfg1 	= (u32)0xB66E8B63;
	mmdc_p0->mdcfg2 	= (u32)0x01FF00DB;
	mmdc_p0->mdmisc 	= (u32)0x00081740;

	mmdc_p0->mdscr		= (u32)0x00008000;
	mmdc_p0->mdrwd 		= (u32)0x000026d2;
	mmdc_p0->mdor 		= (u32)0x00431023;
	mmdc_p0->mdasp 		= (u32)0x00000027;

	mmdc_p0->mdctl 		= (u32)0x831A0000;

	mmdc_p0->mdscr 		= (u32)0x04008032;
	mmdc_p0->mdscr 		= (u32)0x00008033;
	mmdc_p0->mdscr 		= (u32)0x00048031;
	mmdc_p0->mdscr 		= (u32)0x05208030;
	mmdc_p0->mdscr 		= (u32)0x04008040;

	/* final DDR setup */
	mmdc_p0->mdref 		= (u32)0x00005800;

	mmdc_p0->mpodtctrl 	= (u32)0x00011117;
	mmdc_p1->mpodtctrl 	= (u32)0x00011117;

	mmdc_p0->mdpdc 		= (u32)0x00025565;
	mmdc_p1->mapsr 		= (u32)0x00011006;

	mmdc_p0->mdscr 		= (u32)0x00000000;
}

#ifndef CONFIG_DDR_2GB
static void spl_dram_init_mx6q_1g(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	volatile struct mmdc_p_regs *mmdc_p1;

	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;
	mmdc_p1 = (struct mmdc_p_regs *) MMDC_P1_BASE_ADDR;

	/* Calibrations */
	/* ZQ */
	mmdc_p0->mpzqhwctrl 	= (u32)0xa1390003;
	while (mmdc_p0->mpzqhwctrl & 0x00010000);

	/* write leveling */
	mmdc_p0->mpwldectrl0 	= (u32)0x001C0019;
	mmdc_p0->mpwldectrl1 	= (u32)0x00260026;
	mmdc_p1->mpwldectrl0 	= (u32)0x001D002C;
	mmdc_p1->mpwldectrl1 	= (u32)0x0019002E;
	/*
	 * DQS gating, read delay, write delay calibration values
	 * based on calibration compare of 0x00ffff00
	 */
	mmdc_p0->mpdgctrl0 	= (u32)0x45300544;
	mmdc_p0->mpdgctrl1 	= (u32)0x052C0520;
	mmdc_p1->mpdgctrl0 	= (u32)0x4528053C;
	mmdc_p1->mpdgctrl1 	= (u32)0x052C0470;

	mmdc_p0->mprddlctl 	= (u32)0x3E363A40;
	mmdc_p1->mprddlctl 	= (u32)0x403C3246;

	mmdc_p0->mpwrdlctl 	= (u32)0x3A38443C;
	mmdc_p1->mpwrdlctl 	= (u32)0x48364A3E;

	mmdc_p0->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby3dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby3dl 	= (u32)0x33333333;

	mmdc_p0->mpmur0 	= (u32)0x00000800;
	mmdc_p1->mpmur0 	= (u32)0x00000800;
	/* MMDC init: in DDR3, 64-bit mode, only MMDC0 is initiated: */
	mmdc_p0->mdpdc 		= (u32)0x00020036;
	mmdc_p0->mdotc 		= (u32)0x09444040;

	mmdc_p0->mdcfg0 	= (u32)0x555A7974;
	mmdc_p0->mdcfg1 	= (u32)0xDB538F64;
	mmdc_p0->mdcfg2 	= (u32)0x01FF00DB;

	mmdc_p0->mdmisc 	= (u32)0x00001740;
	mmdc_p0->mdscr 		= (u32)0x00000000;

	mmdc_p0->mdrwd 		= (u32)0x000026d2;

	mmdc_p0->mdor 		= (u32)0x005a1023;

	/* 1G */
	mmdc_p0->mdasp 		= (u32)0x00000027;
	mmdc_p0->mdctl 		= (u32)0x841A0000;
	mmdc_p0->mdscr 		= (u32)0x04088032;
	mmdc_p0->mdscr 		= (u32)0x00008033;
	mmdc_p0->mdscr 		= (u32)0x00048031;
	mmdc_p0->mdscr 		= (u32)0x09408030;
	mmdc_p0->mdscr 		= (u32)0x04008040;
	mmdc_p0->mdref 		= (u32)0x00005800;
	mmdc_p0->mpodtctrl 	= (u32)0x00011117;
	mmdc_p1->mpodtctrl 	= (u32)0x00011117;
	mmdc_p0->mdpdc 		= (u32)0x00025576;
	mmdc_p0->mapsr 		= (u32)0x00011006;
	mmdc_p0->mdscr 		= (u32)0x00000000;
}
#endif

static void spl_dram_init_mx6q_2g(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	volatile struct mmdc_p_regs *mmdc_p1;
	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;
	mmdc_p1 = (struct mmdc_p_regs *) MMDC_P1_BASE_ADDR;

	/* Calibrations */
	/* ZQ */
	mmdc_p0->mpzqhwctrl 	= (u32)0xa1390003;
	while (mmdc_p0->mpzqhwctrl & 0x00010000);

#ifdef CONFIG_DDR_2GB
	/* write leveling */
	mmdc_p0->mpwldectrl0 	= (u32)0x001C001C;
	mmdc_p0->mpwldectrl1 	= (u32)0x0026001E;
	mmdc_p1->mpwldectrl0 	= (u32)0x00240030;
	mmdc_p1->mpwldectrl1 	= (u32)0x00150028;
	/*
	 * DQS gating, read delay, write delay calibration values
	 * based on calibration compare of 0x00ffff00
	 */
	mmdc_p0->mpdgctrl0 	= (u32)0x43240338;
	mmdc_p0->mpdgctrl1 	= (u32)0x03240318;
	mmdc_p1->mpdgctrl0 	= (u32)0x43380344;
	mmdc_p1->mpdgctrl1 	= (u32)0x03300268;

	mmdc_p0->mprddlctl 	= (u32)0x3C323436;
	mmdc_p1->mprddlctl 	= (u32)0x38383444;

	mmdc_p0->mpwrdlctl 	= (u32)0x363A3C36;
	mmdc_p1->mpwrdlctl 	= (u32)0x46344840;
#else	/* 1.2Ghz CPU */
	/* write leveling */
	mmdc_p0->mpwldectrl0 	= (u32)0x001F0019;
	mmdc_p0->mpwldectrl1 	= (u32)0x0024001F;
	mmdc_p1->mpwldectrl0 	= (u32)0x001F002B;
	mmdc_p1->mpwldectrl1 	= (u32)0x00130029;
	/*
	 * DQS gating, read delay, write delay calibration values
	 * based on calibration compare of 0x00ffff00
	 */
	mmdc_p0->mpdgctrl0	= (u32)0x43240334;
	mmdc_p0->mpdgctrl1	= (u32)0x03200314;
	mmdc_p1->mpdgctrl0	= (u32)0x432C0340;
	mmdc_p1->mpdgctrl1	= (u32)0x032C0278;

	mmdc_p0->mprddlctl	= (u32)0x4036363A;
	mmdc_p1->mprddlctl	= (u32)0x3A363446;

	mmdc_p0->mpwrdlctl	= (u32)0x38383E3A;
	mmdc_p1->mpwrdlctl	= (u32)0x46344840;
#endif
	mmdc_p0->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby3dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby3dl 	= (u32)0x33333333;

	mmdc_p0->mpmur0 	= (u32)0x00000800;
	mmdc_p1->mpmur0 	= (u32)0x00000800;
	/* MMDC init: in DDR3, 64-bit mode, only MMDC0 is initiated: */
	mmdc_p0->mdpdc 		= (u32)0x00020036;
	mmdc_p0->mdotc 		= (u32)0x09444040;

	mmdc_p0->mdcfg0 	= (u32)0x8A8F7955;
	mmdc_p0->mdcfg1 	= (u32)0xFF328F64;
	mmdc_p0->mdcfg2 	= (u32)0x01ff00db;

	mmdc_p0->mdmisc 	= (u32)0x00081740;
	mmdc_p0->mdscr 		= (u32)0x00008000;

	mmdc_p0->mdrwd 		= (u32)0x000026d2;

	mmdc_p0->mdor 		= (u32)0x008F1023;

	/* 2G */
	mmdc_p0->mdasp 		= (u32)0x00000047;
	mmdc_p0->mdctl 		= (u32)0x841a0000;

	mmdc_p0->mdscr 		= (u32)0x04088032;
	mmdc_p0->mdscr 		= (u32)0x00008033;
	mmdc_p0->mdscr 		= (u32)0x00048031;
	mmdc_p0->mdscr 		= (u32)0x09408030;
	mmdc_p0->mdscr 		= (u32)0x04008040;

	mmdc_p0->mdref 		= (u32)0x00005800;

	mmdc_p0->mpodtctrl 	= (u32)0x00011117;
	mmdc_p1->mpodtctrl 	= (u32)0x00011117;

	mmdc_p0->mdpdc 		= (u32)0x00025576;
	mmdc_p0->mapsr 		= (u32)0x00011006;
	mmdc_p0->mdscr 		= (u32)0x00000000;
}

static void spl_mx6dlsl_dram_setup_iomux(void)
{
	volatile struct mx6sdl_iomux_ddr_regs *mx6dl_ddr_iomux;
	volatile struct mx6sdl_iomux_grp_regs *mx6dl_grp_iomux;

	mx6dl_ddr_iomux = (struct mx6sdl_iomux_ddr_regs *) MX6SDL_IOM_DDR_BASE;
	mx6dl_grp_iomux = (struct mx6sdl_iomux_grp_regs *) MX6SDL_IOM_GRP_BASE;

	mx6dl_grp_iomux->grp_ddr_type   = (u32)0x000c0000;
	mx6dl_grp_iomux->grp_ddrpke     = (u32)0x00000000;
	mx6dl_ddr_iomux->dram_sdclk_0   = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdclk_1   = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_cas       = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_ras       = (u32)0x00000030;
	mx6dl_grp_iomux->grp_addds      = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_reset     = (u32)0x000c0030;
	mx6dl_ddr_iomux->dram_sdcke0    = (u32)0x00003000;
	mx6dl_ddr_iomux->dram_sdcke1    = (u32)0x00003000;
	mx6dl_ddr_iomux->dram_sdba2     = (u32)0x00000000;
	mx6dl_ddr_iomux->dram_sdodt0    = (u32)0x00003030;
	mx6dl_ddr_iomux->dram_sdodt1    = (u32)0x00003030;
	mx6dl_grp_iomux->grp_ctlds      = (u32)0x00000030;
	mx6dl_grp_iomux->grp_ddrmode_ctl= (u32)0x00020000;
	mx6dl_ddr_iomux->dram_sdqs0     = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs1     = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs2     = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs3     = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs4     = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs5     = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs6     = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs7     = (u32)0x00000030;
	mx6dl_grp_iomux->grp_ddrmode    = (u32)0x00020000;
	mx6dl_grp_iomux->grp_b0ds       = (u32)0x00000030;
	mx6dl_grp_iomux->grp_b1ds       = (u32)0x00000030;
	mx6dl_grp_iomux->grp_b2ds       = (u32)0x00000030;
	mx6dl_grp_iomux->grp_b3ds       = (u32)0x00000030;
	mx6dl_grp_iomux->grp_b4ds       = (u32)0x00000030;
	mx6dl_grp_iomux->grp_b5ds       = (u32)0x00000030;
	mx6dl_grp_iomux->grp_b6ds       = (u32)0x00000030;
	mx6dl_grp_iomux->grp_b7ds       = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm0      = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm1      = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm2      = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm3      = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm4      = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm5      = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm6      = (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm7      = (u32)0x00000030;
}

static void spl_dram_init_mx6solo_1gb(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;

	/* ZQ */
	mmdc_p0->mpzqhwctrl     = (u32)0xa1390003;
	/* Write leveling */
	mmdc_p0->mpwldectrl0    = (u32)0x001F001F;
	mmdc_p0->mpwldectrl1    = (u32)0x001F001F;

	mmdc_p0->mpdgctrl0      = (u32)0x42440244;
	mmdc_p0->mpdgctrl1      = (u32)0x02280228;

	mmdc_p0->mprddlctl      = (u32)0x484A4C4A;

	mmdc_p0->mpwrdlctl      = (u32)0x38363236;
	/* Read data bit delay */
	mmdc_p0->mprddqby0dl    = (u32)0x33333333;
	mmdc_p0->mprddqby1dl    = (u32)0x33333333;
	mmdc_p0->mprddqby2dl    = (u32)0x33333333;
	mmdc_p0->mprddqby3dl    = (u32)0x33333333;
	/* Complete calibration by forced measurement */
	mmdc_p0->mpmur0         = (u32)0x00000800;

	mmdc_p0->mdpdc          = (u32)0x00020025;
	mmdc_p0->mdotc          = (u32)0x00333030;
	mmdc_p0->mdcfg0         = (u32)0x676B5313;
	mmdc_p0->mdcfg1         = (u32)0xB66E8B63;
	mmdc_p0->mdcfg2         = (u32)0x01ff00db;
	mmdc_p0->mdmisc         = (u32)0x00001740;

	mmdc_p0->mdscr          = (u32)0x00008000;
	mmdc_p0->mdrwd          = (u32)0x000026d2;
	mmdc_p0->mdor           = (u32)0x006B1023;
	mmdc_p0->mdasp          = (u32)0x00000027;

	mmdc_p0->mdctl          = (u32)0x84190000;

	mmdc_p0->mdscr          = (u32)0x04008032;
	mmdc_p0->mdscr          = (u32)0x00008033;
	mmdc_p0->mdscr          = (u32)0x00048031;
	mmdc_p0->mdscr          = (u32)0x05208030;
	mmdc_p0->mdscr          = (u32)0x04008040;

	mmdc_p0->mdref          = (u32)0x00005800;
	mmdc_p0->mpodtctrl      = (u32)0x00011117;

	mmdc_p0->mdpdc          = (u32)0x00025565;
	mmdc_p0->mdscr          = (u32)0x00000000;
}

/*
 * Sizes are in MiB
 */
static u32 get_actual_ram_size(u32 max_size)
{
#define PATTERN 0x3f3f3f3f
	unsigned int volatile * const port1 = (unsigned int *) PHYS_SDRAM;
	unsigned int volatile * port2;
	unsigned int temp1;
	unsigned int temp2;
	u32 ram_size = max_size;

	do {
		port2 = (unsigned int volatile *) (PHYS_SDRAM + ((ram_size * 1024 * 1024) / 2));

		temp2 = *port2;
		temp1 = *port1;

		*port2 = 0;			/* Write 0 to start of 2nd half of memory */
		*port1 = PATTERN;		/* Write pattern to start of memory */

		if ((PATTERN == *port2) && (ram_size > 512)) {
			*port1 = temp1;
			ram_size /= 2;		/* Next step: devide size by 2 */
		}
		else
		{
			if (0 == *port2) {	/* Done - actual size found */
				*port1 = temp1;
				*port2 = temp2;
				break;
			}
		}
	} while (ram_size > 512);

	return ram_size;
#undef PATTERN
}

void var_legacy_dram_init(bool is_som_solo)
{
	u32 ram_size;
	switch (get_cpu_type()) {
	case MXC_CPU_MX6QP:
	case MXC_CPU_MX6DP:
		spl_mx6dqp_dram_setup_iomux();
		spl_dram_init_mx6dqp_2g();
		ram_size = get_actual_ram_size(2048);
		break;
	case MXC_CPU_MX6Q:
	case MXC_CPU_MX6D:
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
		break;
	case MXC_CPU_MX6SOLO:
		spl_mx6dlsl_dram_setup_iomux();
		spl_dram_init_mx6solo_1gb();
		ram_size = get_actual_ram_size(1024);
		if (ram_size == 512) {
			mdelay(1);
			reset_ddr_solo();
			spl_mx6dlsl_dram_setup_iomux();
			spl_dram_init_mx6solo_512mb();
			mdelay(1);
		}
		break;
	case MXC_CPU_MX6DL:
	default:
		spl_mx6dlsl_dram_setup_iomux();
		if (is_som_solo)
			spl_dram_init_mx6solo_1gb();
		else
			spl_dram_init_mx6dl_1g();
		ram_size = get_actual_ram_size(1024);
		break;
	}

	var_set_ram_size(ram_size);
	printf("\nDDR LEGACY configuration\n");
}

#endif /* CONFIG_SPL_BUILD */
