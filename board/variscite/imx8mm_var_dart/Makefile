#
# Copyright 2018 NXP
# Copyright 2019 Variscite Ltd.
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += ../common/imx8m_eeprom.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
else
obj-y += imx8mm_var_dart.o ../common/mmc.o
endif
