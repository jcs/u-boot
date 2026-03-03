// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2008-2014 Rockchip Electronics
 */

#include <clk.h>
#include <display.h>
#include <dm.h>
#include <linux/delay.h>
#include <log.h>
#include <reset.h>
#include <video.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch-rockchip/vop_rk3288.h>
#include "rk_vop.h"

DECLARE_GLOBAL_DATA_PTR;

enum rk3126_vop_pol {
	HSYNC_POSITIVE = 0,
	VSYNC_POSITIVE = 1,
	DEN_NEGATIVE   = 2,
	DCLK_INVERT    = 3
};

/*
 * RK312x VOP register offsets - confirmed against vendor register dump.
 * Shadow registers: writes take effect after REG_CFG_DONE + vsync.
 */
#define SYS_CTRL		0x00
#define DSP_CTRL0		0x04
#define DSP_CTRL1		0x08
#define WIN0_COLOR_KEY		0x18
#define WIN0_YRGB_MST		0x20
#define AXI_BUS_CTRL		0x2c
#define WIN0_VIR		0x30
#define WIN0_ACT_INFO		0x34
#define WIN0_DSP_INFO		0x38
#define WIN0_DSP_ST		0x3c
#define DSP_HTOTAL_HS_END	0x6c
#define DSP_HACT_ST_END		0x70
#define DSP_VTOTAL_VS_END	0x74
#define DSP_VACT_ST_END		0x78
#define REG_CFG_DONE		0x90

/* SYS_CTRL bits - WIN0 enable/format are here, not in a separate register */
#define m_WIN0_EN		BIT(0)
#define m_WIN0_FORMAT		(0x7 << 3)
#define v_WIN0_FORMAT(x)	(((x) & 0x7) << 3)
#define m_WIN0_RB_SWAP		BIT(15)
#define m_WIN0_OTSD_DISABLE	BIT(22)
#define m_DMA_STOP		BIT(29)
#define m_LCDC_STANDBY		BIT(30)

/* AXI_BUS_CTRL bits - display clock enables for rk312x */
#define m_LVDS_DCLK_INVERT	BIT(27)
#define m_LVDS_DCLK_EN		BIT(26)
#define m_RGB_DCLK_INVERT	BIT(25)
#define m_RGB_DCLK_EN		BIT(24)
#define m_HDMI_DCLK_INVERT	BIT(23)
#define m_HDMI_DCLK_EN		BIT(22)

/* VOP format IDs (different from rk3288!) */
#define VOP_FMT_ARGB888		0
#define VOP_FMT_RGB888		1
#define VOP_FMT_RGB565		2

/*
 * CRU clock gate registers - ungate VOP clocks that the clock driver
 * doesn't handle.  Write-mask: upper 16 = mask, lower 16 = value.
 * Value bit = 1 means gated, 0 means ungated.
 */
#define CRU_BASE		0x20000000
#define CRU_CLKGATE_CON(x)	(CRU_BASE + 0xd0 + (x) * 4)

static void rk3126_vop_ungate_clocks(void)
{
	/* CLKGATE_CON3: bit1=DCLK_VOP, bit2=DCLK_LCDC */
	writel(BIT(17) | BIT(18), (void __iomem *)CRU_CLKGATE_CON(3));
	/* CLKGATE_CON6: bit0=ACLK_VIO0, bit2=HCLK_VIO */
	writel(BIT(16) | BIT(18), (void __iomem *)CRU_CLKGATE_CON(6));
	/* CLKGATE_CON9: bit5=HCLK_LCDC0, bit6=ACLK_LCDC0 */
	writel(BIT(21) | BIT(22), (void __iomem *)CRU_CLKGATE_CON(9));
}

static void rk3126_mode_set(struct udevice *dev,
			    const struct display_timing *edid,
			    enum vop_modes mode)
{
	struct rk_vop_priv *priv = dev_get_priv(dev);
	void __iomem *base = priv->regs;
	u32 hsync_len = edid->hsync_len.typ;
	u32 hback_porch = edid->hback_porch.typ;
	u32 vsync_len = edid->vsync_len.typ;
	u32 vback_porch = edid->vback_porch.typ;
	u32 hfront_porch = edid->hfront_porch.typ;
	u32 vfront_porch = edid->vfront_porch.typ;
	u32 hactive = edid->hactive.typ;
	u32 vactive = edid->vactive.typ;

	rk3126_vop_ungate_clocks();

