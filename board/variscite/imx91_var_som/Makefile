#
# Copyright 2024 NXP
# Copyright 2025 Variscite Ltd.
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx91_var_som.o
obj-$(CONFIG_TARGET_IMX91_VAR_SOM) += ../common/imx9_eeprom.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_TARGET_IMX91_VAR_SOM) += lpddr4_timing.o
else
obj-y += ../common/eth.o
obj-y += ../common/mmc.o
obj-$(CONFIG_EXTCON_PTN5150) += ../common/extcon-ptn5150.o
endif
