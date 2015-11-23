/*
 * Author: Tungyi Lin <tungyilin1127@gmail.com>
 *
 * Derived from EDM_CF_IMX6 code by TechNexion,Inc
 * Copyright (C) 2014 Variscite Ltd. All Rights Reserved.
 * Maintainer: Ron Donio <ron.d@variscite.com>
 * Configuration settings for the Variscite VAR_SOM_MX6 board.
 *
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#ifdef CONFIG_SPL
#include <spl.h>
#include <i2c.h>
#endif

#include "mx6var_eeprom.h"
#include "mx6var_v2_eeprom.h"
#include "mx6var_memories.c"
void p_udelay(int time);

//#define EEPROM_DEBUG 1
#define CONFIG_SPL_STACK	0x0091FFB8

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_SPL_BUILD)

static enum boot_device boot_dev;
enum boot_device get_boot_device(void);
int check_1_2G_only(void);
static ulong sdram_size;

static var_eeprom_config_struct_t g_var_eeprom_cfg;
struct var_eeprom_config_struct_v2_type var_eeprom_config_struct_v2;
static bool g_b_dram_set_by_var_eeprom_config;
u32 is_cpu_pop_package(void);
int eeprom_revision=0;

static inline void setup_boot_device(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4 ;
	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;
	uint bt_mem_mmc = (soc_sbmr & 0x00001000) >> 12;

	switch (bt_mem_ctl) {
	case 0x0:
		if (bt_mem_type)
			boot_dev = MX6_ONE_NAND_BOOT;
		else
			boot_dev = MX6_WEIM_NOR_BOOT;
		break;
	case 0x2:
			boot_dev = MX6_SATA_BOOT;
		break;
	case 0x3:
		if (bt_mem_type)
			boot_dev = MX6_I2C_BOOT;
		else
			boot_dev = MX6_SPI_NOR_BOOT;
		break;
	case 0x4:
	case 0x5:
	case 0x6:
		if (bt_mem_mmc)
			boot_dev = MX6_SD0_BOOT;
		else
			boot_dev = MX6_SD1_BOOT;
		break;
	case 0x7:
		boot_dev = MX6_MMC_BOOT;
		break;
	case 0x8 ... 0xf:
		boot_dev = MX6_NAND_BOOT;
		break;
	default:
		boot_dev = MX6_UNKNOWN_BOOT;
		break;
	}
}

enum boot_device get_boot_device(void) {
	return boot_dev;
}

#include "asm/arch/mx6_ddr_regs.h"

static void spl_dram_init_mx6solo_512mb(void);
static void spl_dram_init_mx6dl_1g(void);
static void spl_dram_init_mx6q_2g(void);

static void ram_size(void)
{
unsigned int volatile * const port1 = (unsigned int *) PHYS_SDRAM;
unsigned int volatile * port2;

	if (is_cpu_pop_package()){
		sdram_size = 1024;
		return;
	}

	do {
		port2 = (unsigned int volatile *) (PHYS_SDRAM + ((sdram_size * 1024 * 1024) / 2));

		*port2 = 0;				// write zero to start of second half of memory.
		*port1 = 0x3f3f3f3f;	// write pattern to start of memory.

		if ((0x3f3f3f3f == *port2) && (sdram_size > 512))
			sdram_size = sdram_size / 2;	// Next step devide size by half
		else

		if (0 == *port2)		// Done actual size found.
			break;

	} while (sdram_size > 512);
}

#ifdef EEPROM_DEBUG
void sdram_long_test_spl(unsigned int capacity)
{
	unsigned int volatile *mem_ptr = (unsigned int *)PHYS_SDRAM;
	unsigned int volatile *mem_end_ptr = (unsigned int *)((unsigned int)PHYS_SDRAM + (1024*1024*capacity) - 4);
	unsigned int volatile mem_size = 1024*1024*capacity;
	unsigned int val, prev_val, i;

	printf("sdram_long_test() - Writing... mem_ptr=0x%08x mem_end_ptr=0x%08x\n", (unsigned int)mem_ptr, (unsigned int)mem_end_ptr);

	// zero memory
	printf("Testing... mem_ptr=0x%08x data=0x%08x \n", (unsigned int)mem_ptr, 0);
	memset(mem_ptr, 0x00, mem_size);

	for (i=0; i<3; i++) {
		switch (i) {
			case 0:
				prev_val =0;val = 0x37373737;
			break;
			case 1:
				prev_val =0x37373737;val =0x73737373 ;
			break;
			case 2:
				prev_val =0x73737373;val = 0;
			break;
		}

		mem_ptr = (unsigned int *)PHYS_SDRAM;
		while (mem_ptr < mem_end_ptr)
		{
			if (0 == ((unsigned int)mem_ptr % 0x1000000))
				printf("Testing... mem_ptr=0x%08x data=0x%08x \n", (unsigned int)mem_ptr, val);

			if (prev_val != *mem_ptr)
				printf("Address <0x%x> excpected 0x%x found 0x%x\n", (unsigned int)mem_ptr, prev_val, *mem_ptr);

			*mem_ptr = val;
			mem_ptr++;
		}
	}
}
#endif

static void reset_ddr_solo(void){
	volatile struct mmdc_p_regs *mmdc_p0;
	u32 conack;

	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;

	mmdc_p0->mdmisc = 2;
	mmdc_p0->mdscr	= (u32)0x00008000;

	do {
		conack = (mmdc_p0->mdscr & 0x4000);
	} while (conack == 0);
}

/*
 * Bugfix: Fix Freescale wrong processor documentation.
 */
