// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd.
 */

#include <dm.h>
#include <ram.h>
#include <debug_uart.h>
#include <syscon.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/cru_rk3128.h>
#include <asm/arch-rockchip/grf_rk3128.h>
#include <asm/arch-rockchip/hardware.h>
#include <asm/arch-rockchip/sdram.h>
#include <asm/arch-rockchip/sdram_rk3128.h>

#include <linux/delay.h>

struct dram_info {
#if IS_ENABLED(CONFIG_TPL_BUILD)
	struct rk3128_cru *cru;
	struct rk3128_ddr_pctl *pctl;
	struct rk3128_ddr_timing *ddr_timing;
	struct rk3128_service_sys *axi_bus;
	struct sdram_cap_info cap_info;
	void __iomem *phy_base;
#else
	struct ram_info info;
#endif
	struct rk3128_grf *grf;
};

#if IS_ENABLED(CONFIG_TPL_BUILD)

#define CPU_AXI_BUS_BASE	0x10128000
#define CRU_BASE		0x20000000
#define GRF_BASE		0x20008000
#define DDR_PCTL_BASE		0x20004000
#define DDR_PHY_BASE		0x2000a000

#define AXI_VIO_QOS_PRIO	CPU_AXI_BUS_BASE + 0x7188
#define AXI_VIP_QOS_PRIO	CPU_AXI_BUS_BASE + 0x7208

// from ddr_rk3126.c
#define READLATENCY		0x3f
#define PHY_DLL_DISABLE_FREQ	266
#define PHY_CLK_DRV		PHY_RON_44OHM
#define PHY_CMD_DRV		PHY_RON_44OHM
#define PHY_DQS_DRV		PHY_RON_44OHM

struct rk3128_ddr_timing rk3128_ddr_timings[] = {
#if IS_ENABLED(CONFIG_RAM_ROCKCHIP_LPDDR2)
# include	"sdram-rk3128-lpddr2.inc"
#else
# include	"sdram-rk3128-ddr3.inc"
#endif
};

/*
 * from RK312X TRM "7.2.3 Memory Scheduler"
 *
 * [7:6]  bank(n:n bit bank)
 * [5:4]  row(12+n)
 * [3]    cs(0:1 cs, 1:2 cs)
 * [2:1]  bank(n:n bit bank)
 * [0]    col(9+n)
 */
static const char ddr_cfg_2_rbc[] = {
	((0 << 6) | (0 << 4) | (0 << 3) | (1 << 2) | 1),
	((0 << 6) | (1 << 4) | (0 << 3) | (1 << 2) | 1),
	((0 << 6) | (2 << 4) | (0 << 3) | (1 << 2) | 1),
	((0 << 6) | (3 << 4) | (0 << 3) | (1 << 2) | 1),
	((0 << 6) | (1 << 4) | (0 << 3) | (1 << 2) | 2),
	((0 << 6) | (2 << 4) | (0 << 3) | (1 << 2) | 2),
	((0 << 6) | (3 << 4) | (0 << 3) | (1 << 2) | 2),
	((0 << 6) | (0 << 4) | (0 << 3) | (1 << 2) | 0),
	((0 << 6) | (1 << 4) | (0 << 3) | (1 << 2) | 0),
	((0 << 6) | (2 << 4) | (0 << 3) | (1 << 2) | 0),
	((0 << 6) | (3 << 4) | (0 << 3) | (1 << 2) | 0),
	((0 << 6) | (2 << 4) | (0 << 3) | (0 << 2) | 1),
	((1 << 6) | (1 << 4) | (0 << 3) | (0 << 2) | 2),
	((1 << 6) | (1 << 4) | (0 << 3) | (0 << 2) | 1),
	((0 << 6) | (3 << 4) | (1 << 3) | (1 << 2) | 1),
	((0 << 6) | (3 << 4) | (1 << 3) | (1 << 2) | 0),
};

