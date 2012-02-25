/*
 *  eeefsb_wq.h - cpu freq stepping for eeefsb
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
 #include <linux/kernel.h>
#include <linux/module.h>

#ifndef _EEEFSB_WQ_H_
#define _EEEFSB_WQ_H_
/*void intrpt_routine(struct work_struct *private_);*/
void eeefsb_wq_start(int cpu_freq);
void eeefsb_wq_init(void);
void eeefsb_wq_cleanup(void);
#endif
