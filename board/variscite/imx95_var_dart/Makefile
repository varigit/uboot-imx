#
# Copyright 2023 NXP
# Copyright 2024 Variscite Ltd.
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx95_var_dart.o
obj-$(CONFIG_TARGET_IMX95_VAR_DART) += ../common/imx9_eeprom.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
else
obj-y += ../common/eth.o
obj-y += ../common/mmc.o
obj-$(CONFIG_EXTCON_PTN5150) += ../common/extcon-ptn5150.o
endif
