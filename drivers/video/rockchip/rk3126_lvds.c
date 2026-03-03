// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2008-2014 Rockchip Electronics
 */

#include <display.h>
#include <dm.h>
#include <log.h>
#include <panel.h>
#include <regmap.h>
#include <syscon.h>
#include <generic-phy.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/hardware.h>
#include <linux/err.h>

DECLARE_GLOBAL_DATA_PTR;

/* GRF_LVDS_CON0 offset within GRF */
#define RK3126_GRF_LVDS_CON0	0x150

/*
 * GRF_LVDS_CON0 bits for rk312x (from vendor rk312x_lcdc.h)
 * Uses write-mask: upper 16 bits = mask, lower 16 bits = value
 */
#define LVDS_CON0_DATA_SEL(x)		(((x) & 1) << 0)  /* 0=from LCDC */
#define LVDS_CON0_OUTPUT_FORMAT(x)	(((x) & 3) << 1)  /* format */
#define LVDS_CON0_MSBSEL		BIT(3)             /* 1=MSB at D7 */
#define LVDS_CON0_LVDSMODE_EN		BIT(6)             /* LVDS mode enable */
#define LVDS_CON0_TTL_EN		BIT(7)             /* TTL mode enable */
#define LVDS_CON0_LANE0_EN		BIT(8)             /* MIPI PHY lane0 */
#define LVDS_CON0_FORCEX_EN		BIT(9)             /* DPI force enable */

/* LVDS output format values */
#define LVDS_FMT_8BIT_1	0
#define LVDS_FMT_8BIT_2	1
#define LVDS_FMT_8BIT_3	2  /* JEIDA-24 */
#define LVDS_FMT_6BIT	3

struct rk3126_lvds_priv {
	void __iomem *grf;
	struct udevice *panel;
	struct phy phy;
	const char *data_mapping;
	int data_width;
	const char *output;
};

static int rk3126_lvds_read_timing(struct udevice *dev,
				   struct display_timing *timing)
{
	struct rk3126_lvds_priv *priv = dev_get_priv(dev);
	ofnode panel_node, timing_node;

	panel_node = dev_ofnode(priv->panel);

	/* Try display-timings first, then panel-timing */
	if (!ofnode_decode_display_timing(panel_node, 0, timing))
		return 0;

	timing_node = ofnode_find_subnode(panel_node, "panel-timing");
	if (!ofnode_valid(timing_node)) {
		debug("%s: no panel-timing subnode\n", __func__);
		return -EINVAL;
	}

	memset(timing, 0, sizeof(*timing));

	timing->hback_porch.typ = ofnode_read_u32_default(timing_node,
							  "hback-porch", 0);
	timing->hfront_porch.typ = ofnode_read_u32_default(timing_node,
							   "hfront-porch", 0);
	timing->hactive.typ = ofnode_read_u32_default(timing_node,
						      "hactive", 0);
	timing->hsync_len.typ = ofnode_read_u32_default(timing_node,
							"hsync-len", 0);
	timing->vback_porch.typ = ofnode_read_u32_default(timing_node,
							  "vback-porch", 0);
	timing->vfront_porch.typ = ofnode_read_u32_default(timing_node,
							   "vfront-porch", 0);
	timing->vactive.typ = ofnode_read_u32_default(timing_node,
						      "vactive", 0);
	timing->vsync_len.typ = ofnode_read_u32_default(timing_node,
							"vsync-len", 0);
	timing->pixelclock.typ = ofnode_read_u32_default(timing_node,
							 "clock-frequency", 0);

	timing->flags = 0;
	if (ofnode_read_u32_default(timing_node, "hsync-active", 0))
		timing->flags |= DISPLAY_FLAGS_HSYNC_HIGH;
	if (ofnode_read_u32_default(timing_node, "vsync-active", 0))
		timing->flags |= DISPLAY_FLAGS_VSYNC_HIGH;
	if (ofnode_read_u32_default(timing_node, "de-active", 0))
		timing->flags |= DISPLAY_FLAGS_DE_HIGH;
	if (ofnode_read_u32_default(timing_node, "pixelclk-active", 0))
		timing->flags |= DISPLAY_FLAGS_PIXDATA_POSEDGE;

	return 0;
}

