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
 
#define EEEFSB_WORK_QUEUE_NAME "WQeeefsb.c"

static void intrpt_routine(struct work_struct *private_);

static int die = 0; /* set this to 1 for shutdown */
static int stepping_ready = 1;

/* The work queue structure for this task, from workqueue.h */
static struct workqueue_struct *eeefsb_workqueue;

static struct delayed_work eeefsb_task;
static DECLARE_DELAYED_WORK(eeefsb_task, intrpt_routine);

void eeefsb_wq_start(void)
{
    queue_delayed_work(eeefsb_workqueue, &eeefsb_task, EEEFSB_STEPDELAY);
}

/* 
 * This function will be called on every timer interrupt.
 */
static void intrpt_routine(struct work_struct *private_)
{
	/* 
	 * Do some tasks 
	 */
    
	/* If cleanup wants us to die */
	if (die == 0 && stepping_ready == 0)
		eeefsb_wq_start();
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
