/*
 *  Copyright (C) 2014 Gateworks Corporation
 *  Tim Harvey <tharvey@gateworks.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __PFUZE100_PMIC_H_
#define __PFUZE100_PMIC_H_

/* Device ID */
enum {PFUZE100 = 0x10, PFUZE200 = 0x11, PFUZE3000 = 0x30};

#define PFUZE100_REGULATOR_DRIVER	"pfuze100_regulator"

/* PFUZE100 registers */
enum {
	PFUZE100_DEVICEID	= 0x00,
	PFUZE100_REVID		= 0x03,
	PFUZE100_FABID		= 0x04,

	PFUZE100_SW1ABVOL	= 0x20,
	PFUZE100_SW1ABSTBY	= 0x21,
	PFUZE100_SW1ABOFF	= 0x22,
	PFUZE100_SW1ABMODE	= 0x23,
	PFUZE100_SW1ABCONF	= 0x24,
	PFUZE100_SW1CVOL	= 0x2e,
	PFUZE100_SW1CSTBY	= 0x2f,
	PFUZE100_SW1COFF	= 0x30,
	PFUZE100_SW1CMODE	= 0x31,
	PFUZE100_SW1CCONF	= 0x32,
	PFUZE100_SW2VOL		= 0x35,
	PFUZE100_SW2STBY	= 0x36,
	PFUZE100_SW2OFF		= 0x37,
	PFUZE100_SW2MODE	= 0x38,
	PFUZE100_SW2CONF	= 0x39,
	PFUZE100_SW3AVOL	= 0x3c,
	PFUZE100_SW3ASTBY	= 0x3D,
	PFUZE100_SW3AOFF	= 0x3E,
	PFUZE100_SW3AMODE	= 0x3F,
	PFUZE100_SW3ACONF	= 0x40,
	PFUZE100_SW3BVOL	= 0x43,
	PFUZE100_SW3BSTBY	= 0x44,
	PFUZE100_SW3BOFF	= 0x45,
	PFUZE100_SW3BMODE	= 0x46,
	PFUZE100_SW3BCONF	= 0x47,
	PFUZE100_SW4VOL		= 0x4a,
	PFUZE100_SW4STBY	= 0x4b,
	PFUZE100_SW4OFF		= 0x4c,
	PFUZE100_SW4MODE	= 0x4d,
	PFUZE100_SW4CONF	= 0x4e,
	PFUZE100_SWBSTCON1	= 0x66,
	PFUZE100_VREFDDRCON	= 0x6a,
	PFUZE100_VSNVSVOL	= 0x6b,
	PFUZE100_VGEN1VOL	= 0x6c,
	PFUZE100_VGEN2VOL	= 0x6d,
	PFUZE100_VGEN3VOL	= 0x6e,
	PFUZE100_VGEN4VOL	= 0x6f,
	PFUZE100_VGEN5VOL	= 0x70,
	PFUZE100_VGEN6VOL	= 0x71,

	PFUZE100_NUM_OF_REGS	= 0x7f,
};

/* Registor offset based on VOLT register */
#define PFUZE100_VOL_OFFSET	0
#define PFUZE100_STBY_OFFSET	1
#define PFUZE100_OFF_OFFSET	2
#define PFUZE100_MODE_OFFSET	3
#define PFUZE100_CONF_OFFSET	4

/*
 * Buck Regulators
 */

#define SW_MODE_MASK	0xf
#define SW_MODE_SHIFT	0

/* SW1A/B/C */
#define PFUZE100_SW1ABC_SETP(x)	((x - 3000) / 250)

