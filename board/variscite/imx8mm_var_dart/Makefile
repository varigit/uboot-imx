#
# Copyright 2018 NXP
# Copyright 2019 Variscite Ltd.
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += ../common/imx8_eeprom.o
obj-y += imx8mm_var_dart.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
obj-$(CONFIG_IMX8M_DDR4) += ddr4_timing.o
else
obj-y += ../common/mmc.o
obj-$(CONFIG_VIDEO) += ../common/video.o
obj-$(CONFIG_FEC_MXC) += ../common/eth.o
obj-$(CONFIG_EXTCON_PTN5150) += ../common/extcon-ptn5150.o
obj-$(CONFIG_IMX8M_BOARD_INIT_DRAM) += ../common/imx8_dram.o
endif
