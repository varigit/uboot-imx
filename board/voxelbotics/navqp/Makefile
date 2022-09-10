#
# Copyright 2023 Voxelbotics
# Copyright 2019,2024 NXP
#
# SPDX-License-Identifier:      GPL-2.0+

obj-y += navqp.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_micron_8gb.o
endif