static void spl_mx6qd_dram_setup_iomux_check_reset(void)
{
	volatile struct mx6qd_iomux_ddr_regs *mx6q_ddr_iomux;
	volatile struct mx6qd_iomux_grp_regs *mx6q_grp_iomux;
	u32 cpurev, imxtype;

	cpurev = get_cpu_rev();
	imxtype = (cpurev & 0xFF000) >> 12;
	get_imx_type(imxtype);

	if ((imxtype == MXC_CPU_MX6D) || (imxtype == MXC_CPU_MX6Q)){
		mx6q_ddr_iomux = (struct mx6dqd_iomux_ddr_regs *) MX6DQ_IOM_DDR_BASE;
		mx6q_grp_iomux = (struct mx6dqd_iomux_grp_regs *) MX6DQ_IOM_GRP_BASE;
	
		if (mx6q_ddr_iomux->dram_reset == (u32)0x000C0030)
			mx6q_ddr_iomux->dram_reset = (u32)0x00000030;
	}
}


static void spl_mx6qd_dram_setup_iomux(void)
{
	volatile struct mx6qd_iomux_ddr_regs *mx6q_ddr_iomux;
	volatile struct mx6qd_iomux_grp_regs *mx6q_grp_iomux;

	mx6q_ddr_iomux = (struct mx6dqd_iomux_ddr_regs *) MX6DQ_IOM_DDR_BASE;
	mx6q_grp_iomux = (struct mx6dqd_iomux_grp_regs *) MX6DQ_IOM_GRP_BASE;

	mx6q_grp_iomux->grp_ddr_type 		= (u32)0x000c0000;
	mx6q_grp_iomux->grp_ddrpke 		= (u32)0x00000000;
	mx6q_ddr_iomux->dram_sdclk_0 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdclk_1 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_cas 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_ras 		= (u32)0x00000030;
	mx6q_grp_iomux->grp_addds 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_reset 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdcke0 		= (u32)0x00003000;
	mx6q_ddr_iomux->dram_sdcke1 		= (u32)0x00003000;
	mx6q_ddr_iomux->dram_sdba2 		= (u32)0x00000000;
	mx6q_ddr_iomux->dram_sdodt0 		= (u32)0x00003030;
	mx6q_ddr_iomux->dram_sdodt1 		= (u32)0x00003030;
	mx6q_grp_iomux->grp_ctlds 		= (u32)0x00000030;
	mx6q_grp_iomux->grp_ddrmode_ctl 	= (u32)0x00020000;
	mx6q_ddr_iomux->dram_sdqs0 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs1 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs2 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs3 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs4 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs5 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs6 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_sdqs7 		= (u32)0x00000030;
	mx6q_grp_iomux->grp_ddrmode 		= (u32)0x00020000;
	mx6q_grp_iomux->grp_b0ds 		= (u32)0x00000030;
	mx6q_grp_iomux->grp_b1ds 		= (u32)0x00000030;
	mx6q_grp_iomux->grp_b2ds 		= (u32)0x00000030;
	mx6q_grp_iomux->grp_b3ds 		= (u32)0x00000030;
	mx6q_grp_iomux->grp_b4ds 		= (u32)0x00000030;
	mx6q_grp_iomux->grp_b5ds 		= (u32)0x00000030;
	mx6q_grp_iomux->grp_b6ds 		= (u32)0x00000030;
	mx6q_grp_iomux->grp_b7ds 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm0 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm1 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm2 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm3 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm4 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm5 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm6 		= (u32)0x00000030;
	mx6q_ddr_iomux->dram_dqm7 		= (u32)0x00000030;
}

