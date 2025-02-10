// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _ASM_ARCH_SDRAM_RK3128_H
#define _ASM_ARCH_SDRAM_RK3128_H

#include <asm/arch-rockchip/sdram_common.h>

#define NUM_DQS			4
#define PHY_REG(base, n)	((base) + 4 * (n))
#define PHY_DRV_ODT_SET(n)	(((n) << 4) | (n))
#define QOS_PRIORITY(n)		(((n) << 2) | (n))

enum rk3128_dram_config {
#if IS_ENABLED(CONFIG_RAM_ROCKCHIP_LPDDR2)
	DRAM_LPDDR2_300_MHZ,
	DRAM_LPDDR2_400_MHZ,
#else
	DRAM_DDR3_300_MHZ,
	DRAM_DDR3_456_MHZ,
	DRAM_DDR3_528_MHZ,
#endif
	DRAM_INVALID,
};

enum {
	PHY_RON_DISABLE		= 0,
	PHY_RON_309OHM		= 1,
	PHY_RON_155OHM,
	PHY_RON_103OHM		= 3,
	PHY_RON_63OHM		= 5,
	PHY_RON_45OHM		= 7,
	PHY_RON_77OHM,
	PHY_RON_62OHM,
	PHY_RON_52OHM,
	PHY_RON_44OHM,
	PHY_RON_39OHM,
	PHY_RON_34OHM,
	PHY_RON_31OHM,
	PHY_RON_28OHM,
	PHY_RON_MASK		= 15,
};

enum {
	PHY_RTT_DISABLE		= 0,
	PHY_RTT_861OHM		= 1,
	PHY_RTT_431OHM,
	PHY_RTT_287OHM,
	PHY_RTT_216OHM,
	PHY_RTT_172OHM,
	PHY_RTT_145OHM,
	PHY_RTT_124OHM,
	PHY_RTT_215OHM,
	PHY_RTT_144OHM		= 10,
	PHY_RTT_123OHM,
	PHY_RTT_108OHM,
	PHY_RTT_96OHM,
	PHY_RTT_86OHM,
	PHY_RTT_78OHM,
	PHY_RTT_MASK		= 15,
};

enum {
	DQS_DLL_NO_DELAY	= 0,
	DQS_DLL_22P5_DELAY,
	DQS_DLL_45_DELAY,
	DQS_DLL_67P5_DELAY,
	DQS_DLL_90_DELAY,
	DQS_DLL_112P5_DELAY,
	DQS_DLL_135_DELAY,
	DQS_DLL_157P5_DELAY,
};

enum {
	LPDDR2_TREFI_3_9_us	= 39,
	LPDDR2_TREFI_7_8_us	= 78,
};

enum {
	DDR3_TRFC_512Mb		= 90,
	DDR3_TRFC_1Gb		= 110,
	DDR3_TRFC_2Gb		= 160,
	DDR3_TRFC_4Gb		= 300,
	DDR3_TRFC_8Gb		= 350,

	LPDDR2_TRFC_512Mb	= 90,
	LPDDR2_TRFC_4Gb		= 130,
	LPDDR2_TRFC_8Gb		= 210,
};

/* GRF_SOC_CON0 */
enum {
	MOBILE_DDR_SEL		= BIT(7),
	DDR_8_BIT_SEL		= BIT(13),
	DDR_16_BIT_SEL		= BIT(14),
};

/* GRF_SOC_CON2 */
enum {
	PCTL_SELF_REFRESH_ENTER	= BIT(1),
	PHY_LOWPWR_DISBALE	= BIT(2),
	MSCH4_MAINDDR3		= BIT(7),
};

/* PTCL */
enum {
	/* PCTL_TRP */
	TRP_PREA_EXTRA_SHIFT		= 16,
	TRP_PREA_EXTRA_MASK		= GENMASK(19, TRP_PREA_EXTRA_SHIFT),
	TRP_LPDDR2_8_BK			= 3,

	/* PCTL_DFISTCFG0 */
	DFI_INIT_START			= BIT(0),
	DFI_FREQ_RATIO_EN		= BIT(1),
	DFI_DATA_BYTE_DISABLE_EN	= BIT(2),

	/* PCTL_DFISTCFG1 */
	DFI_DRAM_CLK_SR_EN		= BIT(0),
	DFI_DRAM_CLK_DPD_EN		= BIT(1),

