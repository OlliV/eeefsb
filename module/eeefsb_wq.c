/*
 *  eeefsb_wq.c - cpu freq stepping for eeefsb
 *
 *  Copyright (C) 2012 Olli Vanhoja
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Ths program is distributed in the hope that it will be useful,
 *  but WITOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTAILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Template Place, Suite 330, Boston, MA  02111-1307 USA
 *  
 *  ---------
 *
 *  This code comes WITHOUT ANY WARRANTY whatsoever.
 */

/*** Work queue for freq stepping *********************************************
 * Usage:                                                                     *
 * Put the task in the work_timer task queue, so it will be executed at       *
 * next timer interrupt:                                                      *
 * queue_delayed_work(eeefsb_workqueue, &eeefsb_task, EEEFSB_STEPDELAY);      *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>        /* We need to put ourselves to sleep
                                   and wake up later */
#include <linux/init.h>         /* For __init and __exit */
#include <linux/interrupt.h>	/* For irqreturn_t */
#include "options.h"
#include "pll.h"
#include "ec.h"
 
#define EEEFSB_WORK_QUEUE_NAME "WQeeefsb.c"

static void intrpt_routine(struct work_struct *private_);

static int die = 0; /* set this to 1 for shutdown */
static int n_target    = EEEFSB_CPU_N_SAFE;
static int n_current   = EEEFSB_CPU_N_SAFE;
static int m_target    = EEEFSB_CPU_M_SAFE;
static int m_current   = EEEFSB_CPU_M_SAFE;
static int pci_target  = EEEFSB_PCI_SAFE;
static int fan_ctrl_bfr_set_m = 0; /* Fan control state before setting M     */
static unsigned int fan_speed_bfr_set_m = 0; /* Fan speed before forcing to
                                                manual control by eeefsb     */

/* The work queue structure for this task, from workqueue.h */
static struct workqueue_struct *eeefsb_workqueue;

static struct delayed_work eeefsb_task;
static DECLARE_DELAYED_WORK(eeefsb_task, intrpt_routine);

void eeefsb_wq_start(int cpu_freq)
{
    int cpuM = 0;
    int cpuN = 0;
    int PCID = 0;

    eeefsb_get_freq(&cpuM, &cpuN, &PCID);
    n_current = cpuN;
    m_current = cpuM;
    pci_target = PCID;
    
    if (cpu_freq < 1774)
    {
        /* Check if we have previously forced the EC's fan control to manual  *
         * control mode.                                                      *
         * TODO This function will need some rework if we'd like to implement *
         * our own optimum fan control in the future                          *
         *                                                                    *
         * I think we don't need to set manual control back again if user     *
         * so lets just check the current state and decide the correct        *
         * action then.                                                       *
         */
        if (fan_speed_bfr_set_m && eeefsb_fan_get_manual())
        {
            eeefsb_fan_set_speed(fan_speed_bfr_set_m);
        }
        /* Calculate new parameters */
        n_target = (cpu_freq * cpuM) / (EEEFSB_PLL_CONST_MUL * EEEFSB_CPU_MUL);
        m_target = 50; /* bad bad bad, hard codded values */
    } else { /* This will lock the maximum speed to 1782 MHz */
        n_target = 455;
        m_target = 49;
        /* This is mandatory ...but remember to not set your laptop on sleep  *
         * or your CPU will be toasted on start up                            *
         */
        fan_ctrl_bfr_set_m = eeefsb_fan_get_manual(); /* Remember current fan *
        *                                                ctrl state           *
        * and fan speed too. If user will now revert back to auto... well     *
        * that's bad luck, since one should know that overclocking and        *
        * standard cooling doesn't usually work very well.                    *
        */
        if (fan_ctrl_bfr_set_m)
        {
            fan_speed_bfr_set_m = eeefsb_fan_get_speed();
        }
        eeefsb_fan_set_control(1);
        eeefsb_fan_set_speed(70);
    }
    
    die = 0;
    queue_delayed_work(eeefsb_workqueue, &eeefsb_task, EEEFSB_STEPDELAY);
}

/* 
 * This function will be called on every timer interrupt.
 */
static void intrpt_routine(struct work_struct *private_)
{
    /* Update M NOW if we are coming down from higher speed(s) *
     * where M has beed altered                                */
    if ((n_target > n_current) && (m_target > m_current))
    {
        m_current = m_target;
        eeefsb_set_freq(m_current, n_current, pci_target);
    }

    // Increment or decrement N */
    if (n_target > n_current)
    {
        /* Target frequency is higher than current CPU frequency */
        if ((n_current + EEEFSB_MULSTEP) > n_target)
        {
            n_current = n_target;
        }
        else
        {
            n_current += EEEFSB_MULSTEP;
        }
        eeefsb_set_freq(m_target, n_current, pci_target);
    }
    else if (n_target < n_current) 
    {
        /* Target frequency is lower than current CPU frequency */
        if ((n_current - EEEFSB_MULSTEP) < n_target)
        {
            n_current = n_target;
        }
        else
        {
            n_current -= EEEFSB_MULSTEP;
        }
        eeefsb_set_freq(m_target, n_current, pci_target);
    }
    
    /* Check N min & max */
    if (n_current <= EEEFSB_MINFSBN)
    {
        n_current = EEEFSB_MINFSBN;
        die = 1;
    }
    else if (n_current >= EEEFSB_MAXFSBN)
    {
        n_current = EEEFSB_MAXFSBN;
        die = 1;
    }
    
    /* Is it safe to update M? */
    if ((n_current == n_target || die) && (m_target != m_current))
    {
        /* This part applies only when we are going up to higher speed(s)    */
        m_current = m_target;
        eeefsb_set_freq(m_current, n_current, pci_target);
    }
    
    /* Set high cpu core voltage if needed */
    if (((n_current * EEEFSB_PLL_CONST_MUL * EEEFSB_CPU_MUL) / m_target) >= EEEFSB_HIVOLTFREQ)
    {
        eeefsb_set_voltage(1);
    } else {
        eeefsb_set_voltage(0);
    }
    
    printk(KERN_DEBUG "eeefsb: set n = %i, target = %i\n", n_current, n_target);
    
	/* If cleanup wants us to die */
	if (die == 0 && n_current != n_target)
		queue_delayed_work(eeefsb_workqueue, &eeefsb_task, EEEFSB_STEPDELAY);
}

/*
 * Initializer for work queue
 */
void eeefsb_wq_init(void)
{
    eeefsb_workqueue = create_workqueue(EEEFSB_WORK_QUEUE_NAME);
}

/*
 * Cleanup for work queue
 */
void eeefsb_wq_cleanup(void)
{
    die = 1;                     /* keep intrp_routine from queueing itself */
	cancel_delayed_work(&eeefsb_task); /* no "new ones"                     */
	flush_workqueue(eeefsb_workqueue); /* wait till all "old ones" finished */
	destroy_workqueue(eeefsb_workqueue);
    
    /* 
	 * Sleep until intrpt_routine is called one last time. This is 
	 * necessary, because otherwise we'll deallocate the memory holding 
	 * intrpt_routine and Task while work_timer still references them.
	 * Notice that here we don't allow signals to interrupt us.
	 *
	 * Since WaitQ is now not NULL, this automatically tells the interrupt
	 * routine it's time to die.
	 */
}