static void spl_mx6dlsl_dram_setup_iomux(void)
{
	volatile struct mx6sdl_iomux_ddr_regs *mx6dl_ddr_iomux;
	volatile struct mx6sdl_iomux_grp_regs *mx6dl_grp_iomux;

	mx6dl_ddr_iomux = (struct mx6sdl_iomux_ddr_regs *) MX6SDL_IOM_DDR_BASE;
	mx6dl_grp_iomux = (struct mx6sdl_iomux_grp_regs *) MX6SDL_IOM_GRP_BASE;

	mx6dl_grp_iomux->grp_ddr_type 	= (u32)0x000c0000;
	mx6dl_grp_iomux->grp_ddrpke 	= (u32)0x00000000;
	mx6dl_ddr_iomux->dram_sdclk_0 	= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdclk_1 	= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_cas 		= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_ras 		= (u32)0x00000030;
	mx6dl_grp_iomux->grp_addds 		= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_reset 	= (u32)0x000c0030;
	mx6dl_ddr_iomux->dram_sdcke0 	= (u32)0x00003000;
	mx6dl_ddr_iomux->dram_sdcke1 	= (u32)0x00003000;
	mx6dl_ddr_iomux->dram_sdba2 	= (u32)0x00000000;
	mx6dl_ddr_iomux->dram_sdodt0 	= (u32)0x00003030;
	mx6dl_ddr_iomux->dram_sdodt1 	= (u32)0x00003030;
	mx6dl_grp_iomux->grp_ctlds 		= (u32)0x00000030;
	mx6dl_grp_iomux->grp_ddrmode_ctl= (u32)0x00020000;
	mx6dl_ddr_iomux->dram_sdqs0 	= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs1 	= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs2 	= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs3 	= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs4 	= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs5 	= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs6 	= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_sdqs7 	= (u32)0x00000030;
	mx6dl_grp_iomux->grp_ddrmode 	= (u32)0x00020000;
	mx6dl_grp_iomux->grp_b0ds 		= (u32)0x00000030;
	mx6dl_grp_iomux->grp_b1ds 		= (u32)0x00000030;
	mx6dl_grp_iomux->grp_b2ds 		= (u32)0x00000030;
	mx6dl_grp_iomux->grp_b3ds 		= (u32)0x00000030;
	mx6dl_grp_iomux->grp_b4ds 		= (u32)0x00000030;
	mx6dl_grp_iomux->grp_b5ds 		= (u32)0x00000030;
	mx6dl_grp_iomux->grp_b6ds 		= (u32)0x00000030;
	mx6dl_grp_iomux->grp_b7ds 		= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm0 		= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm1 		= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm2 		= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm3 		= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm4 		= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm5 		= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm6 		= (u32)0x00000030;
	mx6dl_ddr_iomux->dram_dqm7 		= (u32)0x00000030;
}

static void spl_dram_init_mx6solo_512mb(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;

	/* ZQ */
	mmdc_p0->mpzqhwctrl 	= (u32)0xa1390003;
	/* Write leveling */
	mmdc_p0->mpwldectrl0 	= (u32)0x001F001F;
	mmdc_p0->mpwldectrl1 	= (u32)0x001F001F;

	mmdc_p0->mpdgctrl0 		= (u32)0x421C0216;
	mmdc_p0->mpdgctrl1 		= (u32)0x017B017A;

	mmdc_p0->mprddlctl 		= (u32)0x4B4A4E4C;
	mmdc_p0->mpwrdlctl 		= (u32)0x38363236;
	/* Read data bit delay */
	mmdc_p0->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby3dl 	= (u32)0x33333333;

	/* Complete calibration by forced measurement */
	mmdc_p0->mpmur0 		= (u32)0x00000800;
	mmdc_p0->mdpdc 			= (u32)0x00020025;
	mmdc_p0->mdotc 			= (u32)0x00333030;
	mmdc_p0->mdcfg0 		= (u32)0x676B5313;
	mmdc_p0->mdcfg1 		= (u32)0xB66E8B63;
	mmdc_p0->mdcfg2 		= (u32)0x01ff00db;
	mmdc_p0->mdmisc 		= (u32)0x00001740;

	mmdc_p0->mdscr 			= (u32)0x00008000;
	mmdc_p0->mdrwd 			= (u32)0x000026d2;
	mmdc_p0->mdor 			= (u32)0x006B1023;
	mmdc_p0->mdasp 			= (u32)0x00000017;

	mmdc_p0->mdctl 			= (u32)0x84190000;

	mmdc_p0->mdscr 			= (u32)0x04008032;
	mmdc_p0->mdscr 			= (u32)0x00008033;
	mmdc_p0->mdscr 			= (u32)0x00048031;
	mmdc_p0->mdscr 			= (u32)0x07208030;
	mmdc_p0->mdscr 			= (u32)0x04008040;

	mmdc_p0->mdref 			= (u32)0x00005800;
	mmdc_p0->mpodtctrl 		= (u32)0x00011117;

	mmdc_p0->mdpdc 			= (u32)0x00025565;
	mmdc_p0->mapsr 			= (u32)0x00011006;
	mmdc_p0->mdscr 			= (u32)0x00000000;
	sdram_size = 512;
}