/* Output Voltage Configuration */
#define SW1x_0_300V 0
#define SW1x_0_325V 1
#define SW1x_0_350V 2
#define SW1x_0_375V 3
#define SW1x_0_400V 4
#define SW1x_0_425V 5
#define SW1x_0_450V 6
#define SW1x_0_475V 7
#define SW1x_0_500V 8
#define SW1x_0_525V 9
#define SW1x_0_550V 10
#define SW1x_0_575V 11
#define SW1x_0_600V 12
#define SW1x_0_625V 13
#define SW1x_0_650V 14
#define SW1x_0_675V 15
#define SW1x_0_700V 16
#define SW1x_0_725V 17
#define SW1x_0_750V 18
#define SW1x_0_775V 19
#define SW1x_0_800V 20
#define SW1x_0_825V 21
#define SW1x_0_850V 22
#define SW1x_0_875V 23
#define SW1x_0_900V 24
#define SW1x_0_925V 25
#define SW1x_0_950V 26
#define SW1x_0_975V 27
#define SW1x_1_000V 28
#define SW1x_1_025V 29
#define SW1x_1_050V 30
#define SW1x_1_075V 31
#define SW1x_1_100V 32
#define SW1x_1_125V 33
#define SW1x_1_150V 34
#define SW1x_1_175V 35
#define SW1x_1_200V 36
#define SW1x_1_225V 37
#define SW1x_1_250V 38
#define SW1x_1_275V 39
#define SW1x_1_300V 40
#define SW1x_1_325V 41
#define SW1x_1_350V 42
#define SW1x_1_375V 43
#define SW1x_1_400V 44
#define SW1x_1_425V 45
#define SW1x_1_450V 46
#define SW1x_1_475V 47
#define SW1x_1_500V 48
#define SW1x_1_525V 49
#define SW1x_1_550V 50
#define SW1x_1_575V 51
#define SW1x_1_600V 52
#define SW1x_1_625V 53
#define SW1x_1_650V 54
#define SW1x_1_675V 55
#define SW1x_1_700V 56
#define SW1x_1_725V 57
#define SW1x_1_750V 58
#define SW1x_1_775V 59
#define SW1x_1_800V 60
#define SW1x_1_825V 61
#define SW1x_1_850V 62
#define SW1x_1_875V 63

#define SW1x_NORMAL_MASK  0x3f
#define SW1x_STBY_MASK    0x3f
#define SW1x_OFF_MASK     0x3f

#define SW1xCONF_DVSSPEED_MASK 0xc0
#define SW1xCONF_DVSSPEED_2US  0x00
#define SW1xCONF_DVSSPEED_4US  0x40
#define SW1xCONF_DVSSPEED_8US  0x80
#define SW1xCONF_DVSSPEED_16US 0xc0

/* SW2,SW3A/B and SW4 */

/*
 * PFUZE100_SWxVOL[bit 6] == 0 -> Low Range used
 * PFUZE100_SWxVOL[bit 6] == 1 -> High Range used
 *
 * In order to optimize the performance:
 * 0.4V to 1.975V should be used in Low Range, and
 * 2.0V to 3.300V should be used in the High Range
 */
#define PFUZE100_SWx_LR_SETP(x)	((x - 4000) / 250)
#define PFUZE100_SWx_HR_SETP(x)	(((x - 8000) / 500) + 64)

/* Output Voltage Configuration */

/* Low output voltage range */
#define SWx_LR_0_400V 0
#define SWx_LR_0_425V 1
#define SWx_LR_0_450V 2
#define SWx_LR_0_475V 3
#define SWx_LR_0_500V 4
#define SWx_LR_0_525V 5
#define SWx_LR_0_550V 6
#define SWx_LR_0_575V 7
#define SWx_LR_0_600V 8
#define SWx_LR_0_625V 9
#define SWx_LR_0_650V 10
#define SWx_LR_0_675V 11
#define SWx_LR_0_700V 12
#define SWx_LR_0_725V 13
#define SWx_LR_0_750V 14
#define SWx_LR_0_775V 15
#define SWx_LR_0_800V 16
#define SWx_LR_0_825V 17
#define SWx_LR_0_850V 18
#define SWx_LR_0_875V 19
#define SWx_LR_0_900V 20
#define SWx_LR_0_925V 21
#define SWx_LR_0_950V 22
#define SWx_LR_0_975V 23
#define SWx_LR_1_000V 24
#define SWx_LR_1_025V 25
#define SWx_LR_1_050V 26
#define SWx_LR_1_075V 27
#define SWx_LR_1_100V 28
#define SWx_LR_1_125V 29
#define SWx_LR_1_150V 30
#define SWx_LR_1_175V 31
#define SWx_LR_1_200V 32
#define SWx_LR_1_225V 33
#define SWx_LR_1_250V 34
#define SWx_LR_1_275V 35
#define SWx_LR_1_300V 36
#define SWx_LR_1_325V 37
#define SWx_LR_1_350V 38
#define SWx_LR_1_375V 39
#define SWx_LR_1_400V 40
#define SWx_LR_1_425V 41
#define SWx_LR_1_450V 42
#define SWx_LR_1_475V 43
#define SWx_LR_1_500V 44
#define SWx_LR_1_525V 45
#define SWx_LR_1_550V 46
#define SWx_LR_1_575V 47
#define SWx_LR_1_600V 48
#define SWx_LR_1_625V 49
#define SWx_LR_1_650V 50
#define SWx_LR_1_675V 51
#define SWx_LR_1_700V 52
#define SWx_LR_1_725V 53
#define SWx_LR_1_750V 54
#define SWx_LR_1_775V 55
#define SWx_LR_1_800V 56
#define SWx_LR_1_825V 57
#define SWx_LR_1_850V 58
#define SWx_LR_1_875V 59
#define SWx_LR_1_900V 60
#define SWx_LR_1_925V 61
#define SWx_LR_1_950V 62
#define SWx_LR_1_975V 63