	/* PCTL_DFISTCFG2 */
	DFI_PARITY_INTR_EN		= BIT(0),
	DFI_PARITY_EN			= BIT(1),

	/* PCTL_DFILPCFG0 */
	DFI_LP_EN_PD_SHIFT		= 0,
	DFI_LP_EN_PD			= BIT(DFI_LP_EN_PD_SHIFT),
	DFI_LP_EN_SR_SHIFT		= 8,
	DFI_LP_EN_SR			= BIT(DFI_LP_EN_SR_SHIFT),
	DFI_TLP_RESP_SHIFT		= 16,
	DFI_TLP_RESP			= 7 << DFI_TLP_RESP_SHIFT,

	/* PCTL_DFITPHYUPDTYPE0 */
	TPHYUPD_TYPE0			= 1,

	/* PCTL_DFITPHYRDLAT */
	TPHY_RDLAT			= 13,

	/* PCTL_DFITPHYWRDATA */
	TPHY_WRDATA			= 1,

	/* PCTL_DFIUPDCFG */
	DFI_PHYUPD_DISABLE		= 0 << 1,
	DFI_CTRLUPD_DISABLE		= 0,

	/* PCTL_DFIODTCFG */
	RANK0_ODT_READ_SEL_SHIFT	= 1,
	RANK0_ODT_READ_SEL		= BIT(RANK0_ODT_READ_SEL_SHIFT),
	RANK0_ODT_WRITE_SEL_SHIFT	= 3,
	RANK0_ODT_WRITE_SEL		= BIT(RANK0_ODT_WRITE_SEL_SHIFT),
	RANK1_ODT_READ_SEL_SHIFT	= 8,
	RANK1_ODT_READ_SEL		= BIT(RANK1_ODT_READ_SEL_SHIFT),
	RANK1_ODT_WRITE_SEL_SHIFT	= 11,
	RANK1_ODT_WRITE_SEL		= BIT(RANK1_ODT_WRITE_SEL_SHIFT),

	/* PCTL_DFIODTCFG1 */
	ODT_LEN_BL8_W_SHIFT		= 16,
	ODT_LEN_BL8_W_MASK		= GENMASK(18, ODT_LEN_BL8_W_SHIFT),
	ODT_LEN_BL8_W			= 7 << ODT_LEN_BL8_W_SHIFT,
	ODT_LEN_BL8_W_0			= 0 << ODT_LEN_BL8_W_SHIFT,
	ODT_LEN_BL8_R_SHIFT		= 24,
	ODT_LEN_BL8_R_MASK		= GENMASK(26, ODT_LEN_BL8_R_SHIFT),
	ODT_LEN_BL8_R			= 7 << ODT_LEN_BL8_R_SHIFT,
	ODT_LEN_BL8_R_0			= 0 << ODT_LEN_BL8_R_SHIFT,

	/* PCTL_MCFG */
	MEM_BL_4			= 0 << 0,
	MEM_BL_8			= BIT(0),
	TWO_T_SHIFT			= 3,
	TWO_T_EN			= BIT(TWO_T_SHIFT),
	DDR3_EN				= BIT(5),
	DDR2_EN				= 0 << 5,
	LPDDR2_S4			= BIT(6),
	PD_IDLE_SHIFT			= 8,
	PD_IDLE_DISABLE			= 0 << PD_IDLE_SHIFT,
	PD_IDLE_MASK			= GENMASK(15, PD_IDLE_SHIFT),
	PD_TYPE_ACT_PD			= BIT(16),
	PD_EXIT_SLOW_EXIT_MODE		= 0 << 17,
	PD_EXIT_FAST_EXIT_MODE		= BIT(17),
	TFAW_CFG_5_TDDR			= BIT(18),
	TFAW_CFG_6_TDDR			= 2 << 18,
	MDDR_LPDDR2_BL_4		= BIT(20),
	MDDR_LPDDR2_BL_8		= 2 << 20,
	LPDDR2_EN			= 3 << 22,
	MDDR_CLOCK_STOP_IDLE_DIS	= 0 << 24,

	/* PCTL_MCFG1 */
	HW_EXIT_IDLE_EN_MASK		= 1,
	HW_EXIT_IDLE_EN_SHIFT		= 31,
	SR_IDLE_MASK			= GENMASK(8, 0),
	SR_IDLE_SHIFT			= 0,

	/* PCTL_SCFG */
	HW_LOW_POWER_EN			= BIT(0),

