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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <asm/io.h>         /* For inb() and outb() */

#define EC_IDX_ADDRH 0x381
#define EC_IDX_ADDRL 0x382
#define EC_IDX_DATA 0x383
#define HIGH_BYTE(x) ((x & 0xff00) >> 8)
#define LOW_BYTE(x) (x & 0x00ff)
static DEFINE_MUTEX(eeefsb_ec_mutex);

static unsigned char eeefsb_ec_read(unsigned short addr) {
    unsigned char data;

    mutex_lock(&eeefsb_ec_mutex);
    outb(HIGH_BYTE(addr), EC_IDX_ADDRH);
    outb(LOW_BYTE(addr), EC_IDX_ADDRL);
    data = inb(EC_IDX_DATA);
    mutex_unlock(&eeefsb_ec_mutex);

    return data;
}

static void eeefsb_ec_write(unsigned short addr, unsigned char data)
{
    mutex_lock(&eeefsb_ec_mutex);
    outb(HIGH_BYTE(addr), EC_IDX_ADDRH);
    outb(LOW_BYTE(addr), EC_IDX_ADDRL);
    outb(data, EC_IDX_DATA);
    mutex_unlock(&eeefsb_ec_mutex);
}

void eeefsb_ec_gpio_set(int pin, int value)
{
    unsigned short port;
    unsigned char mask;

    port = 0xFC20 + ((pin >> 3) & 0x1f);
    mask = 1 << (pin & 0x07);
    if (value) {
        eeefsb_ec_write(port, eeefsb_ec_read(port) | mask);
    } else {
        eeefsb_ec_write(port, eeefsb_ec_read(port) & ~mask);
    }
}

int eeefsb_ec_gpio_get(int pin)
{
    unsigned short port;
    unsigned char mask;
    unsigned char status;

    port = 0xfc20 + ((pin >> 3) & 0x1f);
    mask = 1 << (pin & 0x07);
    status = eeefsb_ec_read(port) & mask;

    return (status) ? 1 : 0;
}

/*** Voltage functions ********************************************************
 * ICS9LPR426A                                                                *
*/
#define EC_VOLTAGE_PIN 0x66
int eeefsb_get_voltage(void)
{
    return eeefsb_ec_gpio_get(EC_VOLTAGE_PIN);
}

void eeefsb_set_voltage(int voltage)
{
    if (voltage > 0)
    {
        voltage = 1;
    } else {
        voltage = 0;
    }
    eeefsb_ec_gpio_set(EC_VOLTAGE_PIN, voltage);
}

/*** Fan and temperature functions *******************************************
 * ENE KB3310                                                                */
#define EC_ST00 0xF451          /* Temperature of CPU (C)                    */
#define EC_SC02 0xF463          /* Fan PWM duty cycle (%)                    */
#define EC_SC05 0xF466          /* High byte of fan speed (RPM)              */
#define EC_SC06 0xF467          /* Low byte of fan speed (RPM)               */
#define EC_SFB3 0xF4D3          /* Flag byte containing SF25 (FANctrl)       */

unsigned int eeefsb_get_temperature(void)
{
    return eeefsb_ec_read(EC_ST00);
}

unsigned int eeefsb_fan_get_rpm(void)
{
    return (eeefsb_ec_read(EC_SC05) << 8) | eeefsb_ec_read(EC_SC06);
}

/* Get fan control mode status                                                *
 * Returns 1 if fan is in manual mode, 0 if controlled by the EC             */
int eeefsb_fan_get_manual(void)
{
    return (eeefsb_ec_read(EC_SFB3) & 0x02) ? 1 : 0;
}

void eeefsb_fan_set_control(int manual)
{
    if (manual) {
        /* SF25=1: Prevent the EC from controlling the fan. */
        eeefsb_ec_write(EC_SFB3, eeefsb_ec_read(EC_SFB3) | 0x02);
    } else {
        /* SF25=0: Allow the EC to control the fan. */
        eeefsb_ec_write(EC_SFB3, eeefsb_ec_read(EC_SFB3) & ~0x02);
    }
}

void eeefsb_fan_set_speed(unsigned int speed)
{
    eeefsb_ec_write(EC_SC02, (speed > 100) ? 100 : speed);
}

unsigned int eeefsb_fan_get_speed(void)
{
    return eeefsb_ec_read(EC_SC02);
}
