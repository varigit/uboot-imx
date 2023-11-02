#
# Copyright 2019 NXP
# Copyright 2020-2023 Variscite Ltd.
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mp_var_dart.o ../common/imx8_eeprom.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
else
obj-y += ../common/mmc.o
obj-$(CONFIG_FEC_MXC) += ../common/eth.o
obj-$(CONFIG_EXTCON_PTN5150) += ../common/extcon-ptn5150.o
obj-$(CONFIG_IMX8M_BOARD_INIT_DRAM) += ../common/imx8_dram.o
endif