	/* PCTL_POWCTL */
	POWER_UP_START			= BIT(0),

	/* PCTL_POWSTAT */
	POWER_UP_DONE			= BIT(0),

	/*PCTL_PPCFG*/
	PPMEM_EN_SHIFT			= 0,
	PPMEM_EN_MASK			= GENMASK(0, PPMEM_EN_SHIFT),
	PPMEM_EN			= BIT(PPMEM_EN_SHIFT),
	PPMEM_DIS			= 0 << PPMEM_EN_SHIFT,

	/* PCTL_MCMD */
	START_CMD			= BIT(31),
	RANK_SEL_SHIFT			= 20,
	RANK_SEL_CS0			= 1,
	RANK_SEL_CS1			= 2,
	RANK_SEL_CS0_CS1		= 3,
	BANK_ADDR_SHIFT			= 17,
	BANK_ADDR_MASK			= GENMASK(2, 0),
	CMD_ADDR_SHIFT			= 4,
	CMD_ADDR_MASK			= GENMASK(12, 0),
	LPDDR23_MA_SHIFT		= 4,
	LPDDR23_MA_MASK			= GENMASK(7, 0),
	LPDDR23_OP_SHIFT		= 12,
	LPDDR23_OP_MASK			= GENMASK(7, 0),
	DDR3_DLL_RESET			= BIT(8),
	DESELECT_CMD			= 0,

	PREA_CMD			= 1,
	REF_CMD,
	MRS_CMD,
	ZQCS_CMD,
	ZQCL_CMD,
	RSTL_CMD,
	MPR_CMD				= 8,
	DFICTRLUPD_CMD			= 10,

	MR0				= 0,
	MR1,
	MR2,
	MR3,

	/* PCTL_STAT */
	INIT_MEM			= 0,
	CONFIG,
	CONFIG_REQ,
	ACCESS,
	ACCESS_REQ,
	LOW_POWER,
	LOW_POWER_ENTRY_REQ,
	LOW_POWER_EXIT_REQ,
	PCTL_STAT_MASK			= 7,

	/* PCTL_SCTL */
	INIT_STATE			= 0,
	CFG_STATE,
	GO_STATE,
	SLEEP_STATE,
	WAKEUP_STATE,
};

/* DDRPHY */
enum {
	/* DDRPHY_REG00 */
	RESET_ANALOG_LOGIC_SHIFT	= 2,
	RESET_ANALOG_LOGIC_MASK		= 1 << RESET_ANALOG_LOGIC_SHIFT,
	RESET_ANALOG_LOGIC_ACT		= 0 << RESET_ANALOG_LOGIC_SHIFT,
	RESET_ANALOG_LOGIC_DIS		= 1 << RESET_ANALOG_LOGIC_SHIFT,

	RESET_DIGITAL_CORE_SHIFT	= 3,
	RESET_DIGITAL_CORE_MASK		= 1 << RESET_DIGITAL_CORE_SHIFT,
	RESET_DIGITAL_CORE_ACT		= 0 << RESET_DIGITAL_CORE_SHIFT,
	RESET_DIGITAL_CORE_DIS		= 1 << RESET_DIGITAL_CORE_SHIFT,

	CHANNEL_SELECT_SHIFT		= 4,
	CHANNEL_SELECT_MASK		= 15 << CHANNEL_SELECT_SHIFT,
	CHANNEL_SELECT_DQ_16_BIT	= 3 << CHANNEL_SELECT_SHIFT,
	CHANNEL_SELECT_DQ_32_BIT	= 15 << CHANNEL_SELECT_SHIFT,

	/* DDRPHY_REG01 */
	MEMORY_SELECT_DDR3		= 0,
	MEMORY_SELECT_DDR2,
	MEMORY_SELECT_LPDDR2,
	PHY_BL_4			= 0 << 2,
	PHY_BL_8			= BIT(2),

	/* DDRPHY_REG02 */
	DQS_SQU_CAL_START		= BIT(0),
	DQS_SQU_NO_CAL			= 0 << 0,
	DQS_SQU_CAL_MASK		= 1,