static int rk3126_lvds_enable(struct udevice *dev, int panel_bpp,
			      const struct display_timing *edid)
{
	struct rk3126_lvds_priv *priv = dev_get_priv(dev);
	u32 val;
	int ret;

	/*
	 * Configure GRF_LVDS_CON0 per vendor rk312x_lcdc.h.
	 * LVDS_8BIT_3 (=2) is the JEIDA-24 format.
	 */
	val = LVDS_CON0_LVDSMODE_EN |
	      LVDS_CON0_DATA_SEL(0) |
	      LVDS_CON0_OUTPUT_FORMAT(LVDS_FMT_8BIT_3) |
	      LVDS_CON0_MSBSEL |
	      LVDS_CON0_LANE0_EN |
	      LVDS_CON0_FORCEX_EN;

	/* Write with mask in upper 16 bits */
	writel(val | (0xffff << 16), priv->grf + RK3126_GRF_LVDS_CON0);

	/* Set PHY mode to LVDS and power on */
	ret = generic_phy_set_mode(&priv->phy, PHY_MODE_LVDS, 0);
	if (ret)
		return ret;

	ret = generic_phy_power_on(&priv->phy);
	if (ret)
		return ret;

	/*
	 * Drive panel enable GPIO and backlight directly.
	 * panel enable-gpios = GPIO0_C0 (pin 24)
	 * backlight enable-gpios = GPIO0_C3 (pin 27)
	 * PWM0 at 0x20050000, period=0x61a8 (25000ns = 40kHz)
	 */
#define GPIO0_BASE	0x2007c000
#define GPIO_SWPORTA_DR		0x00
#define GPIO_SWPORTA_DDR	0x04
#define PWM0_BASE	0x20050000
#define PWM_CNT		0x00
#define PWM_PERIOD	0x04
#define PWM_DUTY	0x08
#define PWM_CTRL	0x0c
	{
		void __iomem *gpio0 = (void __iomem *)GPIO0_BASE;
		void __iomem *pwm0 = (void __iomem *)PWM0_BASE;
		u32 val;

		/* Set GPIO0_C0 (pin 24) and GPIO0_C3 (pin 27) as output high */
		val = readl(gpio0 + GPIO_SWPORTA_DDR);
		val |= BIT(24) | BIT(27);
		writel(val, gpio0 + GPIO_SWPORTA_DDR);

		val = readl(gpio0 + GPIO_SWPORTA_DR);
		val |= BIT(24) | BIT(27);
		writel(val, gpio0 + GPIO_SWPORTA_DR);

		/* Configure PWM0: period=25000, duty=12500 (50%), enable */
		writel(25000, pwm0 + PWM_PERIOD);
		writel(12500, pwm0 + PWM_DUTY);
		writel(BIT(0) | BIT(3), pwm0 + PWM_CTRL); /* enable, continuous */
	}

	return 0;
}

static int rk3126_lvds_of_to_plat(struct udevice *dev)
{
	struct rk3126_lvds_priv *priv = dev_get_priv(dev);

	priv->grf = syscon_get_first_range(ROCKCHIP_SYSCON_GRF);
	if (IS_ERR(priv->grf))
		return PTR_ERR(priv->grf);

	priv->data_mapping = dev_read_string(dev, "rockchip,data-mapping");
	priv->data_width = dev_read_u32_default(dev, "rockchip,data-width", 24);
	priv->output = dev_read_string(dev, "rockchip,output");

	return 0;
}

static int rk3126_lvds_probe(struct udevice *dev)
{
	struct rk3126_lvds_priv *priv = dev_get_priv(dev);
	int ret;

	/* Find the panel via port@1 endpoint */
	ret = uclass_get_device_by_phandle(UCLASS_PANEL, dev, "rockchip,panel",
					   &priv->panel);
	if (ret) {
		/*
		 * No rockchip,panel phandle - try to find panel via
		 * port@1 remote-endpoint. Walk: ports/port@1/endpoint@0
		 * -> remote-endpoint -> parent -> parent -> panel device.
		 */
		ofnode ports, port, ep;
		u32 phandle;
		ofnode remote, panel_node;

		ports = dev_read_subnode(dev, "ports");
		if (!ofnode_valid(ports)) {
			debug("%s: no ports subnode\n", __func__);
			return -EINVAL;
		}

		port = ofnode_find_subnode(ports, "port@1");
		if (!ofnode_valid(port)) {
			debug("%s: no port@1\n", __func__);
			return -EINVAL;
		}

		/* Find first endpoint */
		ep = ofnode_first_subnode(port);
		if (!ofnode_valid(ep)) {
			debug("%s: no endpoint in port@1\n", __func__);
			return -EINVAL;
		}

		ret = ofnode_read_u32(ep, "remote-endpoint", &phandle);
		if (ret) {
			debug("%s: no remote-endpoint\n", __func__);
			return -EINVAL;
		}

		remote = ofnode_get_by_phandle(phandle);
		if (!ofnode_valid(remote)) {
			debug("%s: invalid remote phandle\n", __func__);
			return -EINVAL;
		}

		/* Walk up from the endpoint to the panel node */
		panel_node = ofnode_get_parent(remote);	/* port */
		panel_node = ofnode_get_parent(panel_node); /* panel */

		ret = uclass_get_device_by_ofnode(UCLASS_PANEL, panel_node,
						  &priv->panel);
		if (ret) {
			debug("%s: cannot find panel: %d\n", __func__, ret);
			return ret;
		}
	}

	/* Get PHY */
	ret = generic_phy_get_by_name(dev, "dphy", &priv->phy);
	if (ret) {
		debug("%s: cannot get dphy: %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static const struct dm_display_ops rk3126_lvds_ops = {
	.read_timing = rk3126_lvds_read_timing,
	.enable = rk3126_lvds_enable,
};

static const struct udevice_id rk3126_lvds_ids[] = {
	{ .compatible = "rockchip,rk3126-lvds" },
	{ }
};

U_BOOT_DRIVER(rk3126_lvds) = {
	.name	= "rk3126_lvds",
	.id	= UCLASS_DISPLAY,
	.of_match = rk3126_lvds_ids,
	.ops	= &rk3126_lvds_ops,
	.of_to_plat = rk3126_lvds_of_to_plat,
	.probe	= rk3126_lvds_probe,
	.priv_auto = sizeof(struct rk3126_lvds_priv),
};
