/*
 *  ec.c - eee PC CPU speed control tool Embedded controller access functions.*
 *                                                                            *
 * The ENE KB3310 embedded controller has a feature known as "Index IO"       *
 * which allows the entire 64KB address space of the controller to be         *
 * accessed via a set of ISA I/O ports at 0x380-0x384.  This allows us        *
 * direct access to all of the controller's ROM, RAM, SFRs, and peripheral    *
 * registers;  this access bypasses the EC firmware entirely.                 *
 *                                                                            *
 * This is much faster than using ec_transaction(), and it also allows us to  *
 * do things which are not possible through the EC's official interface.      *
 *                                                                            *
 * An Indexed IO write to an EC register takes approx. 90us, while an EC      *
 * transaction takes approx. 2500ms.                                          *
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
#ifndef _EC_H_
#define _EC_H_
/*unsigned char eeefsb_ec_read(unsigned short addr);
void eeefsb_ec_write(unsigned short addr, unsigned char data); */
void eeefsb_ec_gpio_set(int pin, int value);
int eeefsb_ec_gpio_get(int pin);
int eeefsb_get_voltage(void);
void eeefsb_set_voltage(int voltage);
int eeefsb_fan_get_manual(void);
unsigned int eeefsb_get_temperature(void);
unsigned int eeefsb_fan_get_rpm(void);
void eeefsb_fan_set_control(int manual);
void eeefsb_fan_set_speed(unsigned int speed);
unsigned int eeefsb_fan_get_speed(void);
#endif