	/* DDRPHY_REG13 */
	CMD_DLL_DELAY_SHIFT		= 0,
	CMD_DLL_DELAY_MASK		= 7 << CMD_DLL_DELAY_SHIFT,
	CMD_DLL_DELAY_NO_DELAY		= 0 << CMD_DLL_DELAY_SHIFT,
	CMD_DLL_DELAY_22P5		= 1 << CMD_DLL_DELAY_SHIFT,
	CMD_DLL_DELAY_45		= 2 << CMD_DLL_DELAY_SHIFT,
	CMD_DLL_DELAY_67P5		= 3 << CMD_DLL_DELAY_SHIFT,
	CMD_DLL_DELAY_90		= 4 << CMD_DLL_DELAY_SHIFT,
	CMD_DLL_DELAY_112P5		= 5 << CMD_DLL_DELAY_SHIFT,
	CMD_DLL_DELAY_135		= 6 << CMD_DLL_DELAY_SHIFT,
	CMD_DLL_DELAY_157P5		= 7 << CMD_DLL_DELAY_SHIFT,

	CMD_DLL_SHIFT			= 3,
	CMD_DLL_MASK			= 1 << CMD_DLL_SHIFT,
	CMD_DLL_ENABLE			= 1 << CMD_DLL_SHIFT,

	CMD_DLL_CLOCK_PHASE_SHIFT	= 4,
	CMD_DLL_CLOCK_PHASE_MASK	= 1 << CMD_DLL_CLOCK_PHASE_SHIFT,
	CMD_DLL_CLOCK_PHASE_NO_DELAY	= 0 << CMD_DLL_CLOCK_PHASE_SHIFT,
	CMD_DLL_CLOCK_PHASE_180_DELAY	= 1 << CMD_DLL_CLOCK_PHASE_SHIFT,

	/* DDRPHY_REG14 */
	CK_DLL_DELAY_SHIFT		= 0,
	CK_DLL_DELAY_MASK		= 7 << CK_DLL_DELAY_SHIFT,
	CK_DLL_DELAY_NO_DELAY		= 0 << CK_DLL_DELAY_SHIFT,
	CK_DLL_DELAY_22P5		= 1 << CK_DLL_DELAY_SHIFT,
	CK_DLL_DELAY_45			= 2 << CK_DLL_DELAY_SHIFT,
	CK_DLL_DELAY_67P5		= 3 << CK_DLL_DELAY_SHIFT,
	CK_DLL_DELAY_90			= 4 << CK_DLL_DELAY_SHIFT,
	CK_DLL_DELAY_112P5		= 5 << CK_DLL_DELAY_SHIFT,
	CK_DLL_DELAY_135		= 6 << CK_DLL_DELAY_SHIFT,
	CK_DLL_DELAY_157P5		= 7 << CK_DLL_DELAY_SHIFT,

	CK_DLL_CLOCK_PHASE_SHIFT	= 4,
	CK_DLL_CLOCK_PHASE_MASK		= 1 << CK_DLL_CLOCK_PHASE_SHIFT,
	CK_DLL_CLOCK_PHASE_NO_DELAY	= 0 << CK_DLL_CLOCK_PHASE_SHIFT,
	CK_DLL_CLOCK_PHASE_180_DELAY	= 1 << CK_DLL_CLOCK_PHASE_SHIFT,

	/* DDRPHY_REG26, DDRPHY_REG36, DDRPHY_REG46, DDRPHY_REG56*/
	DQ_DLL_DELAY_SHIFT		= 0,
	DQ_DLL_DELAY_MASK		= 7 << DQ_DLL_DELAY_SHIFT,
	DQ_DLL_DELAY_NO_DELAY		= 0 << DQ_DLL_DELAY_SHIFT,
	DQ_DLL_DELAY_22P5		= 1 << DQ_DLL_DELAY_SHIFT,
	DQ_DLL_DELAY_45			= 2 << DQ_DLL_DELAY_SHIFT,
	DQ_DLL_DELAY_67P5		= 3 << DQ_DLL_DELAY_SHIFT,
	DQ_DLL_DELAY_90			= 4 << DQ_DLL_DELAY_SHIFT,
	DQ_DLL_DELAY_112P5		= 5 << DQ_DLL_DELAY_SHIFT,
	DQ_DLL_DELAY_135		= 6 << DQ_DLL_DELAY_SHIFT,
	DQ_DLL_DELAY_157P5		= 7 << DQ_DLL_DELAY_SHIFT,

	DQ_DLL_SHIFT			= 3,
	DQ_DLL_MASK			= 1 << DQ_DLL_SHIFT,
	DQ_DLL_ENABLE			= 1 << DQ_DLL_SHIFT,