static int rkclk_set_dpll(struct dram_info *dram_info)
{
	struct rk3128_pll *pll = &dram_info->cru->pll[1];
	struct pll_div dpll_cfg;
	/* PLL runs at 2 * phy freq */
	unsigned int pll_freq = dram_info->ddr_timing->base.ddr_freq * 2;
	unsigned int timeout = 1000;

	dpll_cfg.refdiv = 1;
	if (pll_freq <= 400) {
		dpll_cfg.postdiv1 = 6;
		dpll_cfg.postdiv2 = 1;
	} else if (pll_freq <= 600) {
		dpll_cfg.postdiv1 = 4;
		dpll_cfg.postdiv2 = 1;
	} else if (pll_freq <= 800) {
		dpll_cfg.postdiv1 = 3;
		dpll_cfg.postdiv2 = 1;
	} else {
		dpll_cfg.postdiv1 = 2;
		dpll_cfg.postdiv2 = 1;
	}

	dpll_cfg.fbdiv = (pll_freq * dpll_cfg.refdiv * dpll_cfg.postdiv1 * dpll_cfg.postdiv2) / 24;

	rk_clrsetreg(&dram_info->cru->cru_mode_con, DPLL_MODE_MASK,
		     DPLL_MODE_SLOW << DPLL_MODE_SHIFT);

	rk_setreg(&pll->con1, 1 << PLL_DSMPD_SHIFT);

	rk_clrsetreg(&pll->con0,
		     PLL_POSTDIV1_MASK | PLL_FBDIV_MASK,
		     (dpll_cfg.postdiv1 << PLL_POSTDIV1_SHIFT) |
			dpll_cfg.fbdiv);
	rk_clrsetreg(&pll->con1, PLL_POSTDIV2_MASK | PLL_REFDIV_MASK,
		     (dpll_cfg.postdiv2 << PLL_POSTDIV2_SHIFT |
		      dpll_cfg.refdiv << PLL_REFDIV_SHIFT));

	while (timeout > 0) {
		udelay(1);
		if (readl(&pll->con1) & (1 << PLL_LOCK_STATUS_SHIFT))
			break;
		timeout--;
	}

	if (timeout == 0) {
		debug("Failed to wait for DPLL lock");
		return -1;
	}

	rk_clrsetreg(&dram_info->cru->cru_mode_con, DPLL_MODE_MASK,
		     DPLL_MODE_NORM << DPLL_MODE_SHIFT);
	return 0;
}

static void phy_softreset(struct dram_info *dram_info)
{
	clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x00),
			RESET_DIGITAL_CORE_MASK | RESET_ANALOG_LOGIC_MASK,
			RESET_DIGITAL_CORE_ACT | RESET_ANALOG_LOGIC_ACT);
	udelay(10);

	clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x00),
			RESET_ANALOG_LOGIC_MASK, RESET_ANALOG_LOGIC_DIS);
	udelay(5);

	clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x00),
			RESET_DIGITAL_CORE_MASK, RESET_DIGITAL_CORE_DIS);
	udelay(1);
}

static void phy_pctrl_reset(struct dram_info *dram_info)
{
	rk_clrsetreg(&dram_info->cru->cru_softrst_con[5],
		     1 << DDRCTRL_PSRST_SHIFT | 1 << DDRCTRL_SRST_SHIFT |
		     1 << DDRPHY_PSRST_SHIFT | 1 << DDRPHY_SRST_SHIFT,
		     1 << DDRCTRL_PSRST_SHIFT | 1 << DDRCTRL_SRST_SHIFT |
		     1 << DDRPHY_PSRST_SHIFT | 1 << DDRPHY_SRST_SHIFT);
	udelay(10);

	rk_clrreg(&dram_info->cru->cru_softrst_con[5],
		  1 << DDRPHY_PSRST_SHIFT |
		  1 << DDRPHY_SRST_SHIFT);
	udelay(10);

	rk_clrreg(&dram_info->cru->cru_softrst_con[5],
		  1 << DDRCTRL_PSRST_SHIFT |
		  1 << DDRCTRL_SRST_SHIFT);
	udelay(10);

	phy_softreset(dram_info);
}

void pctl_copy_to_reg(unsigned int *dest, const unsigned short *src,
		      unsigned int n)
{
	unsigned int i;

	for (i = 0; i < n / sizeof(unsigned short); i++) {
		writel_relaxed(*src, dest);
		src++;
		dest++;
	}
}

static void move_to_access_state(struct dram_info *dram_info)
{
	struct rk3128_ddr_pctl *pctl = dram_info->pctl;
	unsigned int state;

	while (1) {
		state = readl(&pctl->stat) & PCTL_STAT_MASK;
		switch (state) {
		case LOW_POWER:
			writel(WAKEUP_STATE, &pctl->sctl);
			while ((readl(&pctl->stat) & PCTL_STAT_MASK) != ACCESS)
				;
			break;
		case INIT_MEM:
			writel(CFG_STATE, &pctl->sctl);
			while ((readl(&pctl->stat) & PCTL_STAT_MASK) != CONFIG)
				;
			fallthrough;
		case CONFIG:
			writel(GO_STATE, &pctl->sctl);
			while ((readl(&pctl->stat) & PCTL_STAT_MASK) != ACCESS)
				;
			break;
		case ACCESS:
			return;
		default:
			break;
		}
	}
}

static void move_to_config_state(struct dram_info *dram_info)
{
	struct rk3128_ddr_pctl *pctl = dram_info->pctl;
	unsigned int state;

	while (1) {
		state = readl(&pctl->stat) & PCTL_STAT_MASK;
		switch (state) {
		case LOW_POWER:
			writel(WAKEUP_STATE, &pctl->sctl);
			while ((readl(&pctl->stat) & PCTL_STAT_MASK) != ACCESS)
				;
			fallthrough;
		case ACCESS:
			fallthrough;
		case INIT_MEM:
			writel(CFG_STATE, &pctl->sctl);
			while ((readl(&pctl->stat) & PCTL_STAT_MASK) != CONFIG)
				;
			break;
		case CONFIG:
			return;
		default:
			break;
		}
	}
}

