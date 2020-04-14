/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <mt-plat/aee.h>
#include <linux/interrupt.h>
#include <mt-plat/sync_write.h>
#include "scp_ipi_pin.h"
#include "scp_helper.h"
#include "scp_excep.h"
#include "scp_dvfs.h"


/*
 * handler for wdt irq for scp
 * dump scp register
 */
static void scp_A_wdt_handler(void)
{
	pr_debug("[SCP] CM4 A WDT exception\n");
	scp_A_dump_regs();
}

/*
 * dispatch scp irq
 * reset scp and generate exception if needed
 * @param irq:      irq id
 * @param dev_id:   should be NULL
 */
irqreturn_t scp_A_irq_handler(int irq, void *dev_id)
{
	unsigned int reg = readl(SCP_A_TO_HOST_REG);
#if 1//SCP_RECOVERY_SUPPORT
	/* if WDT and IPI triggered on the same time, ignore the IPI */
	if (reg & SCP_IRQ_WDT) {
		int retry;
		unsigned long tmp;

		scp_A_wdt_handler();
		if (scp_set_reset_status() == RESET_STATUS_STOP) {
			pr_debug("[SCP] CM4 WDT handler start to reset scp...\n");
			scp_send_reset_wq(RESET_TYPE_WDT);
		} else
			pr_notice("scp_A_wdt_handler: scp resetting\n");

		/* clr after SCP side INT trigger,
		 * or SCP may lost INT max wait 5000*40u = 200ms
		 */
		for (retry = SCP_AWAKE_TIMEOUT; retry > 0; retry--) {
			tmp = readl(SCP_GPR_CM4_A_REBOOT);
			if (tmp == CM4_A_READY_TO_REBOOT)
				break;
			udelay(40);
		}
		if (retry == 0)
			pr_debug("[SCP] SCP_A wakeup timeout\n");
		udelay(10);
		writel(SCP_IRQ_WDT, SCP_A_TO_HOST_REG);
	} else if (reg & SCP_IRQ_SCP2HOST) {
		/* if WDT and IPI triggered on the same time, ignore the IPI */
		//scp_A_ipi_handler();
		writel(SCP_IRQ_SCP2HOST, SCP_A_TO_HOST_REG);
	}
#endif  // SCP_RECOVERY_SUPPORT

	return IRQ_HANDLED;
}

/*
 * scp irq initialize
 */
void scp_A_irq_init(void)
{
	writel(SCP_IRQ_SCP2HOST, SCP_A_TO_HOST_REG); /* clear scp irq */
}
