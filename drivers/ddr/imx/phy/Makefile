#
# Copyright 2018 NXP
#
# SPDX-License-Identifier:	GPL-2.0+
#

ifdef CONFIG_SPL_BUILD
obj-$(CONFIG_IMX_SNPS_DDR_PHY) += helper.o ddrphy_utils.o ddrphy_train.o ddrphy_csr.o
ifeq ($(CONFIG_IMX93)$(CONFIG_IMX91), y)
obj-$(CONFIG_IMX_SNPS_DDR_PHY_QB_GEN) += ddrphy_qb_gen.o ddrphy_qb_errata.o
obj-$(CONFIG_IMX_SNPS_DDR_PHY_QB) += ddrphy_qb.o ddrphy_qb_errata.o
endif
endif
