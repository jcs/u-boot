// SPDX-License-Identifier: MIT
/*
 * Toshiba TC3589x I2C keypad controller driver
 *
 * Copyright (c) 2026 joshua stein <jcs@jcs.org>
 */

#include <dm.h>
#include <i2c.h>
#include <input.h>
#include <keyboard.h>
#include <log.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <linux/input.h>

/* keypad registers */
#define TC3589x_KBDSETTLE_REG		0x01
#define  TC3589x_KPD_SETTLE_TIME	0xa3
#define TC3589x_KBDBOUNCE		0x02
#define  TC3589x_KPD_DEBOUNCE_PERIOD	0xa3
#define TC3589x_KBDSIZE			0x03
#define TC3589x_KBCFG_LSB		0x04
#define TC3589x_KBCFG_MSB		0x05
#define  TC3589x_KBCFG_DEDICATED_KEY	0xff
#define TC3589x_KBDIC			0x08
#define  TC3589x_EVT_INT_CLR		0x2
#define  TC3589x_KBD_INT_CLR		0x1
#define TC3589x_KBDMSK			0x09
#define  TC3589x_EVT_LOSS_INT		0x8
#define  TC3589x_EVT_INT		0x4
#define TC3589x_KBDCODE0		0x0b
#define  TC3589x_KBDCODE_FIFO_EMPTY	0x7f
#define  TC3589x_KBDCODE_COL_MASK	0x0f
#define  TC3589x_KBDCODE_ROW_MASK	0x70
#define  TC3589x_KBDCODE_ROW_SHIFT	4
#define  TC3589x_KBDCODE_NREGS		4

/* system registers */
#define TC3589x_MANFCODE		0x80
#define  TC3589x_MANFCODE_MAGIC		0x03
#define TC3589x_VERSION			0x81
#define TC3589x_RSTCTRL			0x82
#define  TC3589x_RSTCTRL_KBDRST		(1 << 1)
#define  TC3589x_RSTCTRL_ROTRST		(1 << 2)
#define  TC3589x_RSTCTRL_TIMRST		(1 << 3)
#define TC3589x_RSTINTCLR		0x84
#define TC3589x_CLKCFG			0x89
#define TC3589x_CLKEN			0x8a
#define  TC3589x_CLKEN_KPD		0x1
#define TC3589x_KBDMFS			0x8f
#define  TC3589x_KBDMFS_EN		0x1
#define TC3589x_IRQST			0x91
#define  TC3589x_IRQST_PORIRQ		(1 << 7)

/* IO configuration registers */
#define TC3589x_IOCFG			0xa7
#define  TC3589x_IOCFG_IG		0x08
#define TC3589x_IOPULLCFG0_LSB		0xaa
#define TC3589x_IOPULLCFG0_MSB		0xab
#define TC3589x_IOPULLCFG1_LSB		0xac
#define TC3589x_IOPULLCFG1_MSB		0xad
#define TC3589x_IOPULLCFG2_LSB		0xae
#define  TC3589x_PULLUP_ALL		0xaa

/* direct keyboard registers */
#define TC3589x_DKBDIC			0xf2
#define TC3589x_DKBDMSK			0xf3

#define TC3589x_MAX_ROWS		8
#define TC3589x_MAX_KMAP_ROWS		16	/* including Fn layer */
#define TC3589x_MAX_COLS		12

struct tc3589x_keyb_priv {
	struct gpio_desc reset_gpio;
	int rows;
	int cols;
	u16 keymap[TC3589x_MAX_KMAP_ROWS][TC3589x_MAX_COLS];
	u16 states[TC3589x_MAX_ROWS + 1];
};