/* High output voltage range */
#define SWx_HR_0_800V 64
#define SWx_HR_0_850V 65
#define SWx_HR_0_900V 66
#define SWx_HR_0_950V 67
#define SWx_HR_1_000V 68
#define SWx_HR_1_050V 69
#define SWx_HR_1_100V 70
#define SWx_HR_1_150V 71
#define SWx_HR_1_200V 72
#define SWx_HR_1_250V 73
#define SWx_HR_1_300V 74
#define SWx_HR_1_350V 75
#define SWx_HR_1_400V 76
#define SWx_HR_1_450V 77
#define SWx_HR_1_500V 78
#define SWx_HR_1_550V 79
#define SWx_HR_1_600V 80
#define SWx_HR_1_650V 81
#define SWx_HR_1_700V 82
#define SWx_HR_1_750V 83
#define SWx_HR_1_800V 84
#define SWx_HR_1_850V 85
#define SWx_HR_1_900V 86
#define SWx_HR_1_950V 87
#define SWx_HR_2_000V 88
#define SWx_HR_2_050V 89
#define SWx_HR_2_100V 90
#define SWx_HR_2_150V 91
#define SWx_HR_2_200V 92
#define SWx_HR_2_250V 93
#define SWx_HR_2_300V 94
#define SWx_HR_2_350V 95
#define SWx_HR_2_400V 96
#define SWx_HR_2_450V 97
#define SWx_HR_2_500V 98
#define SWx_HR_2_550V 99
#define SWx_HR_2_600V 100
#define SWx_HR_2_650V 101
#define SWx_HR_2_700V 102
#define SWx_HR_2_750V 103
#define SWx_HR_2_800V 104
#define SWx_HR_2_850V 105
#define SWx_HR_2_900V 106
#define SWx_HR_2_950V 107
#define SWx_HR_3_000V 108
#define SWx_HR_3_050V 109
#define SWx_HR_3_100V 110
#define SWx_HR_3_150V 111
#define SWx_HR_3_200V 112
#define SWx_HR_3_250V 113
#define SWx_HR_3_300V 114

/* Note that bit 6 is read only */
#define SWx_NORMAL_MASK	0x7f
#define SWx_STBY_MASK	0x7f
#define SWx_OFF_MASK	0x7f

#define SWxCONF_DVSSPEED_MASK	0xc0
#define SWxCONF_DVSSPEED_4US	0x00
#define SWxCONF_DVSSPEED_8US	0x40
#define SWxCONF_DVSSPEED_16US	0x80
#define SWxCONF_DVSSPEED_32US	0xc0

/*
 * LDO Configuration
 */

/* VGEN1/2 Voltage Configuration */
#define LDOA_0_80V	0
#define LDOA_0_85V	1
#define LDOA_0_90V	2
#define LDOA_0_95V	3
#define LDOA_1_00V	4
#define LDOA_1_05V	5
#define LDOA_1_10V	6
#define LDOA_1_15V	7
#define LDOA_1_20V	8
#define LDOA_1_25V	9
#define LDOA_1_30V	10
#define LDOA_1_35V	11
#define LDOA_1_40V	12
#define LDOA_1_45V	13
#define LDOA_1_50V	14
#define LDOA_1_55V	15

