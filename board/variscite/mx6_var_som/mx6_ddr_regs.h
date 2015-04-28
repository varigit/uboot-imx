/*
 * Copyright (C) 2013 TechNexion Inc.
 *
 * Author: Edward Lin <linuxfae@technexion.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __ASM_ARCH_MX6_DDR_REGS_H__
#define __ASM_ARCH_MX6_DDR_REGS_H__


/* MMDC P0/P1 Registers */
struct mmdc_p_regs {
	u32 mdctl;
	u32 mdpdc;
	u32 mdotc;
	u32 mdcfg0;
	u32 mdcfg1;
	u32 mdcfg2;
	u32 mdmisc;
	u32 mdscr;
	u32 mdref;
	u32 res1[2];
	u32 mdrwd;
	u32 mdor;
	u32 res2[3];
	u32 mdasp;
	u32 res3[240];
	u32 mapsr;
	u32 res4[254];
	u32 mpzqhwctrl;
	u32 res5[2];
	u32 mpwldectrl0;
	u32 mpwldectrl1;
	u32 res6;
	u32 mpodtctrl;
	u32 mprddqby0dl;
	u32 mprddqby1dl;
	u32 mprddqby2dl;
	u32 mprddqby3dl;
	u32 res7[4];
	u32 mpdgctrl0;
	u32 mpdgctrl1;
	u32 res8;
	u32 mprddlctl;
	u32 res9;
	u32 mpwrdlctl;
	u32 res10[25];
	u32 mpmur0;
};

#define MX6DQ_IOM_DDR_BASE	0x020e0500
struct mx6qd_iomux_ddr_regs {
	u32 res1[3];
	u32 dram_sdqs5;
	u32 dram_dqm5;
	u32 dram_dqm4;
	u32 dram_sdqs4;
	u32 dram_sdqs3;
	u32 dram_dqm3;
	u32 dram_sdqs2;
	u32 dram_dqm2;
	u32 res2[16];
	u32 dram_cas;
	u32 res3[2];
	u32 dram_ras;
	u32 dram_reset;
	u32 res4[2];
	u32 dram_sdclk_0;
	u32 dram_sdba2;
	u32 dram_sdcke0;
	u32 dram_sdclk_1;
	u32 dram_sdcke1;
	u32 dram_sdodt0;
	u32 dram_sdodt1;
	u32 res5;
	u32 dram_sdqs0;
	u32 dram_dqm0;
	u32 dram_sdqs1;
	u32 dram_dqm1;
	u32 dram_sdqs6;
	u32 dram_dqm6;
	u32 dram_sdqs7;
	u32 dram_dqm7;
};

#define MX6DQ_IOM_GRP_BASE	0x020e0700
struct mx6qd_iomux_grp_regs {
	u32 res1[18];
	u32 grp_b7ds;
	u32 grp_addds;
	u32 grp_ddrmode_ctl;
	u32 res2;
	u32 grp_ddrpke;
	u32 res3[6];
	u32 grp_ddrmode;
	u32 res4[3];
	u32 grp_b0ds;
	u32 grp_b1ds;
	u32 grp_ctlds;
	u32 res5;
	u32 grp_b2ds;
	u32 grp_ddr_type;
	u32 grp_b3ds;
	u32 grp_b4ds;
	u32 grp_b5ds;
	u32 grp_b6ds;
};

#define MX6SDL_IOM_DDR_BASE	0x020e0400
struct mx6sdl_iomux_ddr_regs {
	u32 res1[25];
	u32 dram_cas;
	u32 res2[2];
	u32 dram_dqm0;
	u32 dram_dqm1;
	u32 dram_dqm2;
	u32 dram_dqm3;
	u32 dram_dqm4;
	u32 dram_dqm5;
	u32 dram_dqm6;
	u32 dram_dqm7;
	u32 dram_ras;
	u32 dram_reset;
	u32 res3[2];
	u32 dram_sdba2;
	u32 dram_sdcke0;
	u32 dram_sdcke1;
	u32 dram_sdclk_0;
	u32 dram_sdclk_1;
	u32 dram_sdodt0;
	u32 dram_sdodt1;
	u32 dram_sdqs0;
	u32 dram_sdqs1;
	u32 dram_sdqs2;
	u32 dram_sdqs3;
	u32 dram_sdqs4;
	u32 dram_sdqs5;
	u32 dram_sdqs6;
	u32 dram_sdqs7;
};

#define MX6SDL_IOM_GRP_BASE	0x020e0700
struct mx6sdl_iomux_grp_regs {
	u32 res1[18];
	u32 grp_b7ds;
	u32 grp_addds;
	u32 grp_ddrmode_ctl;
	u32 grp_ddrpke;
	u32 res2[2];
	u32 grp_ddrmode;
	u32 grp_b0ds;
	u32 res3;
	u32 grp_ctlds;
	u32 grp_b1ds;
	u32 grp_ddr_type;
	u32 grp_b2ds;
	u32 grp_b3ds;
	u32 grp_b4ds;
	u32 grp_b5ds;
	u32 res4;
	u32 grp_b6ds;
};

#endif