static int tc3589x_read_keys(struct input_config *input)
{
	struct udevice *dev = input->dev;
	struct tc3589x_keyb_priv *priv = dev_get_priv(dev);
	u16 new_states[TC3589x_MAX_ROWS + 1] = { 0 };
	int col, row, i, was, now, code, ret;
	u8 k;

	/* clear keyboard interrupt before reading keys */
	dm_i2c_reg_clrset(dev, TC3589x_KBDIC, 0x0,
			  TC3589x_EVT_INT_CLR | TC3589x_KBD_INT_CLR);

	/* read pressed keys from KBDCODE registers */
	for (i = 0; i < TC3589x_KBDCODE_NREGS; i++) {
		ret = dm_i2c_reg_read(dev, TC3589x_KBDCODE0 + i);
		if (ret < 0)
			continue;
		k = ret;
		if (k == TC3589x_KBDCODE_FIFO_EMPTY)
			continue;

		row = (k & TC3589x_KBDCODE_ROW_MASK) >>
		    TC3589x_KBDCODE_ROW_SHIFT;
		col = k & TC3589x_KBDCODE_COL_MASK;
		new_states[row] |= (1 << col);
	}

	/* collect state changes */
	for (row = 0; row < priv->rows; row++) {
		for (col = 0; col < priv->cols; col++) {
			was = priv->states[row] & (1 << col);
			now = new_states[row] & (1 << col);

			code = priv->keymap[row][col];
			if (code == 0)
				continue;

			if (!was && now)
				input_add_keycode(input, code, false);
			else if (was && !now)
				input_add_keycode(input, code, true);
		}

		priv->states[row] = new_states[row];
	}

	return 0;
}