/* VGEN3/4/5/6 Voltage Configuration */
#define LDOB_1_80V	0
#define LDOB_1_90V	1
#define LDOB_2_00V	2
#define LDOB_2_10V	3
#define LDOB_2_20V	4
#define LDOB_2_30V	5
#define LDOB_2_40V	6
#define LDOB_2_50V	7
#define LDOB_2_60V	8
#define LDOB_2_70V	9
#define LDOB_2_80V	10
#define LDOB_2_90V	11
#define LDOB_3_00V	12
#define LDOB_3_10V	13
#define LDOB_3_20V	14
#define LDOB_3_30V	15

#define LDO_VOL_MASK	0xf
#define LDO_EN		(1 << 4)

/*
 * LDO Mode Control
 *
 * VGENxCTL[4]
 * Mode		|      value
 *   ON   		0x0
 *   OFF		0x1
 */
#define LDO_MODE_SHIFT	4
#define LDO_MODE_MASK	(1 << 4)
#define LDO_MODE_OFF	0
#define LDO_MODE_ON	1

/*
 * LDO "Extended" Mode Control
 *
 * If LDO Mode (VGENxCTL[4]) is ON:
 *
 * VGENxCTL[6:5]
 * Normal Mode   |   Standby Mode   |   value
 *   ON   		ON		0x0
 *   ON			OFF		0x1
 *   LOW POWER		LOW POWER	0x2
 *   ON			LOW POWER	0x3
 *
 *
 * If LDO Mode (VGENxCTL[4]) is OFF:
 *
 * VGENxCTL[6:5]
 * Normal Mode  |  Standby Mode	|      value
 *   OFF		OFF		Irrelevant
 *
 */
#define LDO_STBY		(1 << 5)
#define LDO_LPWR		(1 << 6)
#define LDO_EXT_MODE_SHIFT	5
#define LDO_EXT_MODE_MASK	(3 << 5)
#define LDO_EXT_MODE_ON_ON	0
#define LDO_EXT_MODE_ON_OFF	1
#define LDO_EXT_MODE_LPM_LPM	2
#define LDO_EXT_MODE_ON_LPM	3

#define VREFDDRCON_EN	(1 << 4)
/*
 * Boost Regulator
 */

/* SWBST Output Voltage */
#define SWBST_5_00V	0
#define SWBST_5_05V	1
#define SWBST_5_10V	2
#define SWBST_5_15V	3

#define SWBST_VOL_MASK	0x3
#define SWBST_MODE_MASK	0xC
#define SWBST_MODE_SHIFT 0x2
#define SWBST_MODE_OFF	0
#define SWBST_MODE_PFM	1
#define SWBST_MODE_AUTO	2
#define SWBST_MODE_APS	3

/*
 * Regulator Mode Control
 *
 * OFF: The regulator is switched off and the output voltage is discharged.
 * PFM: In this mode, the regulator is always in PFM mode, which is useful
 *      at light loads for optimized efficiency.
 * PWM: In this mode, the regulator is always in PWM mode operation
 *	regardless of load conditions.
 * APS: In this mode, the regulator moves automatically between pulse
 *	skipping mode and PWM mode depending on load conditions.
 *
 * SWxMODE[3:0]
 * Normal Mode  |  Standby Mode	|      value
 *   OFF		OFF		0x0
 *   PWM		OFF		0x1
 *   PFM		OFF		0x3
 *   APS		OFF		0x4
 *   PWM		PWM		0x5
 *   PWM		APS		0x6
 *   APS		APS		0x8
 *   APS		PFM		0xc
 *   PWM		PFM		0xd
 */
#define OFF_OFF		0x0
#define PWM_OFF		0x1
#define PFM_OFF		0x3
#define APS_OFF		0x4
#define PWM_PWM		0x5
#define PWM_APS		0x6
#define APS_APS		0x8
#define APS_PFM		0xc
#define PWM_PFM		0xd

#define SWITCH_SIZE	0x7

int power_pfuze100_init(unsigned char bus);
#endif