static void spl_dram_init_mx6solo_1gb(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;

	/* ZQ */
	mmdc_p0->mpzqhwctrl 	= (u32)0xa1390003;
	/* Write leveling */
	mmdc_p0->mpwldectrl0 	= (u32)0x001F001F;
	mmdc_p0->mpwldectrl1 	= (u32)0x001F001F;

	mmdc_p0->mpdgctrl0 		= (u32)0x42440244;
	mmdc_p0->mpdgctrl1 		= (u32)0x02280228;

	mmdc_p0->mprddlctl 		= (u32)0x484A4C4A;

	mmdc_p0->mpwrdlctl 		= (u32)0x38363236;
	/* Read data bit delay */
	mmdc_p0->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby3dl 	= (u32)0x33333333;
	/* Complete calibration by forced measurement */
	mmdc_p0->mpmur0 		= (u32)0x00000800;

	mmdc_p0->mdpdc 			= (u32)0x00020025;
	mmdc_p0->mdotc 			= (u32)0x00333030;
	mmdc_p0->mdcfg0 		= (u32)0x676B5313;
	mmdc_p0->mdcfg1 		= (u32)0xB66E8B63;
	mmdc_p0->mdcfg2 		= (u32)0x01ff00db;
	mmdc_p0->mdmisc 		= (u32)0x00001740;

	mmdc_p0->mdscr 			= (u32)0x00008000;
	mmdc_p0->mdrwd 			= (u32)0x000026d2;
	mmdc_p0->mdor 			= (u32)0x006B1023;
	mmdc_p0->mdasp 			= (u32)0x00000027;

	mmdc_p0->mdctl 			= (u32)0x84190000;

	mmdc_p0->mdscr 			= (u32)0x04008032;
	mmdc_p0->mdscr 			= (u32)0x00008033;
	mmdc_p0->mdscr 			= (u32)0x00048031;
	mmdc_p0->mdscr 			= (u32)0x05208030;
	mmdc_p0->mdscr 			= (u32)0x04008040;

	mmdc_p0->mdref 			= (u32)0x00005800;
	mmdc_p0->mpodtctrl 		= (u32)0x00011117;

	mmdc_p0->mdpdc 			= (u32)0x00025565;
	mmdc_p0->mdscr 			= (u32)0x00000000;
	sdram_size = 1024;
}

//Copy from DCD in flash_header.S
//CONFIG_MX6DL_DDR3
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
	/* DQS gating, read delay, write delay calibration values
	based on calibration compare of 0x00ffff00 */
	mmdc_p0->mpdgctrl0 		= (u32)0x42440244;
	mmdc_p0->mpdgctrl1 		= (u32)0x02300238;
	mmdc_p1->mpdgctrl0 		= (u32)0x421C0228;
	mmdc_p1->mpdgctrl1 		= (u32)0x0214021C;

	mmdc_p0->mprddlctl 		= (u32)0x38362E32;
	mmdc_p1->mprddlctl 		= (u32)0x3234342C;

	mmdc_p0->mpwrdlctl 		= (u32)0x38362E32;
	mmdc_p1->mpwrdlctl 		= (u32)0x2B35382B;

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
	mmdc_p0->mpmur0 		= (u32)0x00000800;
	mmdc_p1->mpmur0 		= (u32)0x00000800;
	/* MMDC init:
	 in DDR3, 64-bit mode, only MMDC0 is initiated: */
	mmdc_p0->mdpdc 			= (u32)0x0002002d;
	mmdc_p0->mdotc 			= (u32)0x00333030;
	mmdc_p0->mdcfg0 		= (u32)0x3F435313;
	mmdc_p0->mdcfg1 		= (u32)0xB66E8B63;
	mmdc_p0->mdcfg2 		= (u32)0x01FF00DB;
	mmdc_p0->mdmisc 		= (u32)0x00081740;

	mmdc_p0->mdscr			= (u32)0x00008000;
	mmdc_p0->mdrwd 			= (u32)0x000026d2;
	mmdc_p0->mdor 			= (u32)0x00431023;
	mmdc_p0->mdasp 			= (u32)0x00000027;

	mmdc_p0->mdctl 			= (u32)0x831A0000;

	mmdc_p0->mdscr 			= (u32)0x04008032;
	mmdc_p0->mdscr 			= (u32)0x00008033;
	mmdc_p0->mdscr 			= (u32)0x00048031;
	mmdc_p0->mdscr 			= (u32)0x05208030;
	mmdc_p0->mdscr 			= (u32)0x04008040;

	/* final DDR setup */
	mmdc_p0->mdref 			= (u32)0x00005800;

	mmdc_p0->mpodtctrl 		= (u32)0x00011117;
	mmdc_p1->mpodtctrl 		= (u32)0x00011117;

	mmdc_p0->mdpdc 			= (u32)0x00025565;
	mmdc_p1->mapsr 			= (u32)0x00011006;

	mmdc_p0->mdscr 			= (u32)0x00000000;
	sdram_size = 1024;
}

