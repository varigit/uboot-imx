#
# Copyright 2019 Variscite Ltd.
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += imx8qxp_var_som.o ../common/mmc.o ../common/imx8_eeprom.o ../common/imx8_dram.o
obj-$(CONFIG_VIDEO) += ../common/video.o
obj-$(CONFIG_FEC_MXC) += ../common/eth.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
endif
