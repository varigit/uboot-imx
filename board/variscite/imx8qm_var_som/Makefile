#
# Copyright 2017 NXP
# Copyright 2019-2020 Variscite Ltd.
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += imx8qm_var_som.o ../common/mmc.o ../common/imx8_eeprom.o
obj-$(CONFIG_VIDEO) += ../common/video.o
obj-$(CONFIG_FEC_MXC) += ../common/eth.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
endif