/* i.MX6Q */


static void spl_dram_init_mx6q_1g(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	volatile struct mmdc_p_regs *mmdc_p1;

	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;
	mmdc_p1 = (struct mmdc_p_regs *) MMDC_P1_BASE_ADDR;

	/* Calibrations */
	/* ZQ */
	mmdc_p0->mpzqhwctrl 	= (u32)0xa1390003;
	while (mmdc_p0->mpzqhwctrl & 0x00010000)
		;
	/* write leveling */
	mmdc_p0->mpwldectrl0 	= (u32)0x001C0019;
	mmdc_p0->mpwldectrl1 	= (u32)0x00260026;
	mmdc_p1->mpwldectrl0 	= (u32)0x001D002C;
	mmdc_p1->mpwldectrl1 	= (u32)0x0019002E;
	/* DQS gating, read delay, write delay calibration values
	 based on calibration compare of 0x00ffff00  */
	mmdc_p0->mpdgctrl0 		= (u32)0x45300544;
	mmdc_p0->mpdgctrl1 		= (u32)0x052C0520;
	mmdc_p1->mpdgctrl0 		= (u32)0x4528053C;
	mmdc_p1->mpdgctrl1 		= (u32)0x052C0470;

	mmdc_p0->mprddlctl 		= (u32)0x3E363A40;
	mmdc_p1->mprddlctl 		= (u32)0x403C3246;

	mmdc_p0->mpwrdlctl 		= (u32)0x3A38443C;
	mmdc_p1->mpwrdlctl 		= (u32)0x48364A3E;

	mmdc_p0->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby3dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby3dl 	= (u32)0x33333333;

	mmdc_p0->mpmur0 		= (u32)0x00000800;
	mmdc_p1->mpmur0 		= (u32)0x00000800;
	/* MMDC init:
	  in DDR3, 64-bit mode, only MMDC0 is initiated: */
	mmdc_p0->mdpdc 			= (u32)0x00020036;
	mmdc_p0->mdotc 			= (u32)0x09444040;

	mmdc_p0->mdcfg0 		= (u32)0x555A7974;
	mmdc_p0->mdcfg1 		= (u32)0xDB538F64;
	mmdc_p0->mdcfg2 		= (u32)0x01FF00DB;

	mmdc_p0->mdmisc 		= (u32)0x00001740;
	mmdc_p0->mdscr 			= (u32)0x00000000;

	mmdc_p0->mdrwd 			= (u32)0x000026d2;

	mmdc_p0->mdor 			= (u32)0x005a1023;

	/* 2G
	mmdc_p0->mdasp 			= (u32)0x00000047;
	mmdc_p0->mdctl 			= (u32)0x841a0000;*/

	/* 1G */
	mmdc_p0->mdasp 			= (u32)0x00000027;
	mmdc_p0->mdctl 			= (u32)0x841A0000;

	mmdc_p0->mdscr 			= (u32)0x04088032;
	mmdc_p0->mdscr 			= (u32)0x00008033;
	mmdc_p0->mdscr 			= (u32)0x00048031;
	mmdc_p0->mdscr 			= (u32)0x09408030;
	mmdc_p0->mdscr 			= (u32)0x04008040;

	mmdc_p0->mdref 			= (u32)0x00005800;

	mmdc_p0->mpodtctrl 		= (u32)0x00011117;
	mmdc_p1->mpodtctrl 		= (u32)0x00011117;

	mmdc_p0->mdpdc 			= (u32)0x00025576;
	mmdc_p0->mapsr 			= (u32)0x00011006;
	mmdc_p0->mdscr 			= (u32)0x00000000;
	sdram_size = 1024;

}

static void spl_dram_init_mx6q_2g(void)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	volatile struct mmdc_p_regs *mmdc_p1;
	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;
	mmdc_p1 = (struct mmdc_p_regs *) MMDC_P1_BASE_ADDR;

	/* Calibrations */
	/* ZQ */
	mmdc_p0->mpzqhwctrl 	= (u32)0xa1390003;
	while (mmdc_p0->mpzqhwctrl & 0x00010000)
		;
#ifdef CONFIG_DDR_2GBO
	/* write leveling */
	mmdc_p0->mpwldectrl0 	= (u32)0x001C001C;
	mmdc_p0->mpwldectrl1 	= (u32)0x0026001E;
	mmdc_p1->mpwldectrl0 	= (u32)0x00240030;
	mmdc_p1->mpwldectrl1 	= (u32)0x00150028;
	/* DQS gating, read delay, write delay calibration values
	 based on calibration compare of 0x00ffff00  */
	mmdc_p0->mpdgctrl0 		= (u32)0x43240338;
	mmdc_p0->mpdgctrl1 		= (u32)0x03240318;
	mmdc_p1->mpdgctrl0 		= (u32)0x43380344;
	mmdc_p1->mpdgctrl1 		= (u32)0x03300268;
	
	mmdc_p0->mprddlctl 		= (u32)0x3C323436;
	mmdc_p1->mprddlctl 		= (u32)0x38383444;

	mmdc_p0->mpwrdlctl 		= (u32)0x363A3C36;
	mmdc_p1->mpwrdlctl 		= (u32)0x46344840;