static void move_to_lowpower_state(struct dram_info *dram_info)
{
	struct rk3128_ddr_pctl *pctl = dram_info->pctl;
	unsigned int state;

	while (1) {
		state = readl(&pctl->stat) & PCTL_STAT_MASK;
		switch (state) {
		case INIT_MEM:
			writel(CFG_STATE, &pctl->sctl);
			while ((readl(&pctl->stat) & PCTL_STAT_MASK) != CONFIG)
				;
			fallthrough;
		case CONFIG:
			writel(GO_STATE, &pctl->sctl);
			while ((readl(&pctl->stat) & PCTL_STAT_MASK) != ACCESS)
				;
			fallthrough;
		case ACCESS:
			writel(SLEEP_STATE, &pctl->sctl);
			while ((readl(&pctl->stat) & PCTL_STAT_MASK) != LOW_POWER)
				;
			break;
		case LOW_POWER:
			return;
		default:
			break;
		}
	}
}

static void send_command(struct rk3128_ddr_pctl *pctl,
			 unsigned int rank, unsigned int cmd, unsigned int arg)
{
	writel((START_CMD | (rank << RANK_SEL_SHIFT) | arg | cmd), &pctl->mcmd);
	udelay(1);
	while (readl(&pctl->mcmd) & START_CMD)
		;
}

static void set_bw(struct dram_info *dram_info, unsigned int bw)
{
	struct rk3128_ddr_pctl *pctl = dram_info->pctl;
	struct rk3128_grf *grf = dram_info->grf;

	if (bw == 1) {
		clrsetbits_le32(&pctl->ppcfg, PPMEM_EN_MASK, PPMEM_EN);
		clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x00),
				CHANNEL_SELECT_MASK,
				CHANNEL_SELECT_DQ_16_BIT);
		rk_clrsetreg(&grf->soc_con0, DDR_8_BIT_SEL | DDR_16_BIT_SEL,
			     DDR_16_BIT_SEL);
		clrbits_le32(PHY_REG(dram_info->phy_base, 0x46), DQ_DLL_ENABLE);
		clrbits_le32(PHY_REG(dram_info->phy_base, 0x56), DQ_DLL_ENABLE);
	} else {
		clrbits_le32(&pctl->ppcfg, PPMEM_EN);
		clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x00),
				CHANNEL_SELECT_MASK,
				CHANNEL_SELECT_DQ_32_BIT);
		rk_clrreg(&grf->soc_con0, DDR_16_BIT_SEL | DDR_8_BIT_SEL);
		setbits_le32(PHY_REG(dram_info->phy_base, 0x46), DQ_DLL_ENABLE);
		setbits_le32(PHY_REG(dram_info->phy_base, 0x56), DQ_DLL_ENABLE);
	}
}

static void phy_drv_odt_set(struct dram_info *dram_info)
{
	struct rk3128_ddr_timing *ddr_timing = dram_info->ddr_timing;
	struct sdram_base_params *base = &ddr_timing->base;
	unsigned int odt, i;

	if (base->dramtype == DDR3)
		odt = PHY_RTT_216OHM;
	else
		odt = PHY_RTT_DISABLE;

	for (i = 0; i < NUM_DQS; i++)
		clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x21 + (i * 0x10)),
				PHY_DRV_ODT_SET(PHY_RTT_MASK),
				PHY_DRV_ODT_SET(odt));
}

