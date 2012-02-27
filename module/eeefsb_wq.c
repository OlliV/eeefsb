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
    n_current  = cpuN;
    m_current  = cpuM;
    pci_target = PCID;
    
    if (cpu_freq <= 1775)
    {
        /* Set back to automatic fan control by EC */
        eeefsb_fan_set_control(0);
        
        m_target = 50; /* for frequencies <= 1774 MHz */
    } else { /* CPU clock over 1774 MHz was requested */
        /* This is mandatory ...but remember to not set your laptop on sleep  *
         * or your CPU will be toasted on start up                            *
         */
        eeefsb_fan_set_control(1);
        /* Calculate needed fan speed */
        eeefsb_fan_set_speed((unsigned int)(80 + (cpu_freq - 1782) / 2));
        
        m_target = 49; /* for frequencies above 1774 MHz */
    }
    
    /* Calculate new N */
    n_target = (cpu_freq * m_target) / (EEEFSB_PLL_CONST_MUL * EEEFSB_CPU_MUL);
    
    die = 0;
    queue_delayed_work(eeefsb_workqueue, &eeefsb_task, EEEFSB_STEPDELAY);
}

/* 
 * This function will be called on every timer interrupt.
 */
static void intrpt_routine(struct work_struct *private_)
{
    int min_fsb_n;
    int max_fsb_n;
    
    /* Update M now? */
    if ((n_target > n_current) && (m_target > m_current))
    {
        m_current = m_target;
        eeefsb_set_freq(m_current, n_current, pci_target);
        printk(KERN_DEBUG "eeefsb: (a) set m = %i", m_current);
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
    min_fsb_n = (m_current == 50) ? EEEFSB_MINFSBNL : EEEFSB_MINFSBNH;
    max_fsb_n = (m_target == 50 &&  m_target < m_current) ? EEEFSB_MAXFSBNL : EEEFSB_MAXFSBNH;
    if (n_current <= min_fsb_n)
    {
        n_current = min_fsb_n;
        die = 1;
    }
    else if (n_current >= max_fsb_n)
    {
        n_current = max_fsb_n;
        die = 1;
    }
        
    /* Is it safe to update M? */
    if (((n_current == n_target) || die || (n_current-1 == min_fsb_n)) && (m_target != m_current))
    {
        m_current = m_target;
        eeefsb_set_freq(m_current, n_current, pci_target);
        printk(KERN_DEBUG "eeefsb: (b) set m = %i", m_current);
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