#else
	/* write leveling */
	mmdc_p0->mpwldectrl0 	= (u32)0x001F0019;
	mmdc_p0->mpwldectrl1 	= (u32)0x0024001F;
	mmdc_p1->mpwldectrl0 	= (u32)0x001F002B;
	mmdc_p1->mpwldectrl1 	= (u32)0x00130029;
	/* DQS gating, read delay, write delay calibration values
	 based on calibration compare of 0x00ffff00  */
	mmdc_p0->mpdgctrl0 		= (u32)0x43240334;
	mmdc_p0->mpdgctrl1 		= (u32)0x03200314;
	mmdc_p1->mpdgctrl0 		= (u32)0x432C0340;
	mmdc_p1->mpdgctrl1 		= (u32)0x032C0278;
	
	mmdc_p0->mprddlctl 		= (u32)0x4036363A;
	mmdc_p1->mprddlctl 		= (u32)0x3A363446;

	mmdc_p0->mpwrdlctl 		= (u32)0x38383E3A;
	mmdc_p1->mpwrdlctl 		= (u32)0x46344840;
#endif
	mmdc_p0->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p0->mprddqby3dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby0dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby1dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby2dl 	= (u32)0x33333333;
	mmdc_p1->mprddqby3dl 	= (u32)0x33333333;

	mmdc_p0->mpmur0 		= (u32)0x00000800;
	mmdc_p1->mpmur0 		= (u32)0x00000800;
	/* MMDC init:
	  in DDR3, 64-bit mode, only MMDC0 is initiated: */
	mmdc_p0->mdpdc 			= (u32)0x00020036;
	mmdc_p0->mdotc 			= (u32)0x09444040;

	mmdc_p0->mdcfg0 		= (u32)0x8A8F7955; // 0x555a7975;
	mmdc_p0->mdcfg1 		= (u32)0xFF328F64; // 0xff538f64;
	mmdc_p0->mdcfg2 		= (u32)0x01ff00db;

	mmdc_p0->mdmisc 		= (u32)0x00081740;
	mmdc_p0->mdscr 			= (u32)0x00008000;

	mmdc_p0->mdrwd 			= (u32)0x000026d2;

	mmdc_p0->mdor 			= (u32)0x008F1023; //0x005a1023;
	
	/* 2G */ 
	mmdc_p0->mdasp 			= (u32)0x00000047;
	mmdc_p0->mdctl 			= (u32)0x841a0000;
	
	/* 1G
	mmdc_p0->mdasp 			= (u32)0x00000027;
	mmdc_p0->mdctl 			= (u32)0x831a0000;*/

	mmdc_p0->mdscr 			= (u32)0x04088032;
	mmdc_p0->mdscr 			= (u32)0x00008033;
	mmdc_p0->mdscr 			= (u32)0x00048031;
	mmdc_p0->mdscr 			= (u32)0x09408030;
	mmdc_p0->mdscr 			= (u32)0x04008040;

	mmdc_p0->mdref 			= (u32)0x00005800;
	
	mmdc_p0->mpodtctrl 		= (u32)0x00011117;
	mmdc_p1->mpodtctrl 		= (u32)0x00011117;

	mmdc_p0->mdpdc 			= (u32)0x00025576;
	mmdc_p0->mapsr 			= (u32)0x00011006;
	mmdc_p0->mdscr 			= (u32)0x00000000;
	sdram_size = 2048;
}

static char is_som_solo(void){
int oldbus;
char flag;

	oldbus = i2c_get_bus_num();
	i2c_set_bus_num(1);

	if (i2c_probe(0x8))
		flag = true;
	else
		flag = false;
	i2c_set_bus_num(oldbus);

	return flag;
}

/* 
 * First phase ddr init. Use legacy autodetect
 */
