/*
 * Helpers for controlling modem lines via GPIO
 *
 * Copyright (C) 2014 Paratronic S.A.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/device.h>
#include <linux/irq.h>
#include <linux/gpio/consumer.h>
#include <linux/termios.h>
#include <linux/serial_core.h>

#include "serial_mctrl_gpio.h"

struct mctrl_gpios {
	struct uart_port *port;
	struct gpio_desc *gpio[UART_GPIO_MAX];
	int irq[UART_GPIO_MAX];
	unsigned int mctrl_prev;
	bool mctrl_on;
};

static const struct {
	const char *name;
	unsigned int mctrl;
	bool dir_out;
} mctrl_gpios_desc[UART_GPIO_MAX] = {
	{ "cts", TIOCM_CTS, false, },
	{ "dsr", TIOCM_DSR, false, },
	{ "dcd", TIOCM_CD, false, },
	{ "rng", TIOCM_RNG, false, },
	{ "rts", TIOCM_RTS, true, },
	{ "dtr", TIOCM_DTR, true, },
	{ "out1", TIOCM_OUT1, true, },
	{ "out2", TIOCM_OUT2, true, },
	{ "out3", TIOCM_OUT3, true, },
	{ "out4", TIOCM_OUT4, true, },
	{ "out5", TIOCM_OUT5, true, },
	{ "out6", TIOCM_OUT6, true, },
};

void mctrl_gpio_set(struct mctrl_gpios *gpios, unsigned int mctrl)
{
	enum mctrl_gpio_idx i;
	struct gpio_desc *desc_array[UART_GPIO_MAX];
	int value_array[UART_GPIO_MAX];
	unsigned int count = 0;

	for (i = 0; i < UART_GPIO_MAX; i++)
		if (gpios->gpio[i] && mctrl_gpios_desc[i].dir_out) {
			desc_array[count] = gpios->gpio[i];
			value_array[count] = !!(mctrl & mctrl_gpios_desc[i].mctrl);
			count++;
		}
	gpiod_set_array_value(count, desc_array, value_array);
}
EXPORT_SYMBOL_GPL(mctrl_gpio_set);

struct gpio_desc *mctrl_gpio_to_gpiod(struct mctrl_gpios *gpios,
				      enum mctrl_gpio_idx gidx)
{
	return gpios->gpio[gidx];
}
EXPORT_SYMBOL_GPL(mctrl_gpio_to_gpiod);

unsigned int mctrl_gpio_get(struct mctrl_gpios *gpios, unsigned int *mctrl)
{
	enum mctrl_gpio_idx i;

	for (i = 0; i < UART_GPIO_MAX; i++) {
		if (gpios->gpio[i] && !mctrl_gpios_desc[i].dir_out) {
			if (gpiod_get_value(gpios->gpio[i]))
				*mctrl |= mctrl_gpios_desc[i].mctrl;
			else
				*mctrl &= ~mctrl_gpios_desc[i].mctrl;
		}
	}

	return *mctrl;
}
EXPORT_SYMBOL_GPL(mctrl_gpio_get);

struct mctrl_gpios *mctrl_gpio_init_noauto(struct device *dev, unsigned int idx)
{
	struct mctrl_gpios *gpios;
	enum mctrl_gpio_idx i;

	gpios = devm_kzalloc(dev, sizeof(*gpios), GFP_KERNEL);
	if (!gpios)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < UART_GPIO_MAX; i++) {
		enum gpiod_flags flags;

		if (mctrl_gpios_desc[i].dir_out)
			flags = GPIOD_OUT_LOW;
		else
			flags = GPIOD_IN;

		gpios->gpio[i] =
			devm_gpiod_get_index_optional(dev,
						      mctrl_gpios_desc[i].name,
						      idx, flags);

		if (IS_ERR(gpios->gpio[i]))
			return ERR_CAST(gpios->gpio[i]);
	}

	return gpios;
}
EXPORT_SYMBOL_GPL(mctrl_gpio_init_noauto);

#define MCTRL_ANY_DELTA (TIOCM_RI | TIOCM_DSR | TIOCM_CD | TIOCM_CTS)
static irqreturn_t mctrl_gpio_irq_handle(int irq, void *context)
{
	struct mctrl_gpios *gpios = context;
	struct uart_port *port = gpios->port;
	u32 mctrl = gpios->mctrl_prev;
	u32 mctrl_diff;

	mctrl_gpio_get(gpios, &mctrl);

	mctrl_diff = mctrl ^ gpios->mctrl_prev;
	gpios->mctrl_prev = mctrl;

	if (mctrl_diff & MCTRL_ANY_DELTA && port->state != NULL) {
		if ((mctrl_diff & mctrl) & TIOCM_RI)
			port->icount.rng++;

		if ((mctrl_diff & mctrl) & TIOCM_DSR)
			port->icount.dsr++;

		if (mctrl_diff & TIOCM_CD)
			uart_handle_dcd_change(port, mctrl & TIOCM_CD);

		if (mctrl_diff & TIOCM_CTS)
			uart_handle_cts_change(port, mctrl & TIOCM_CTS);

		wake_up_interruptible(&port->state->port.delta_msr_wait);
	}

	return IRQ_HANDLED;
}

struct mctrl_gpios *mctrl_gpio_init(struct uart_port *port, unsigned int idx)
{
	struct mctrl_gpios *gpios;
	enum mctrl_gpio_idx i;

	gpios = mctrl_gpio_init_noauto(port->dev, idx);
	if (IS_ERR(gpios))
		return gpios;

	gpios->port = port;

	for (i = 0; i < UART_GPIO_MAX; ++i) {
		int ret;

		if (!gpios->gpio[i] || mctrl_gpios_desc[i].dir_out)
			continue;

		ret = gpiod_to_irq(gpios->gpio[i]);
		if (ret <= 0) {
			dev_err(port->dev,
				"failed to find corresponding irq for %s (idx=%d, err=%d)\n",
				mctrl_gpios_desc[i].name, idx, ret);
			return ERR_PTR(ret);
		}
		gpios->irq[i] = ret;

		/* irqs should only be enabled in .enable_ms */
		irq_set_status_flags(gpios->irq[i], IRQ_NOAUTOEN);

		ret = devm_request_irq(port->dev, gpios->irq[i],
				       mctrl_gpio_irq_handle,
				       IRQ_TYPE_EDGE_BOTH, dev_name(port->dev),
				       gpios);
		if (ret) {
			/* alternatively implement polling */
			dev_err(port->dev,
				"failed to request irq for %s (idx=%d, err=%d)\n",
				mctrl_gpios_desc[i].name, idx, ret);
			return ERR_PTR(ret);
		}
	}

	return gpios;
}