static void phy_dll_set(struct dram_info *dram_info)
{
	struct rk3128_ddr_timing *ddr_timing = dram_info->ddr_timing;
	struct sdram_base_params *base = &ddr_timing->base;
	unsigned int mask, value, i;

	/* CMD DLL delay step */
	clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x13),
			CMD_DLL_CLOCK_PHASE_MASK | CMD_DLL_MASK |
			CMD_DLL_DELAY_MASK,
			CMD_DLL_CLOCK_PHASE_180_DELAY | CMD_DLL_ENABLE |
			CMD_DLL_DELAY_90);

	/* CK DLL delay step */
	clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x14),
			CK_DLL_DELAY_MASK | CK_DLL_CLOCK_PHASE_MASK,
			CK_DLL_DELAY_NO_DELAY | CK_DLL_CLOCK_PHASE_NO_DELAY);

	/* DM DQ DLL */
	mask = DQ_DLL_DELAY_MASK | DQ_DLL_MASK | DQ_CLOCK_PHASE_MASK;
	value = DQ_DLL_DELAY_90 | DQ_DLL_ENABLE | DQ_CLOCK_PHASE_180_DELAY;
	for (i = 0; i < NUM_DQS; i++)
		clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x26 + (i * 0x10)),
				mask, value);

	/* TX DQS DLL */
	mask = DQS_DLL_DELAY_MASK | DQS_DLL_CLOCK_PHASE_MASK;
	value = DQS_DLL_DELAY_NO_DELAY | DQS_DLL_CLOCK_PHASE_NO_DELAY;
	for (i = 0; i < NUM_DQS; i++)
		clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x27 + (i * 0x10)),
				mask, value);

	if (base->ddr_freq <= PHY_DLL_DISABLE_FREQ)
		setbits_le32(PHY_REG(dram_info->phy_base, 0xa4),
			     RIGHT_CHN_B_TX_DQ_BYPASS_SET |
			     LEFT_CHN_B_TX_DQ_BYPASS_SET |
			     RIGHT_CHN_A_TX_DQ_BYPASS_SET |
			     LEFT_CHN_A_TX_DQ_BYPASS_SET |
			     CK_DLL_DLL_BYPASS_SET);
	else
		clrbits_le32(PHY_REG(dram_info->phy_base, 0xa4),
			     RIGHT_CHN_B_TX_DQ_BYPASS_SET |
			     LEFT_CHN_B_TX_DQ_BYPASS_SET |
			     RIGHT_CHN_A_TX_DQ_BYPASS_SET |
			     LEFT_CHN_A_TX_DQ_BYPASS_SET |
			     CK_DLL_DLL_BYPASS_SET);

	/* read DQS delay phase */
	mask = RX_DQS_DLL_DELAY_MASK;
	if (base->ddr_freq < 350)
		value = RX_DQS_DLL_DELAY_67P5;
	else if (base->ddr_freq < 600)
		value = RX_DQS_DLL_DELAY_45;
	else
		value = RX_DQS_DLL_DELAY_22P5;

	for (i = 0; i < NUM_DQS; i++)
		clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x28 + (i * 0x10)),
				mask, value);
}

static void phy_cfg(struct dram_info *dram_info)
{
	struct rk3128_ddr_timing *ddr_timing = dram_info->ddr_timing;
	struct rk3128_service_sys *axi_bus = dram_info->axi_bus;
	struct sdram_base_params *base = &ddr_timing->base;
	unsigned int i, burst_len =
		((ddr_timing->noc_timing.burstlen << 1) == 4)
			? PHY_BL_4 : PHY_BL_8;

	writel(ddr_timing->noc_timing.noc_timing, &axi_bus->ddrtiming);
	writel(READLATENCY, &axi_bus->readlatency);

	if (base->dramtype == LPDDR2)
		writel(MEMORY_SELECT_LPDDR2 | burst_len,
		       PHY_REG(dram_info->phy_base, 0x01));
	else
		writel(MEMORY_SELECT_DDR3 | burst_len,
		       PHY_REG(dram_info->phy_base, 0x01));

	writel(ddr_timing->phy_timing.cl_al,
	       PHY_REG(dram_info->phy_base, 0x0b));
	writel(ddr_timing->pctl_timing.tcwl,
	       PHY_REG(dram_info->phy_base, 0x0c));

	clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x11),
			PHY_DRV_ODT_SET(PHY_RON_MASK),
			PHY_DRV_ODT_SET(PHY_CMD_DRV));

	clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x16),
			PHY_DRV_ODT_SET(PHY_RON_MASK),
			PHY_DRV_ODT_SET(PHY_CLK_DRV));

	for (i = 0; i < NUM_DQS; i++)
		clrsetbits_le32(PHY_REG(dram_info->phy_base, 0x20 + (i * 0x10)),
				PHY_DRV_ODT_SET(PHY_RON_MASK),
				PHY_DRV_ODT_SET(PHY_DQS_DRV));

	phy_drv_odt_set(dram_info);
}