static void legacy_spl_dram_init(void)
{	
	u32 cpurev, imxtype;

	cpurev = get_cpu_rev();
	imxtype = (cpurev & 0xFF000) >> 12;

	get_imx_type(imxtype);

	switch (imxtype){
	case MXC_CPU_MX6SOLO:
		spl_mx6dlsl_dram_setup_iomux();
		spl_dram_init_mx6solo_1gb();
		ram_size();
		if (sdram_size == 512){
			p_udelay(1000);
			reset_ddr_solo();
			spl_mx6dlsl_dram_setup_iomux();
			spl_dram_init_mx6solo_512mb();
			p_udelay(1000);
		}
		break;
	case MXC_CPU_MX6Q:
		if (is_cpu_pop_package()) {
			load_custom_data(MX6Q_MMDC_LPDDR2_register_programming_aid_v0_Micron_InterL_RamValues);
			setup_ddr_parameters((struct eeprom_command_type *)MX6Q_MMDC_LPDDR2_register_programming_aid_v0_Micron_InterL_commands);
			sdram_size = 1024;
		} else {
			spl_mx6qd_dram_setup_iomux();
#ifdef CONFIG_DDR_2GBO
			spl_dram_init_mx6q_2g();
#else
			if (check_1_2G_only())
				spl_dram_init_mx6q_2g();
			else
				spl_dram_init_mx6q_1g();
#endif
			ram_size();
		}
		break;
	case MXC_CPU_MX6D:
		if (is_cpu_pop_package()) {
			load_custom_data(MX6Q_MMDC_LPDDR2_register_programming_aid_v0_Micron_InterL_RamValues);
			setup_ddr_parameters((struct eeprom_command_type *)MX6Q_MMDC_LPDDR2_register_programming_aid_v0_Micron_InterL_commands);
			sdram_size = 1024;
		} else {
			spl_dram_init_mx6q_1g();
			ram_size();
		}
		break;
	case MXC_CPU_MX6DL:
	default:
		spl_mx6dlsl_dram_setup_iomux();
		if (is_som_solo())
			spl_dram_init_mx6solo_1gb();
		else
 			spl_dram_init_mx6dl_1g();
		ram_size();
		break;	
	}
}

/* 
 * Second phase ddr init. Use eeprom values.
 */
static int  spl_dram_init(void)
{
	u32 cpurev, imxtype;
	bool b_is_valid_eeprom_cfg_struct = false;
	int ret;

	cpurev = get_cpu_rev();
	imxtype = (cpurev & 0xFF000) >> 12;

	get_imx_type(imxtype);

	/* Add here: Read EEPROM and parse Variscite struct */
	var_eeprom_i2c_init();
	memset(&g_var_eeprom_cfg, 0x00, sizeof(var_eeprom_config_struct_t));
	ret = var_eeprom_read_struct(&g_var_eeprom_cfg);
	if (ret)
		return SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM;

	/* is valid Variscite EEPROM? */
	b_is_valid_eeprom_cfg_struct = var_eeprom_is_valid(&g_var_eeprom_cfg);
	if (!b_is_valid_eeprom_cfg_struct)
		return SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM_STRUCT_DETECTED;

        switch (imxtype) {
        case MXC_CPU_MX6DL:
        case MXC_CPU_MX6SOLO:
               	var_eeprom_mx6dlsl_dram_setup_iomux_from_struct(&g_var_eeprom_cfg.pinmux_group);
                break;
        case MXC_CPU_MX6Q:
        case MXC_CPU_MX6D:
        default:
                var_eeprom_mx6qd_dram_setup_iomux_from_struct(&g_var_eeprom_cfg.pinmux_group);
                break;  
        }

	var_eeprom_dram_init_from_struct(&g_var_eeprom_cfg);

	sdram_size = g_var_eeprom_cfg.header.ddr_size;
	g_b_dram_set_by_var_eeprom_config = true;

	return SPL_DRAM_INIT_STATUS_OK;
}

/* 
 * Second phase ddr init. Use eeprom values.
 */
static int  spl_dram_init_v2(void)
{
	u32 cpurev, imxtype;
	bool b_is_valid_eeprom_cfg_struct = false;
	int ret;

	cpurev = get_cpu_rev();
	imxtype = (cpurev & 0xFF000) >> 12;

	get_imx_type(imxtype);

	/* Add here: Read EEPROM and parse Variscite struct */
	memset(&var_eeprom_config_struct_v2, 0x00, sizeof(var_eeprom_config_struct_v2));
	
	if (is_cpu_pop_package())
		ret = var_eeprom_v2_read_struct(&var_eeprom_config_struct_v2,0x52);
	else
		ret = var_eeprom_v2_read_struct(&var_eeprom_config_struct_v2,0x56);
	
	if (ret)
		return SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM;

	if(var_eeprom_config_struct_v2.variscite_magic!=0x32524156) //Test for VAR2 in the header.
		return SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM_STRUCT_DETECTED;
		
	handle_eeprom_data(&var_eeprom_config_struct_v2);
	
	sdram_size = var_eeprom_config_struct_v2.ddr_size*128;
	g_b_dram_set_by_var_eeprom_config = true;

	return SPL_DRAM_INIT_STATUS_OK;
}

/* 
 * board dram init legacy or eeprom.
 */
static int spl_status;