	DQ_CLOCK_PHASE_SHIFT		= 4,
	DQ_CLOCK_PHASE_MASK		= 1 << DQ_CLOCK_PHASE_SHIFT,
	DQ_CLOCK_PHASE_NO_DELAY		= 0 << DQ_CLOCK_PHASE_SHIFT,
	DQ_CLOCK_PHASE_180_DELAY	= 1 << DQ_CLOCK_PHASE_SHIFT,

	/* DDRPHY_REG27, DDRPHY_REG37, DDRPHY_REG47, DDRPHY_REG57*/
	DQS_DLL_DELAY_SHIFT		= 0,
	DQS_DLL_DELAY_MASK		= 7 << DQS_DLL_DELAY_SHIFT,
	DQS_DLL_DELAY_NO_DELAY		= 0 << DQS_DLL_DELAY_SHIFT,
	DQS_DLL_DELAY_22P5		= 1 << DQS_DLL_DELAY_SHIFT,
	DQS_DLL_DELAY_45		= 2 << DQS_DLL_DELAY_SHIFT,
	DQS_DLL_DELAY_67P5		= 3 << DQS_DLL_DELAY_SHIFT,
	DQS_DLL_DELAY_90		= 4 << DQS_DLL_DELAY_SHIFT,
	DQS_DLL_DELAY_112P5		= 5 << DQS_DLL_DELAY_SHIFT,
	DQS_DLL_DELAY_135		= 6 << DQS_DLL_DELAY_SHIFT,
	DQS_DLL_DELAY_157P5		= 7 << DQS_DLL_DELAY_SHIFT,

	DQS_DLL_CLOCK_PHASE_SHIFT	= 4,
	DQS_DLL_CLOCK_PHASE_MASK	= 1 << DQS_DLL_CLOCK_PHASE_SHIFT,
	DQS_DLL_CLOCK_PHASE_NO_DELAY	= 0 << DQS_DLL_CLOCK_PHASE_SHIFT,
	DQS_DLL_CLOCK_PHASE_180_DELAY	= 1 << DQS_DLL_CLOCK_PHASE_SHIFT,

	/* DDRPHY_REG28, DDRPHY_REG38, DDRPHY_REG48, DDRPHY_REG58*/
	RX_DQS_DLL_DELAY_SHIFT		= 0,
	RX_DQS_DLL_DELAY_MASK		= 3 << RX_DQS_DLL_DELAY_SHIFT,
	RX_DQS_DLL_NO_DELAY		= 0 << RX_DQS_DLL_DELAY_SHIFT,
	RX_DQS_DLL_DELAY_22P5		= 1 << RX_DQS_DLL_DELAY_SHIFT,
	RX_DQS_DLL_DELAY_45		= 2 << RX_DQS_DLL_DELAY_SHIFT,
	RX_DQS_DLL_DELAY_67P5		= 3 << RX_DQS_DLL_DELAY_SHIFT,

	/* PHY_REGA4 */
	RIGHT_CHN_B_TX_DQ_BYPASS_SHIFT	= 4,
	RIGHT_CHN_B_TX_DQ_BYPASS_MASK	= 1 << RIGHT_CHN_B_TX_DQ_BYPASS_SHIFT,
	RIGHT_CHN_B_TX_DQ_BYPASS_SET	= 1 << RIGHT_CHN_B_TX_DQ_BYPASS_SHIFT,

	LEFT_CHN_B_TX_DQ_BYPASS_SHIFT	= 3,
	LEFT_CHN_B_TX_DQ_BYPASS_MASK	= 1 << LEFT_CHN_B_TX_DQ_BYPASS_SHIFT,
	LEFT_CHN_B_TX_DQ_BYPASS_SET	= 1 << LEFT_CHN_B_TX_DQ_BYPASS_SHIFT,

	RIGHT_CHN_A_TX_DQ_BYPASS_SHIFT	= 2,
	RIGHT_CHN_A_TX_DQ_BYPASS_MASK	= 1 << RIGHT_CHN_A_TX_DQ_BYPASS_SHIFT,
	RIGHT_CHN_A_TX_DQ_BYPASS_SET	= 1 << RIGHT_CHN_A_TX_DQ_BYPASS_SHIFT,

