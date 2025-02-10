/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 */
#ifndef _ASM_ARCH_GRF_RK3128_H
#define _ASM_ARCH_GRF_RK3128_H

struct rk3128_grf {
	unsigned int reserved[0x2a];
	unsigned int gpio0a_iomux;
	unsigned int gpio0b_iomux;
	unsigned int gpio0c_iomux;
	unsigned int gpio0d_iomux;
	unsigned int gpio1a_iomux;
	unsigned int gpio1b_iomux;
	unsigned int gpio1c_iomux;
	unsigned int gpio1d_iomux;
	unsigned int gpio2a_iomux;
	unsigned int gpio2b_iomux;
	unsigned int gpio2c_iomux;
	unsigned int gpio2d_iomux;
	unsigned int gpio3a_iomux;
	unsigned int gpio3b_iomux;
	unsigned int gpio3c_iomux;
	unsigned int gpio3d_iomux;
	unsigned int gpio2c_iomux2;
	unsigned int grf_cif_iomux;
	unsigned int grf_cif_iomux1;
	unsigned int reserved1[(0x118 - 0xf0) / 4 - 1];
	unsigned int gpio0l_pull;
	unsigned int gpio0h_pull;
	unsigned int gpio1l_pull;
	unsigned int gpio1h_pull;
	unsigned int gpio2l_pull;
	unsigned int gpio2h_pull;
	unsigned int gpio3l_pull;
	unsigned int gpio3h_pull;
	unsigned int reserved2;
	unsigned int acodec_con;
	unsigned int soc_con0;
	unsigned int soc_con1;
	unsigned int soc_con2;
	unsigned int soc_status0;
	unsigned int reserved3[6];
	unsigned int mac_con0;
	unsigned int mac_con1;
	unsigned int reserved4[3];
	unsigned int uoc0_con0;
	unsigned int reserved5;
	unsigned int uoc1_con1;
	unsigned int uoc1_con2;
	unsigned int uoc1_con3;
	unsigned int uoc1_con4;
	unsigned int uoc1_con5;
	unsigned int reserved6;
	unsigned int ddrc_stat;
	unsigned int reserved9;
	unsigned int soc_status1;
	unsigned int cpu_con0;
	unsigned int cpu_con1;
	unsigned int cpu_con2;
	unsigned int cpu_con3;
	unsigned int reserved10;
	unsigned int reserved11;
	unsigned int cpu_status0;
	unsigned int cpu_status1;
	unsigned int os_reg[8];
	unsigned int reserved12[(0x280 - 0x1e4) / 4 - 1];
	unsigned int usbphy0_con[8];
	unsigned int usbphy1_con[8];
	unsigned int uoc_status0;
	unsigned int reserved13[(0x300 - 0x2c0) / 4 - 1];
	unsigned int chip_tag;
	unsigned int sdmmc_det_cnt;
};
check_member(rk3128_grf, sdmmc_det_cnt, 0x304);

struct rk3128_pmu {
	unsigned int wakeup_cfg;
	unsigned int pwrdn_con;
	unsigned int pwrdn_st;
	unsigned int idle_req;
	unsigned int idle_st;
	unsigned int pwrmode_con;
	unsigned int pwr_state;
	unsigned int osc_cnt;
	unsigned int core_pwrdwn_cnt;
	unsigned int core_pwrup_cnt;
	unsigned int sft_con;
	unsigned int ddr_sref_st;
	unsigned int int_con;
	unsigned int int_st;
	unsigned int sys_reg[4];
};
check_member(rk3128_pmu, int_st, 0x34);

#endif
