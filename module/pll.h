/******************************************************************************
 *  pll.c - PLL access functions                                              *
 *                                                                            *
 *  Note that this isn't really the "proper" way to use the I2C API... :)     *
 *  I2C_SMBUS_BLOCK_MAX is 32, the maximum size of a block read/write.        *
 ******************************************************************************
 *
 *  Copyright (C) 2007 Andrew Tipton
 *  Copyright (C) 2009 Andrew Wyatt
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
 #include <linux/i2c.h>
 
#ifndef _PLL_H_
#define _PLL_H_
void eeefsb_get_freq(int *cpuM, int *cpuN, int *PCID);
void eeefsb_set_freq(int cpuM, int cpuN, int PCID);
int eeefsb_pll_init(void);
void eeefsb_pll_cleanup(void);
#endif