	LEFT_CHN_A_TX_DQ_BYPASS_SHIFT	= 1,
	LEFT_CHN_A_TX_DQ_BYPASS_MASK	= 1 << LEFT_CHN_A_TX_DQ_BYPASS_SHIFT,
	LEFT_CHN_A_TX_DQ_BYPASS_SET	= 1 << LEFT_CHN_A_TX_DQ_BYPASS_SHIFT,

	CK_DLL_DLL_BYPASS_SHIFT		= 0,
	CK_DLL_DLL_BYPASS_MASK		= 1 << CK_DLL_DLL_BYPASS_SHIFT,
	CK_DLL_DLL_BYPASS_SET		= 1 << CK_DLL_DLL_BYPASS_SHIFT,

	/* PHY_REGFF */
	CHN_A_TRAINING_DONE_SHIFT	= 0,
	CHN_A_TRAINING_DONE_MASK	= 3 << CHN_A_TRAINING_DONE_SHIFT,
	CHN_A_HIGH_8BIT_TRAINING_DONE	= 2 << CHN_A_TRAINING_DONE_SHIFT,
	CHN_A_LOW_8BIT_TRAINING_DONE	= 1 << CHN_A_TRAINING_DONE_SHIFT,
	CHN_B_TRAINING_DONE_SHIFT	= 2,
	CHN_B_TRAINING_DONE_MASK	= 3 << CHN_B_TRAINING_DONE_SHIFT,
	CHN_B_HIGH_8BIT_TRAINING_DONE	= 2 << CHN_B_TRAINING_DONE_SHIFT,
	CHN_B_LOW_8BIT_TRAINING_DONE	= 1 << CHN_B_TRAINING_DONE_SHIFT,
};

typedef union {
	unsigned int noc_timing;
	struct {
		unsigned int acttoact:6;
		unsigned int rdtomiss:6;
		unsigned int wrtomiss:6;
		unsigned int burstlen:3;
		unsigned int rdtowr:5;
		unsigned int wrtord:5;
		unsigned int bwratio:1;
	};
} rk3128_noc_timing;

struct rk3128_pctl_timing {
	unsigned short togcnt1u;
	unsigned short tinit;
	unsigned short trsth;
	unsigned short togcnt100n;
	unsigned short trefi;
	unsigned short tmrd;
	unsigned short trfc;
	unsigned short trp;
	unsigned short trtw;
	unsigned short tal;
	unsigned short tcl;
	unsigned short tcwl;
	unsigned short tras;
	unsigned short trc;
	unsigned short trcd;
	unsigned short trrd;
	unsigned short trtp;
	unsigned short twr;
	unsigned short twtr;
	unsigned short texsr;
	unsigned short txp;
	unsigned short txpdll;
	unsigned short tzqcs;
	unsigned short tzqcsi;
	unsigned short tdqs;
	unsigned short tcksre;
	unsigned short tcksrx;
	unsigned short tcke;
	unsigned short tmod;
	unsigned short trstl;
	unsigned short tzqcl;
	unsigned short tmrr;
	unsigned short tckesr;
	unsigned short tdpd;
};

struct rk3128_phy_timing {
	unsigned short mr[4];
	unsigned short bl;
	unsigned short cl_al;
};

struct rk3128_ddr_timing {
	struct sdram_base_params base;
	struct rk3128_pctl_timing pctl_timing;
	struct rk3128_phy_timing phy_timing;
	rk3128_noc_timing noc_timing;
};

struct rk3128_service_sys {
	unsigned int id_coreid;
	unsigned int id_revisionid;
	unsigned int ddrconf;
	unsigned int ddrtiming;
	unsigned int ddrmode;
	unsigned int readlatency;
};

