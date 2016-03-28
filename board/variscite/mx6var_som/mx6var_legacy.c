/*
 * Copyright (C) 2016 Variscite Ltd. All Rights Reserved.
 * Maintainer: Eran Matityahu <eran.m@variscite.com>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifdef CONFIG_SPL_BUILD
#include <asm/imx-common/iomux-v3.h>
#include <asm/io.h>
#include <asm/arch/mx6-ddr.h>
#include "mx6var_legacy.h"

/*
 * Sizes are in MiB
 */
u32 get_actual_ram_size(u32 max_size)
{
#define PATTERN 0x3f3f3f3f
	unsigned int volatile * const port1 = (unsigned int *) PHYS_SDRAM;
	unsigned int volatile * port2;
	unsigned int temp1;
	unsigned int temp2;
	u32 ram_size=max_size;

	do {
		port2 = (unsigned int volatile *) (PHYS_SDRAM + ((ram_size * 1024 * 1024) / 2));

		temp2 = *port2;
		temp1 = *port1;

		*port2 = 0;			/* Write zero to start of second half of memory */
		*port1 = PATTERN;		/* Write pattern to start of memory */

		if ((PATTERN == *port2) && (ram_size > 512)) {
			*port1 = temp1;
			ram_size /= 2;		/* Next step devide size by half */
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

void reset_ddr_solo(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;

	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;

	/* Reset data FIFOs twice */
	setbits_le32(&mmdc_p0->mpdgctrl0, 1 << 31);
	wait_for_bit(&mmdc_p0->mpdgctrl0, 1 << 31, 0);

	setbits_le32(&mmdc_p0->mpdgctrl0, 1 << 31);
	wait_for_bit(&mmdc_p0->mpdgctrl0, 1 << 31, 0);

	mmdc_p0->mdmisc = 2;
	mmdc_p0->mdscr  = (u32)0x00008000;

	wait_for_bit(&mmdc_p0->mdscr, 1 << 14, 1);
}

void spl_dram_init_mx6solo_512mb(void)
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
void spl_mx6qd_dram_setup_iomux(void)
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

void spl_dram_init_mx6dl_1g(void)
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
	/* DQS gating, read delay, write delay calibration values
	   based on calibration compare of 0x00ffff00 */
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
	/* MMDC init:
	   in DDR3, 64-bit mode, only MMDC0 is initiated: */
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

/* i.MX6Q */

void spl_dram_init_mx6q_1g(void)
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
	/* DQS gating, read delay, write delay calibration values
	   based on calibration compare of 0x00ffff00  */
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
	/* MMDC init:
	   in DDR3, 64-bit mode, only MMDC0 is initiated: */
	mmdc_p0->mdpdc 		= (u32)0x00020036;
	mmdc_p0->mdotc 		= (u32)0x09444040;

	mmdc_p0->mdcfg0 	= (u32)0x555A7974;
	mmdc_p0->mdcfg1 	= (u32)0xDB538F64;
	mmdc_p0->mdcfg2 	= (u32)0x01FF00DB;

	mmdc_p0->mdmisc 	= (u32)0x00001740;
	mmdc_p0->mdscr 		= (u32)0x00000000;

	mmdc_p0->mdrwd 		= (u32)0x000026d2;

	mmdc_p0->mdor 		= (u32)0x005a1023;

	/* 2G
	   mmdc_p0->mdasp 		= (u32)0x00000047;
	   mmdc_p0->mdctl 		= (u32)0x841a0000;*/

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

#if 0
void print_dram_data(void)
{
	/*printf("%s START\n", __func__);*/
	volatile struct mmdc_p_regs *mmdc_p0;
	volatile struct mmdc_p_regs *mmdc_p1;

	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;
	mmdc_p1 = (struct mmdc_p_regs *) MMDC_P1_BASE_ADDR;

	/* Calibrations */
	/* ZQ */
	printf("Name			Read	Wrote\n");
	printf("mmdc_p0->mpzqhwctrl 	0x%08x 0xa1390003\n", mmdc_p0->mpzqhwctrl);
	while (mmdc_p0->mpzqhwctrl & 0x00010000)
		;
	/* write leveling */	
	printf("mmdc_p0->mpwldectrl0 	0x%08x 0x001C0019\n", mmdc_p0->mpwldectrl0);
	printf("mmdc_p0->mpwldectrl1 	0x%08x 0x00260026\n", mmdc_p0->mpwldectrl1);
	printf("mmdc_p1->mpwldectrl0 	0x%08x 0x001D002C\n", mmdc_p1->mpwldectrl0);
	printf("mmdc_p1->mpwldectrl1 	0x%08x 0x0019002E\n", mmdc_p1->mpwldectrl1);
	/* DQS gating, read delay, write delay calibration values
	   based on calibration compare of 0x00ffff00  */
	printf("mmdc_p0->mpdgctrl0 	0x%08x 0x45300544\n", mmdc_p0->mpdgctrl0);
	printf("mmdc_p0->mpdgctrl1 	0x%08x 0x052C0520\n", mmdc_p0->mpdgctrl1);
	printf("mmdc_p1->mpdgctrl0 	0x%08x 0x4528053C\n", mmdc_p1->mpdgctrl0);
	printf("mmdc_p1->mpdgctrl1 	0x%08x 0x052C0470\n", mmdc_p1->mpdgctrl1);

	printf("mmdc_p0->mprddlctl 	0x%08x 0x3E363A40\n", mmdc_p0->mprddlctl);
	printf("mmdc_p1->mprddlctl 	0x%08x 0x403C3246\n", mmdc_p1->mprddlctl);

	printf("mmdc_p0->mpwrdlctl 	0x%08x 0x3A38443C\n", mmdc_p0->mpwrdlctl);
	printf("mmdc_p1->mpwrdlctl 	0x%08x 0x48364A3E\n", mmdc_p1->mpwrdlctl);

	printf("mmdc_p0->mprddqby0dl 	0x%08x 0x33333333\n", mmdc_p0->mprddqby0dl);
	printf("mmdc_p0->mprddqby1dl 	0x%08x 0x33333333\n", mmdc_p0->mprddqby1dl);
	printf("mmdc_p0->mprddqby2dl 	0x%08x 0x33333333\n", mmdc_p0->mprddqby2dl);
	printf("mmdc_p0->mprddqby3dl 	0x%08x 0x33333333\n", mmdc_p0->mprddqby3dl);
	printf("mmdc_p1->mprddqby0dl 	0x%08x 0x33333333\n", mmdc_p1->mprddqby0dl);
	printf("mmdc_p1->mprddqby1dl 	0x%08x 0x33333333\n", mmdc_p1->mprddqby1dl);
	printf("mmdc_p1->mprddqby2dl 	0x%08x 0x33333333\n", mmdc_p1->mprddqby2dl);
	printf("mmdc_p1->mprddqby3dl 	0x%08x 0x33333333\n", mmdc_p1->mprddqby3dl);

	printf("mmdc_p0->mpmur0 	0x%08x 0x00000800\n", mmdc_p0->mpmur0);
	printf("mmdc_p1->mpmur0 	0x%08x 0x00000800\n", mmdc_p1->mpmur0);
	/* MMDC init:
	   in DDR3, 64-bit mode, only MMDC0 is initiated: */
	printf("mmdc_p0->mdpdc 		0x%08x 0x00020036\n", mmdc_p0->mdpdc);
	printf("mmdc_p0->mdotc 		0x%08x 0x09444040\n", mmdc_p0->mdotc);

	printf("mmdc_p0->mdcfg0 	0x%08x 0x555A7974\n", mmdc_p0->mdcfg0);
	printf("mmdc_p0->mdcfg1 	0x%08x 0xDB538F64\n", mmdc_p0->mdcfg1);
	printf("mmdc_p0->mdcfg2 	0x%08x 0x01FF00DB\n", mmdc_p0->mdcfg2);

	printf("mmdc_p0->mdmisc 	0x%08x 0x00001740\n", mmdc_p0->mdmisc);
	printf("mmdc_p0->mdscr 		0x%08x 0x00000000\n", mmdc_p0->mdscr);

	printf("mmdc_p0->mdrwd 		0x%08x 0x000026d2\n", mmdc_p0->mdrwd);

	printf("mmdc_p0->mdor 		0x%08x 0x005a1023\n", mmdc_p0->mdor);

	/* 2G                       
	   printf("mmdc_p0->mdasp 		0x%08x 0x00000047\n", mmdc_p0->mdasp);
	   printf("mmdc_p0->mdctl 		0x%08x 0x841a0000\n", mmdc_p0->mdctl);*/

	/* 1G */                                  
	printf("mmdc_p0->mdasp 		0x%08x 0x00000027\n", mmdc_p0->mdasp);
	printf("mmdc_p0->mdctl 		0x%08x 0x841A0000\n", mmdc_p0->mdctl);

	printf("mmdc_p0->mdscr 		0x%08x 0x04088032\n", mmdc_p0->mdscr);
	printf("mmdc_p0->mdscr 		0x%08x 0x00008033\n", mmdc_p0->mdscr);
	printf("mmdc_p0->mdscr 		0x%08x 0x00048031\n", mmdc_p0->mdscr);
	printf("mmdc_p0->mdscr 		0x%08x 0x09408030\n", mmdc_p0->mdscr);
	printf("mmdc_p0->mdscr 		0x%08x 0x04008040\n", mmdc_p0->mdscr);

	printf("mmdc_p0->mdref 		0x%08x 0x00005800\n", mmdc_p0->mdref);

	printf("mmdc_p0->mpodtctrl 	0x%08x 0x00011117\n", mmdc_p0->mpodtctrl);
	printf("mmdc_p1->mpodtctrl 	0x%08x 0x00011117\n", mmdc_p1->mpodtctrl);

	printf("mmdc_p0->mdpdc 		0x%08x 0x00025576\n", mmdc_p0->mdpdc);
	printf("mmdc_p0->mapsr 		0x%08x 0x00011006\n", mmdc_p0->mapsr);
	printf("mmdc_p0->mdscr 		0x%08x 0x00000000\n", mmdc_p0->mdscr);

	/*printf("%s END\n", __func__);*/
}
#endif

void spl_dram_init_mx6q_2g(void)
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
	/* DQS gating, read delay, write delay calibration values
	   based on calibration compare of 0x00ffff00  */
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
	/* DQS gating, read delay, write delay calibration values
	   based on calibration compare of 0x00ffff00  */
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
	/* MMDC init:
	   in DDR3, 64-bit mode, only MMDC0 is initiated: */
	mmdc_p0->mdpdc 		= (u32)0x00020036;
	mmdc_p0->mdotc 		= (u32)0x09444040;

	mmdc_p0->mdcfg0 	= (u32)0x8A8F7955; /* 0x555a7975; */
	mmdc_p0->mdcfg1 	= (u32)0xFF328F64; /* 0xff538f64; */
	mmdc_p0->mdcfg2 	= (u32)0x01ff00db;

	mmdc_p0->mdmisc 	= (u32)0x00081740;
	mmdc_p0->mdscr 		= (u32)0x00008000;

	mmdc_p0->mdrwd 		= (u32)0x000026d2;

	mmdc_p0->mdor 		= (u32)0x008F1023; /* 0x005a1023; */

	/* 2G */ 
	mmdc_p0->mdasp 		= (u32)0x00000047;
	mmdc_p0->mdctl 		= (u32)0x841a0000;

	/* 1G
	   mmdc_p0->mdasp 		= (u32)0x00000027;
	   mmdc_p0->mdctl 		= (u32)0x831a0000;*/

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


void spl_mx6dlsl_dram_setup_iomux(void)
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

void spl_dram_init_mx6solo_1gb(void)
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
#endif
