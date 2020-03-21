#
# Copyright 2018-2019 Variscite Ltd
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += ../common/imx8_eeprom.o usb.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_b0.o lpddr4_timing.o lpddr4_timing_default.o
obj-$(CONFIG_POWER_PFUZE100) += ../../freescale/common/pfuze.o
else
obj-y += imx8mq_var_dart.o ../common/mmc.o
obj-$(CONFIG_VIDEO) += ../common/video.o
obj-$(CONFIG_FEC_MXC) += ../common/eth.o
endif