	/* Match vendor register values exactly */
	writel(0, base + SYS_CTRL);          /* clear standby/DMA stop */
	writel(0, base + DSP_CTRL0);         /* no polarity inversion, RGB888 */
	writel(0, base + DSP_CTRL1);         /* clear BLANK_EN/BLACK_EN */
	writel(m_LVDS_DCLK_EN | m_LVDS_DCLK_INVERT,
	       base + AXI_BUS_CTRL);         /* vendor: 0x0c000000 */

	/* Display timing */
	writel(V_HSYNC(hsync_len) |
	       V_HORPRD(hsync_len + hback_porch + hactive + hfront_porch),
	       base + DSP_HTOTAL_HS_END);

	writel(V_HEAP(hsync_len + hback_porch + hactive) |
	       V_HASP(hsync_len + hback_porch),
	       base + DSP_HACT_ST_END);

	writel(V_VSYNC(vsync_len) |
	       V_VERPRD(vsync_len + vback_porch + vactive + vfront_porch),
	       base + DSP_VTOTAL_VS_END);

	writel(V_VAEP(vsync_len + vback_porch + vactive) |
	       V_VASP(vsync_len + vback_porch),
	       base + DSP_VACT_ST_END);

	writel(0x01, base + REG_CFG_DONE);
}

static void rk3126_enable(struct udevice *dev, ulong fbbase,
			  int fb_bits_per_pixel,
			  const struct display_timing *edid,
			  struct reset_ctl *dclk_rst)
{
	struct rk_vop_priv *priv = dev_get_priv(dev);
	void __iomem *base = priv->regs;
	u32 vop_fmt;
	u32 hactive = edid->hactive.typ;
	u32 vactive = edid->vactive.typ;

	/* Re-ungate clocks (clk_set_rate may have disrupted them) */
	rk3126_vop_ungate_clocks();

	switch (fb_bits_per_pixel) {
	case 16:
		vop_fmt = VOP_FMT_RGB565;
		writel(DIV_ROUND_UP(hactive, 2), base + WIN0_VIR);
		break;
	case 24:
		vop_fmt = VOP_FMT_RGB888;
		writel(DIV_ROUND_UP(hactive * 3, 4), base + WIN0_VIR);
		break;
	case 32:
	default:
		vop_fmt = VOP_FMT_ARGB888;
		writel(hactive, base + WIN0_VIR);
		break;
	}

	writel(V_ACT_WIDTH(hactive - 1) | V_ACT_HEIGHT(vactive - 1),
	       base + WIN0_ACT_INFO);
	writel(V_DSP_WIDTH(hactive - 1) | V_DSP_HEIGHT(vactive - 1),
	       base + WIN0_DSP_INFO);
	writel(V_DSP_XST(edid->hsync_len.typ + edid->hback_porch.typ) |
	       V_DSP_YST(edid->vsync_len.typ + edid->vback_porch.typ),
	       base + WIN0_DSP_ST);
	writel(fbbase, base + WIN0_YRGB_MST);

	/* SYS_CTRL: enable WIN0 with format (vendor: 0x11 for RGB565) */
	writel(m_WIN0_EN | v_WIN0_FORMAT(vop_fmt), base + SYS_CTRL);

	/* Commit all shadow registers */
	writel(0x01, base + REG_CFG_DONE);
}

static void rk3126_enable_output(struct udevice *dev, enum vop_modes mode)
{
	/* Output enable is handled in rk3126_mode_set - nothing to do here */
}

struct rkvop_driverdata rk3126_driverdata = {
	.mode_set = rk3126_mode_set,
	.enable = rk3126_enable,
	.enable_output = rk3126_enable_output,
};

static const struct udevice_id rk3126_vop_ids[] = {
	{ .compatible = "rockchip,rk3126-vop",
	  .data = (ulong)&rk3126_driverdata },
	{ }
};

static const struct video_ops rk3126_vop_ops = {
};

U_BOOT_DRIVER(rockchip_rk3126_vop) = {
	.name	= "rockchip_rk3126_vop",
	.id	= UCLASS_VIDEO,
	.of_match = rk3126_vop_ids,
	.ops	= &rk3126_vop_ops,
	.bind	= rk_vop_bind,
	.probe	= rk_vop_probe,
	.priv_auto = sizeof(struct rk_vop_priv),
	.flags	= DM_FLAG_PRE_RELOC,
};
