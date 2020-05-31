#
# Copyright 2018 NXP
# Copyright 2019 Variscite Ltd.
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += ../common/imx8_eeprom.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_DDR4) += ddr4_timing.o
else
obj-y += imx8mn_var_som.o ../common/mmc.o
obj-$(CONFIG_VIDEO) += ../common/video.o
obj-$(CONFIG_FEC_MXC) += ../common/eth.o
endif