static int memory_init(struct dram_info *dram_info)
{
	struct rk3128_ddr_pctl *pctl = dram_info->pctl;
	struct rk3128_ddr_timing *ddr_timing = dram_info->ddr_timing;
	unsigned int timeout = 1000;

	writel(POWER_UP_START, &pctl->powctl);
	while (timeout > 0) {
		udelay(1);
		if (readl(&pctl->powstat) & POWER_UP_DONE)
			break;
		timeout--;
	}

	if (timeout == 0) {
		debug("PCTL failed to power up");
		return -1;
	}

	if (IS_ENABLED(CONFIG_RAM_ROCKCHIP_LPDDR2)) {
		send_command(pctl, RANK_SEL_CS0_CS1, MRS_CMD,
			     (63 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (0 & LPDDR23_OP_MASK) <<
			     LPDDR23_OP_SHIFT);

		udelay(10);
		send_command(pctl, RANK_SEL_CS0, MRS_CMD,
			     (10 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (0xff & LPDDR23_OP_MASK) <<
			     LPDDR23_OP_SHIFT);

		udelay(1);
		send_command(pctl, RANK_SEL_CS1, MRS_CMD,
			     (10 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (0xff & LPDDR23_OP_MASK) <<
			     LPDDR23_OP_SHIFT);

		udelay(1);
		send_command(pctl, RANK_SEL_CS0_CS1, MRS_CMD,
			     (1 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (ddr_timing->phy_timing.mr[1] &
			     LPDDR23_OP_MASK) << LPDDR23_OP_SHIFT);

		send_command(pctl, RANK_SEL_CS0_CS1, MRS_CMD,
			     (2 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (ddr_timing->phy_timing.mr[2] &
			      LPDDR23_OP_MASK) << LPDDR23_OP_SHIFT);

		send_command(pctl, RANK_SEL_CS0_CS1, MRS_CMD,
			     (3 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (ddr_timing->phy_timing.mr[3] &
			      LPDDR23_OP_MASK) << LPDDR23_OP_SHIFT);
	} else {
		send_command(pctl, RANK_SEL_CS0_CS1, DESELECT_CMD, 0);
		udelay(1);

		send_command(pctl, RANK_SEL_CS0_CS1, PREA_CMD, 0);
		send_command(pctl, RANK_SEL_CS0_CS1, DESELECT_CMD, 0);
		udelay(1);

		send_command(pctl, RANK_SEL_CS0_CS1, MRS_CMD,
			     (MR2 & BANK_ADDR_MASK) << BANK_ADDR_SHIFT |
			     (ddr_timing->phy_timing.mr[2] & CMD_ADDR_MASK)
			     << CMD_ADDR_SHIFT);

		send_command(pctl, RANK_SEL_CS0_CS1, MRS_CMD,
			     (MR3 & BANK_ADDR_MASK) << BANK_ADDR_SHIFT |
			     (ddr_timing->phy_timing.mr[3] & CMD_ADDR_MASK)
			     << CMD_ADDR_SHIFT);

		send_command(pctl, RANK_SEL_CS0_CS1, MRS_CMD,
			     (MR1 & BANK_ADDR_MASK) << BANK_ADDR_SHIFT |
			     (ddr_timing->phy_timing.mr[1] & CMD_ADDR_MASK)
			     << CMD_ADDR_SHIFT);

		send_command(pctl, RANK_SEL_CS0_CS1, MRS_CMD,
			     (MR0 & BANK_ADDR_MASK) << BANK_ADDR_SHIFT |
			     (((ddr_timing->phy_timing.mr[0] | DDR3_DLL_RESET) &
			       CMD_ADDR_MASK) << CMD_ADDR_SHIFT));

		send_command(pctl, RANK_SEL_CS0_CS1, ZQCL_CMD, 0);
	}

	return 0;
}

static int data_training(struct dram_info *dram_info)
{
	struct rk3128_ddr_pctl *pctl = dram_info->pctl;
	unsigned int trefi, ret, timeout = 100;
	unsigned int bw =
		readl(PHY_REG(dram_info->phy_base, 0x00)) & CHANNEL_SELECT_MASK;

	/* disable auto refresh */
	trefi = readl(&pctl->trefi);
	writel(0, &pctl->trefi);
	setbits_le32(PHY_REG(dram_info->phy_base, 0x02), DQS_SQU_CAL_START);

	while (timeout > 0) {
		udelay(1);
		ret = readl(PHY_REG(dram_info->phy_base, 0xff));
		if (bw == CHANNEL_SELECT_DQ_32_BIT &&
		    (ret & CHN_B_TRAINING_DONE_MASK) == CHN_B_TRAINING_DONE_MASK)
			break;
		else if (bw == CHANNEL_SELECT_DQ_16_BIT &&
			 (ret & CHN_A_TRAINING_DONE_MASK) == CHN_A_TRAINING_DONE_MASK)
			break;
		timeout--;
	}

	clrbits_le32(PHY_REG(dram_info->phy_base, 0x02), DQS_SQU_CAL_START);

	/*
	 * since data training will take about 20us, so send some auto
	 * refresh(about 7.8us) to complement the lost time
	 */
	send_command(pctl, RANK_SEL_CS0_CS1, PREA_CMD, 0);
	send_command(pctl, RANK_SEL_CS0_CS1, REF_CMD, 0);

	writel(trefi, &pctl->trefi);

	return timeout > 0 ? 0 : -1;
}

static void pctl_cfg(struct dram_info *dram_info)
{
	struct rk3128_grf *grf = dram_info->grf;
	struct rk3128_ddr_pctl *pctl = dram_info->pctl;
	struct rk3128_ddr_timing *ddr_timing = dram_info->ddr_timing;
	struct sdram_base_params *base = &dram_info->ddr_timing->base;
	unsigned int burst_len = (ddr_timing->noc_timing.burstlen << 1);

	setbits_le32(&pctl->dfistcfg0,
		     DFI_INIT_START | DFI_DATA_BYTE_DISABLE_EN);
	setbits_le32(&pctl->dfistcfg1, DFI_DRAM_CLK_SR_EN);
	setbits_le32(&pctl->dfistcfg2,
		     DFI_PARITY_INTR_EN | DFI_PARITY_EN);

	writel(TPHYUPD_TYPE0, &pctl->dfitphyupdtype0);
	writel(TPHY_RDLAT, &pctl->dfitphyrdlat);
	writel(TPHY_WRDATA, &pctl->dfitphywrdata);

	/* phyupd and ctrlupd disabled */
	writel(DFI_PHYUPD_DISABLE | DFI_CTRLUPD_DISABLE, &pctl->dfiupdcfg);

	pctl_copy_to_reg(&pctl->togcnt1u, &ddr_timing->pctl_timing.togcnt1u,
			 sizeof(struct rk3128_pctl_timing));

	if (IS_ENABLED(CONFIG_RAM_ROCKCHIP_LPDDR2)) {
		clrsetbits_le32(&pctl->trp,
				TRP_PREA_EXTRA_MASK,
				DIV_ROUND_UP(TRP_LPDDR2_8_BK * base->ddr_freq, 1000)
				<< TRP_PREA_EXTRA_SHIFT);

		setbits_le32(&pctl->dfistcfg1, DFI_DRAM_CLK_DPD_EN);

		/* cs0 and cs1 write odt disable */
		clrbits_le32(&pctl->dfiodtcfg,
			     RANK0_ODT_READ_SEL | RANK0_ODT_WRITE_SEL |
			     RANK1_ODT_READ_SEL | RANK1_ODT_WRITE_SEL);

		/* disable odt write length */
		clrbits_le32(&pctl->dfiodtcfg1,
			     ODT_LEN_BL8_W_MASK | ODT_LEN_BL8_R_MASK);

		writel(MDDR_CLOCK_STOP_IDLE_DIS | LPDDR2_EN | LPDDR2_S4 |
		       (burst_len == 4 ? MDDR_LPDDR2_BL_4 : MDDR_LPDDR2_BL_8) |
		       TFAW_CFG_6_TDDR | PD_EXIT_FAST_EXIT_MODE |
		       PD_TYPE_ACT_PD | PD_IDLE_DISABLE,
		       &pctl->mcfg);

		writel(DFI_LP_EN_PD | DFI_LP_EN_SR | DFI_TLP_RESP,
		       &pctl->dfilpcfg0);

		writel((readl(&pctl->tcl) - 3), &pctl->dfitrddataen);
		writel(readl(&pctl->tcwl),  &pctl->dfitphywrlat);

		rk_setreg(&grf->soc_con0, MOBILE_DDR_SEL);
		rk_clrreg(&grf->soc_con2, MSCH4_MAINDDR3);
	} else {
		/* cs0 and cs1 write odt enable */
		setbits_le32(&pctl->dfiodtcfg,
			     RANK0_ODT_READ_SEL | RANK0_ODT_WRITE_SEL |
			     RANK1_ODT_READ_SEL | RANK1_ODT_WRITE_SEL);

		/* odt write length */
		clrsetbits_le32(&pctl->dfiodtcfg1,
				ODT_LEN_BL8_W_MASK | ODT_LEN_BL8_R_MASK,
				ODT_LEN_BL8_W | ODT_LEN_BL8_R);

		writel(MDDR_CLOCK_STOP_IDLE_DIS | DDR3_EN | MEM_BL_8 |
		       TFAW_CFG_5_TDDR | PD_EXIT_SLOW_EXIT_MODE |
		       PD_TYPE_ACT_PD | PD_IDLE_DISABLE, &pctl->mcfg);

		writel(DFI_LP_EN_PD | DFI_LP_EN_SR | DFI_TLP_RESP,
		       &pctl->dfilpcfg0);

		writel((readl(&pctl->tcl) - 3), &pctl->dfitrddataen);
		writel((readl(&pctl->tcwl) - 1), &pctl->dfitphywrlat);

		rk_clrreg(&grf->soc_con0, MOBILE_DDR_SEL);
		rk_setreg(&grf->soc_con2, MSCH4_MAINDDR3);
	}

	setbits_le32(&pctl->scfg, HW_LOW_POWER_EN);
}

void pctl_cfg_trfc(struct dram_info *dram_info)
{
	struct rk3128_ddr_pctl *pctl = dram_info->pctl;
	struct sdram_base_params *base = &dram_info->ddr_timing->base;
	struct sdram_cap_info *cap_info = &dram_info->cap_info;
	unsigned int trfc, die_cap;
	unsigned int cs0_cap =
		(u32)sdram_get_cs_cap(cap_info, 0, base->dramtype);

	die_cap = (cs0_cap >> (20 + (cap_info->bw - cap_info->dbw)));
	if (IS_ENABLED(CONFIG_RAM_ROCKCHIP_LPDDR2)) {
		unsigned int texsr, trefi = LPDDR2_TREFI_7_8_us;
		trfc = LPDDR2_TRFC_512Mb;

		if (die_cap > DIE_CAP_1GBIT)
			trefi = LPDDR2_TREFI_3_9_us;

		if (die_cap > DIE_CAP_512MBIT)
			trfc = LPDDR2_TRFC_4Gb;
		if (die_cap > DIE_CAP_4GBIT)
			trfc = LPDDR2_TRFC_8Gb;

		texsr = MAX(2, DIV_ROUND_UP(((trfc + 10) * base->ddr_freq), 1000));
		writel(trefi, &pctl->trefi);
		writel(texsr, &pctl->texsr);

	} else {
		if (die_cap <= DIE_CAP_512MBIT)
			trfc = DDR3_TRFC_512Mb;
		else if (die_cap <= DIE_CAP_1GBIT)
			trfc = DDR3_TRFC_1Gb;
		else if (die_cap <= DIE_CAP_2GBIT)
			trfc = DDR3_TRFC_2Gb;
		else if (die_cap <= DIE_CAP_4GBIT)
			trfc = DDR3_TRFC_4Gb;
		else
			trfc = DDR3_TRFC_8Gb;
	}

	trfc = DIV_ROUND_UP((trfc * base->ddr_freq), 1000);
	writel(trfc, &pctl->trfc);
}

void dram_cfg_rbc(struct dram_info *dram_info)
{
	struct rk3128_service_sys *axi_bus = dram_info->axi_bus;
	struct sdram_cap_info *cap_info = &dram_info->cap_info;
	char noc_config;
	int i = 0;

	move_to_config_state(dram_info);

	if (cap_info->rank == 2 && cap_info->cs1_row == cap_info->cs0_row) {
		if (cap_info->col + cap_info->bw == 12) {
			i = 14;
			goto finish;
		} else if (cap_info->col + cap_info->bw == 11) {
			i = 15;
			goto finish;
		}
	}
	noc_config = ((cap_info->cs0_row - 13) << 4) | ((cap_info->bk - 2) << 2) |
		     (cap_info->col + cap_info->bw - 11);
	for (i = 0; i < 11; i++) {
		if (noc_config == ddr_cfg_2_rbc[i])
			break;
	}

	if (i < 11)
		goto finish;

	noc_config = ((cap_info->bk - 2) << 6) | ((cap_info->cs0_row - 13) << 4) |
		     (cap_info->col + cap_info->bw - 11);

	for (i = 11; i < 14; i++) {
		if (noc_config == ddr_cfg_2_rbc[i])
			break;
	}
	if (i < 14)
		goto finish;
	else
		i = 0;

finish:
	writel(i, &axi_bus->ddrconf);
	move_to_access_state(dram_info);
}

static int detect_bw(struct dram_info *dram_info)
{
	struct sdram_cap_info *cap_info = &dram_info->cap_info;

	set_bw(dram_info, 2);
	if (data_training(dram_info) == 0) {
		cap_info->bw = 2;
	} else {
		set_bw(dram_info, 1);
		move_to_lowpower_state(dram_info);
		phy_softreset(dram_info);
		move_to_config_state(dram_info);
		if (data_training(dram_info)) {
			debug("Failed to detect bandwidth");
			return -1;
		}
		cap_info->bw = 1;
	}

	return 0;
}

static void detect_rank(struct dram_info *dram_info)
{
	struct sdram_cap_info *cap_info = &dram_info->cap_info;

	writel(0, CFG_SYS_SDRAM_BASE);
	writel(PATTERN, CFG_SYS_SDRAM_BASE + (1u << 30));
	writel(~PATTERN, CFG_SYS_SDRAM_BASE + (1u << 30) + 4);

	if ((readl(CFG_SYS_SDRAM_BASE + (1u << 30)) == PATTERN) &&
	    (readl(CFG_SYS_SDRAM_BASE) == 0))
		cap_info->rank = 2;
	else
		cap_info->rank = 1;
}

static int detect_cap(struct dram_info *dram_info)
{
	struct rk3128_service_sys *axi_bus = dram_info->axi_bus;
	struct rk3128_ddr_timing *ddr_timing = dram_info->ddr_timing;
	struct sdram_cap_info *cap_info = &dram_info->cap_info;
	struct sdram_base_params *base = &ddr_timing->base;
	int ret = 0;
	unsigned int coltmp, rowtmp = 16, bktmp = 3;

	ret = detect_bw(dram_info);
	if (ret)
		goto out;

	move_to_config_state(dram_info);
	if (cap_info->bw == 2) {
		writel(6, &axi_bus->ddrconf);
		coltmp = 11;
	} else {
		writel(3, &axi_bus->ddrconf);
		coltmp = 10;
	};

	move_to_access_state(dram_info);
	if (sdram_detect_col(cap_info, coltmp) != 0) {
		ret = -1;
		goto out;
	}
	sdram_detect_bank(cap_info, coltmp, bktmp);

	move_to_config_state(dram_info);

	writel(10, &axi_bus->ddrconf);

	move_to_access_state(dram_info);

	detect_rank(dram_info);

	coltmp = cap_info->bw == 1 ? 10 : 9;
	if (sdram_detect_row(cap_info, coltmp, bktmp, rowtmp) != 0) {
		ret = -1;
		goto out;
	}

	// assume cs1_row == cs0_row
	if (cap_info->rank > 1)
		cap_info->cs1_row = cap_info->cs0_row;

	sdram_detect_dbw(cap_info, base->dramtype);

	move_to_config_state(dram_info);
	// re-configure trfc/trefi based on actual per die capacity
	pctl_cfg_trfc(dram_info);
	move_to_access_state(dram_info);

	sdram_print_ddr_info(cap_info, base, 0);

out:
	return ret;
}

static void sdram_all_config(struct dram_info *dram_info)
{
	struct rk3128_ddr_timing *ddr_timing = dram_info->ddr_timing;
	struct rk3128_grf *grf = dram_info->grf;
	struct sdram_base_params *base = &ddr_timing->base;
	struct sdram_cap_info *cap_info = &dram_info->cap_info;
	unsigned int sys_reg2 = 0;
	unsigned int sys_reg3 = 0;

	sdram_org_config(cap_info, base, &sys_reg2, &sys_reg3, 0);
	writel(sys_reg2, &grf->os_reg[1]);
	writel(sys_reg3, &grf->os_reg[2]);
}

int rk3128_sdram_init(enum rk3128_dram_config dram_config)
{
	struct dram_info dram_info;
	struct rk3128_ddr_timing *ddr_timing = &rk3128_ddr_timings[dram_config];
	struct sdram_base_params *base = &ddr_timing->base;

	debug("\nIn\n");

	if (base->dramtype != DDR3 && base->dramtype != LPDDR2) {
		printascii("\nOnly DDR3 and LPDDR2 are supported.\n");
		return -1;
	}

	dram_info.axi_bus = (void __iomem *)CPU_AXI_BUS_BASE;
	dram_info.ddr_timing = ddr_timing;
	dram_info.cru = (void __iomem *)CRU_BASE;
	dram_info.grf = (void __iomem *)GRF_BASE;
	dram_info.pctl = (void __iomem *)DDR_PCTL_BASE;
	dram_info.phy_base = (void __iomem *)DDR_PHY_BASE;

	if (rkclk_set_dpll(&dram_info))
		return -1;

	phy_pctrl_reset(&dram_info);

	phy_dll_set(&dram_info);

	pctl_cfg(&dram_info);

	phy_cfg(&dram_info);

	if (memory_init(&dram_info))
		return -1;

	if (detect_cap(&dram_info))
		return -1;

	dram_cfg_rbc(&dram_info);

	sdram_all_config(&dram_info);

	/* set vio/vip axi qos to max priority */
	writel(QOS_PRIORITY(3), AXI_VIO_QOS_PRIO);
	writel(QOS_PRIORITY(3), AXI_VIP_QOS_PRIO);

	if (!IS_ENABLED(CONFIG_RAM_ROCKCHIP_DEBUG)) {
		struct sdram_cap_info *cap_info = &dram_info.cap_info;

		if (IS_ENABLED(CONFIG_RAM_ROCKCHIP_LPDDR2))
			printascii("\nLPDRR2, ");
		else
			printascii("\nDDR3, ");
		printdec(base->ddr_freq);
		printascii(" MHz, ");
		printdec((sdram_get_cs_cap(cap_info, 3, base->dramtype) >> 20));
		printascii("MB\n");
	}

	debug("\nOut\n");

	return 0;
}
#else
static int rk3128_dmc_probe(struct udevice *dev)
{
	struct dram_info *priv = dev_get_priv(dev);

	priv->grf = syscon_get_first_range(ROCKCHIP_SYSCON_GRF);
	debug("%s: grf=%p\n", __func__, priv->grf);
	priv->info.base = CFG_SYS_SDRAM_BASE;
	priv->info.size = rockchip_sdram_size(
				(phys_addr_t)&priv->grf->os_reg[1]);

	return 0;
}

static int rk3128_dmc_get_info(struct udevice *dev, struct ram_info *info)
{
	struct dram_info *priv = dev_get_priv(dev);

	*info = priv->info;

	return 0;
}

static struct ram_ops rk3128_dmc_ops = {
	.get_info = rk3128_dmc_get_info,
};

static const struct udevice_id rk3128_dmc_ids[] = {
	{ .compatible = "rockchip,rk3128-dmc" },
	{ }
};

U_BOOT_DRIVER(dmc_rk3128) = {
	.name = "rockchip_rk3128_dmc",
	.id = UCLASS_RAM,
	.of_match = rk3128_dmc_ids,
	.ops = &rk3128_dmc_ops,
	.probe = rk3128_dmc_probe,
	.priv_auto	= sizeof(struct dram_info),
};
#endif
