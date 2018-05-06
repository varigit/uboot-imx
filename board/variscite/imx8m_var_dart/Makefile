#
# Copyright 2018 Variscite Ltd
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8m_var_dart.o eeprom.o

obj-$(CONFIG_POWER_PFUZE100) += ../../freescale/common/pfuze.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-y += ddr/ddr_init.o ddr/ddrphy_train.o ddr/helper.o
endif