void board_dram_init(void)
{
	/* Initialize DDR based on eeprom if exist */
	var_setup_iomux_per_vcc_en();
	/* Reset CODEC to allow i2c communication */
	codec_reset(1);
	udelay(1000 * 50);
	eeprom_revision=1;	
	spl_status = spl_dram_init();
	if (spl_status != SPL_DRAM_INIT_STATUS_OK)
	{
		spl_status=spl_dram_init_v2();
		if(spl_status != SPL_DRAM_INIT_STATUS_OK)
		{
			legacy_spl_dram_init();
			eeprom_revision=0;
		}
		else
			eeprom_revision=2;	
	}

	spl_mx6qd_dram_setup_iomux_check_reset();
}

static void disable_wdog(void)
{
	struct wdog_regs *wdog1 = (struct wdog_regs *)WDOG1_BASE_ADDR;
	struct wdog_regs *wdog2 = (struct wdog_regs *)WDOG2_BASE_ADDR;

	writew(0x73, &wdog1->wcr);
	writew(0x73, &wdog2->wcr);
}
#

/* 
 * board init callback.
 */

void board_init_f(ulong dummy)
{	
	/* Set the stack pointer. */
	asm volatile("mov sp, %0\n" : : "r"(CONFIG_SPL_STACK));

	arch_cpu_init();
	disable_wdog();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* Set global data pointer. */
	gd = &gdata;

	timer_init();

	/* Initialize DDR based on eeprom if exist */
	board_dram_init();

	board_init_r(NULL, 0);
}

void spl_board_init(void)
{
	setup_boot_device();
}

u32 spl_boot_device(void)
{
	u32 imxtype, cpurev;
	u32 *sdram_global;
	

	/* Set global data pointer. */
	gd = &gdata;

	board_early_init_f();	

	p_udelay(1000);

	mem_malloc_init(CONFIG_SYS_SPL_MALLOC_DDR_START, 
			CONFIG_SYS_SPL_MALLOC_DDR_SIZE);

	preloader_console_init();

	cpurev = get_cpu_rev();
	imxtype = (cpurev & 0xFF000) >> 12;
	printf("i.MX%s SOC ", get_imx_type(imxtype));
	if (is_cpu_pop_package())
		printf("P-POP ");
	else
		printf("P-STD ");


	if (spl_status ==SPL_DRAM_INIT_STATUS_OK	) {
		printf("DDR EEPROM configuration\n");
	if(eeprom_revision==1)	
		var_eeprom_strings_print(&g_var_eeprom_cfg);
	if(eeprom_revision==2)	
	{
		var_eeprom_config_struct_v2.part_number[sizeof(var_eeprom_config_struct_v2.part_number)-1] = (u8)0x00;
		var_eeprom_config_struct_v2.Assembly[sizeof(var_eeprom_config_struct_v2.Assembly)-1] = (u8)0x00;
		var_eeprom_config_struct_v2.date[sizeof(var_eeprom_config_struct_v2.date)-1] = (u8)0x00;

		printf("Part number: %s\n", (char *)var_eeprom_config_struct_v2.part_number);
		printf("Assembly: %s\n", (char *)var_eeprom_config_struct_v2.Assembly);
		printf("Date of production: %s\n", (char *)var_eeprom_config_struct_v2.date);
	}
		
	} else {
		printf("DDR LEGACY configuration\n");
		ram_size();
	}
#ifdef EEPROM_DEBUG
	/* Test SDRAM size... */
	sdram_long_test_spl(800);
#endif
	printf("Ram size %ld\n", sdram_size);
	sdram_global =  (u32 *)0x917000;
	*sdram_global = sdram_size;
	printf("Boot Device : ");
	switch (get_boot_device()) {
	case MX6_SD0_BOOT:
		printf("MMC0\n");
		return BOOT_DEVICE_MMC1;
	case MX6_SD1_BOOT:
		printf("MMC1\n");
		return BOOT_DEVICE_MMC2;
	case MX6_MMC_BOOT:
		printf("MMC\n");
		return BOOT_DEVICE_MMC1;
	case MX6_NAND_BOOT:
		printf("NAND\n");
		return BOOT_DEVICE_NAND;
	case MX6_SATA_BOOT:
		printf("SATA\n");
		return BOOT_DEVICE_SATA;
	case MX6_UNKNOWN_BOOT:
	default:
		printf("UNKNOWN\n");
		return BOOT_DEVICE_NONE;
	}

}

u32 spl_boot_mode(void)
{
	switch (spl_boot_device()) {
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		return MMCSD_MODE;
		break;
	case BOOT_DEVICE_SATA:
		return SATA_MODE;
		break;
	case BOOT_DEVICE_NAND:
		return NAND_MODE;
		break;
	default:
		puts("spl: ERROR:  unsupported device\n");
		hang();
	}
}

void reset_cpu(ulong addr)
{
	struct wdog_regs *wdog1 = (struct wdog_regs *)WDOG1_BASE_ADDR;

	writew(4, &wdog1->wcr);
//	__REG16(WDOG1_BASE_ADDR) = 4;
}
#endif