void mctrl_gpio_free(struct device *dev, struct mctrl_gpios *gpios)
{
	enum mctrl_gpio_idx i;

	for (i = 0; i < UART_GPIO_MAX; i++) {
		if (gpios->irq[i])
			devm_free_irq(gpios->port->dev, gpios->irq[i], gpios);

		if (gpios->gpio[i])
			devm_gpiod_put(dev, gpios->gpio[i]);
	}
	devm_kfree(dev, gpios);
}
EXPORT_SYMBOL_GPL(mctrl_gpio_free);

void mctrl_gpio_enable_ms(struct mctrl_gpios *gpios)
{
	enum mctrl_gpio_idx i;

	/* .enable_ms may be called multiple times */
	if (gpios->mctrl_on)
		return;

	gpios->mctrl_on = true;

	/* get initial status of modem lines GPIOs */
	mctrl_gpio_get(gpios, &gpios->mctrl_prev);

	for (i = 0; i < UART_GPIO_MAX; ++i) {
		if (!gpios->irq[i])
			continue;

		enable_irq(gpios->irq[i]);
	}
}
EXPORT_SYMBOL_GPL(mctrl_gpio_enable_ms);

void mctrl_gpio_disable_ms(struct mctrl_gpios *gpios)
{
	enum mctrl_gpio_idx i;

	if (!gpios->mctrl_on)
		return;

	gpios->mctrl_on = false;

	for (i = 0; i < UART_GPIO_MAX; ++i) {
		if (!gpios->irq[i])
			continue;

		disable_irq(gpios->irq[i]);
	}
}