struct rk3128_ddr_pctl {
	unsigned int scfg;
	unsigned int sctl;
	unsigned int stat;
	unsigned int intrstat;
	unsigned int reserved0[12];
	unsigned int mcmd;
	unsigned int powctl;
	unsigned int powstat;
	unsigned int cmdtstat;
	unsigned int cmdtstaten;
	unsigned int reserved1[3];
	unsigned int mrrcfg0;
	unsigned int mrrstat0;
	unsigned int mrrstat1;
	unsigned int reserved2[4];
	unsigned int mcfg1;
	unsigned int mcfg;
	unsigned int ppcfg;
	unsigned int mstat;
	unsigned int lpddr2zqcfg;
	unsigned int reserved3;
	unsigned int dtupdes;
	unsigned int dtuna;
	unsigned int dtune;
	unsigned int dtuprd0;
	unsigned int dtuprd1;
	unsigned int dtuprd2;
	unsigned int dtuprd3;
	unsigned int dtuawdt;
	unsigned int reserved4[3];
	unsigned int togcnt1u;
	unsigned int tinit;
	unsigned int trsth;
	unsigned int togcnt100n;
	unsigned int trefi;
	unsigned int tmrd;
	unsigned int trfc;
	unsigned int trp;
	unsigned int trtw;
	unsigned int tal;
	unsigned int tcl;
	unsigned int tcwl;
	unsigned int tras;
	unsigned int trc;
	unsigned int trcd;
	unsigned int trrd;
	unsigned int trtp;
	unsigned int twr;
	unsigned int twtr;
	unsigned int texsr;
	unsigned int txp;
	unsigned int txpdll;
	unsigned int tzqcs;
	unsigned int tzqcsi;
	unsigned int tdqs;
	unsigned int tcksre;
	unsigned int tcksrx;
	unsigned int tcke;
	unsigned int tmod;
	unsigned int trstl;
	unsigned int tzqcl;
	unsigned int tmrr;
	unsigned int tckesr;
	unsigned int reserved5[47];
	unsigned int dtuwactl;
	unsigned int dturactl;
	unsigned int dtucfg;
	unsigned int dtuectl;
	unsigned int dtuwd0;
	unsigned int dtuwd1;
	unsigned int dtuwd2;
	unsigned int dtuwd3;
	unsigned int dtuwdm;
	unsigned int dturd0;
	unsigned int dturd1;
	unsigned int dturd2;
	unsigned int dturd3;
	unsigned int dtulfsrwd;
	unsigned int dtulfsrrd;
	unsigned int dtueaf;
	unsigned int dfitctrldelay;
	unsigned int dfiodtcfg;
	unsigned int dfiodtcfg1;
	unsigned int dfiodtrankmap;
	unsigned int dfitphywrdata;
	unsigned int dfitphywrlat;
	unsigned int reserved7[2];
	unsigned int dfitrddataen;
	unsigned int dfitphyrdlat;
	unsigned int reserved8[2];
	unsigned int dfitphyupdtype0;
	unsigned int dfitphyupdtype1;
	unsigned int dfitphyupdtype2;
	unsigned int dfitphyupdtype3;
	unsigned int dfitctrlupdmin;
	unsigned int dfitctrlupdmax;
	unsigned int dfitctrlupddly;
	unsigned int reserved9;
	unsigned int dfiupdcfg;
	unsigned int dfitrefmski;
	unsigned int dfitctrlupdi;
	unsigned int reserved10[4];
	unsigned int dfitrcfg0;
	unsigned int dfitrstat0;
	unsigned int dfitrwrlvlen;
	unsigned int dfitrrdlvlen;
	unsigned int dfitrrdlvlgateen;
	unsigned int dfiststat0;
	unsigned int dfistcfg0;
	unsigned int dfistcfg1;
	unsigned int reserved11;
	unsigned int dfitdramclken;
	unsigned int dfitdramclkdis;
	unsigned int dfistcfg2;
	unsigned int dfistparclr;
	unsigned int dfistparlog;
	unsigned int reserved12[3];
	unsigned int dfilpcfg0;
	unsigned int reserved13[3];
	unsigned int dfitrwrlvlresp0;
	unsigned int dfitrwrlvlresp1;
	unsigned int dfitrwrlvlresp2;
	unsigned int dfitrrdlvlresp0;
	unsigned int dfitrrdlvlresp1;
	unsigned int dfitrrdlvlresp2;
	unsigned int dfitrwrlvldelay0;
	unsigned int dfitrwrlvldelay1;
	unsigned int dfitrwrlvldelay2;
	unsigned int dfitrrdlvldelay0;
	unsigned int dfitrrdlvldelay1;
	unsigned int dfitrrdlvldelay2;
	unsigned int dfitrrdlvlgatedelay0;
	unsigned int dfitrrdlvlgatedelay1;
	unsigned int dfitrrdlvlgatedelay2;
	unsigned int dfitrcmd;
	unsigned int reserved14[46];
	unsigned int ipvr;
	unsigned int iptr;
};

check_member(rk3128_ddr_pctl, iptr, 0x03fc);

int rk3128_sdram_init(enum rk3128_dram_config);

#endif