static int tc3589x_keyb_probe(struct udevice *dev)
{
	struct tc3589x_keyb_priv *priv = dev_get_priv(dev);
	struct keyboard_priv *uc_priv = dev_get_uclass_priv(dev);
	struct stdio_dev *sdev = &uc_priv->sdev;
	struct input_config *input = &uc_priv->input;
	ofnode keypad_node = ofnode_null();
	ofnode child;
	const u32 *keymap_prop;
	int keymap_len, nkeys, i, row, col, reg, ret;

	/* hardware reset: assert for 1ms, wait 1ms after deassert */
	ret = gpio_request_by_name(dev, "reset-gpios", 0, &priv->reset_gpio,
				   GPIOD_IS_OUT);
	if (!ret) {
		dm_gpio_set_value(&priv->reset_gpio, 1);
		udelay(1000);
		dm_gpio_set_value(&priv->reset_gpio, 0);
		udelay(1000);
	}

	/* wait for power-on-reset complete */
	for (i = 0; i < 10; i++) {
		reg = dm_i2c_reg_read(dev, TC3589x_IRQST);
		if (reg >= 0 && (reg & TC3589x_IRQST_PORIRQ)) {
			dm_i2c_reg_write(dev, TC3589x_RSTINTCLR, 0x01);
			break;
		}
		udelay(1000);
	}

	/* clear direct key interrupt */
	dm_i2c_reg_write(dev, TC3589x_DKBDIC, 0x01);

	/*
	 * "After power-on-reset, the general call functionality can be used
	 * only after having read out the manufacturer code (0x80) and the
	 * software version number (0x81) of the TC35894FG."
	 */
	reg = dm_i2c_reg_read(dev, TC3589x_MANFCODE);
	if (reg != TC3589x_MANFCODE_MAGIC) {
		printf("tc3589x: invalid manfcode 0x%x\n", reg);
		return -ENODEV;
	}
	dm_i2c_reg_read(dev, TC3589x_VERSION);

	/* put timer, rotary, and keyboard into reset */
	dm_i2c_reg_write(dev, TC3589x_RSTCTRL,
			 TC3589x_RSTCTRL_TIMRST | TC3589x_RSTCTRL_ROTRST |
			 TC3589x_RSTCTRL_KBDRST);

	/* clear reset interrupt */
	dm_i2c_reg_write(dev, TC3589x_RSTINTCLR, 0x1);

	/* mask direct keyboard interrupts */
	dm_i2c_reg_write(dev, TC3589x_DKBDMSK, 0x03);

	/* find keypad child node */
	ofnode_for_each_subnode(child, dev_ofnode(dev)) {
		if (ofnode_device_is_compatible(child,
					       "toshiba,tc3589x-keypad")) {
			keypad_node = child;
			break;
		}
	}

	if (!ofnode_valid(keypad_node)) {
		printf("tc3589x: no keypad child node\n");
		return -ENODEV;
	}

	priv->rows = ofnode_read_u32_default(keypad_node,
					     "keypad,num-rows", 0);
	priv->cols = ofnode_read_u32_default(keypad_node,
					     "keypad,num-columns", 0);
	if (priv->rows < 1 || priv->rows > TC3589x_MAX_ROWS ||
	    priv->cols < 1 || priv->cols > TC3589x_MAX_COLS) {
		printf("tc3589x: invalid rows %d cols %d\n",
		       priv->rows, priv->cols);
		return -EINVAL;
	}

	/* parse linux,keymap: each entry is (row << 24 | col << 16 | code) */
	keymap_prop = ofnode_read_prop(keypad_node, "linux,keymap",
				      &keymap_len);
	if (keymap_prop && keymap_len > 0) {
		nkeys = keymap_len / sizeof(u32);
		for (i = 0; i < nkeys; i++) {
			u32 entry = fdt32_to_cpu(keymap_prop[i]);

			row = (entry >> 24) & 0xff;
			col = (entry >> 16) & 0xff;
			if (row < TC3589x_MAX_KMAP_ROWS &&
			    col < TC3589x_MAX_COLS)
				priv->keymap[row][col] = entry & 0xffff;
		}
	}

	/* pull the keypad module out of reset */
	dm_i2c_reg_clrset(dev, TC3589x_RSTCTRL, TC3589x_RSTCTRL_KBDRST, 0x0);

	/* configure KBDMFS */
	dm_i2c_reg_clrset(dev, TC3589x_KBDMFS, 0x0, TC3589x_KBDMFS_EN);

	/* configure clock */
	dm_i2c_reg_write(dev, TC3589x_CLKCFG, 0x43);

	/* enable the keypad clock */
	dm_i2c_reg_clrset(dev, TC3589x_CLKEN, 0x0, TC3589x_CLKEN_KPD);

	/* clear pending IRQs */
	dm_i2c_reg_clrset(dev, TC3589x_RSTINTCLR, 0x0, 0x1);

	/* init keyboard matrix size */
	dm_i2c_reg_write(dev, TC3589x_KBDSIZE,
			 (priv->rows << TC3589x_KBDCODE_ROW_SHIFT) |
			 priv->cols);

	/* no dedicated key selected */
	dm_i2c_reg_write(dev, TC3589x_KBCFG_LSB, TC3589x_KBCFG_DEDICATED_KEY);
	dm_i2c_reg_write(dev, TC3589x_KBCFG_MSB, TC3589x_KBCFG_DEDICATED_KEY);

	/* configure settle time */
	dm_i2c_reg_write(dev, TC3589x_KBDSETTLE_REG,
			 TC3589x_KPD_SETTLE_TIME);

	/* configure debounce time */
	dm_i2c_reg_write(dev, TC3589x_KBDBOUNCE,
			 TC3589x_KPD_DEBOUNCE_PERIOD);

	/* configure keypad GPIOs */
	dm_i2c_reg_clrset(dev, TC3589x_IOCFG, 0x0, TC3589x_IOCFG_IG);

	/* configure pull-up resistors for all row GPIOs */
	dm_i2c_reg_write(dev, TC3589x_IOPULLCFG0_LSB, TC3589x_PULLUP_ALL);
	dm_i2c_reg_write(dev, TC3589x_IOPULLCFG0_MSB, TC3589x_PULLUP_ALL);

	/* configure pull-up resistors for all column GPIOs */
	dm_i2c_reg_write(dev, TC3589x_IOPULLCFG1_LSB, TC3589x_PULLUP_ALL);
	dm_i2c_reg_write(dev, TC3589x_IOPULLCFG1_MSB, TC3589x_PULLUP_ALL);
	dm_i2c_reg_write(dev, TC3589x_IOPULLCFG2_LSB, TC3589x_PULLUP_ALL);

	/* enable keyboard IRQs (needed for FIFO to fill even in polling) */
	dm_i2c_reg_clrset(dev, TC3589x_KBDMSK, 0x0,
			  TC3589x_EVT_LOSS_INT | TC3589x_EVT_INT);

	/* set up input layer */
	input->dev = dev;
	input->read_keys = tc3589x_read_keys;
	input_add_tables(input, false);

	strcpy(sdev->name, "tc3589x-keyb");
	ret = input_stdio_register(sdev);
	if (ret) {
		printf("tc3589x: failed to register stdio device\n");
		return ret;
	}

	return 0;
}

static const struct keyboard_ops tc3589x_keyb_ops = {
};

static const struct udevice_id tc3589x_keyb_ids[] = {
	{ .compatible = "toshiba,tc35894" },
	{ }
};

U_BOOT_DRIVER(tc3589x_keyb) = {
	.name		= "tc3589x_keyb",
	.id		= UCLASS_KEYBOARD,
	.of_match	= tc3589x_keyb_ids,
	.probe		= tc3589x_keyb_probe,
	.ops		= &tc3589x_keyb_ops,
	.priv_auto	= sizeof(struct tc3589x_keyb_priv),
};
