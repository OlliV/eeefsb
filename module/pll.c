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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include "pll.h"
#include "options.h"

/* Prototypes */
static void eeefsb_pll_read(void);
static void eeefsb_pll_write(void);

static struct i2c_client eeefsb_pll_smbus_client = {
    .adapter = NULL,
    .addr    = 0x69,
    .flags   = 0,
};

static char eeefsb_pll_data[I2C_SMBUS_BLOCK_MAX];
static int eeefsb_pll_datalen = 0;

static void eeefsb_pll_read(void)
{
    // Takes approx 150ms to execute.
    memset(eeefsb_pll_data, 0, I2C_SMBUS_BLOCK_MAX);
    eeefsb_pll_datalen = i2c_smbus_read_block_data(&eeefsb_pll_smbus_client, 0, eeefsb_pll_data);
}

static void eeefsb_pll_write(void)
{
    // Takes approx 150ms to execute ???
    i2c_smbus_write_block_data(&eeefsb_pll_smbus_client, 0, eeefsb_pll_datalen, eeefsb_pll_data);
}

/*** FSB functions ************************************************************
 * ICS9LPR426A                                                                *
 * cpuM and cpuN are CPU PLL VDO dividers                                     *
 * f_CPUVCO = 24 * N/M                                                        *
 * PCID is the PCI and PCI-E divisor                                          *
 * f_PCIVCO = 24 * N/M                                                        *
 */
void eeefsb_get_freq(int *cpuM, int *cpuN, int *PCID)
{
    eeefsb_pll_read();
    *cpuM = eeefsb_pll_data[11] & 0x3F;
    *cpuN = ((int)(eeefsb_pll_data[12] & 0xFF) << 2) | (((int)(eeefsb_pll_data[11]) & 0xC0) >> 6);
    *PCID = eeefsb_pll_data[15] & 0x3F; // Byte 15: PCI M
}

void eeefsb_set_freq(int cpuM, int cpuN, int PCID)
{
    int current_cpuM = 0, current_cpuN = 0, current_PCID = 0;
    eeefsb_get_freq(&current_cpuM, &current_cpuN, &current_PCID);
    if (current_cpuM != cpuM || current_cpuN != cpuN || current_PCID != PCID)
    {
        eeefsb_pll_data[11] = ((cpuM & 0x3F) | ((cpuN & 0x03) << 6)) & 0xFF;
        eeefsb_pll_data[12] = (cpuN >> 2) & 0xFF;
        eeefsb_pll_data[15] = PCID & 0x3F;
        eeefsb_pll_write();
    }
}

int eeefsb_get_cpu_freq()
{
    int cpuM = 0;
    int cpuN = 0;
    int PCID = 0;

    eeefsb_get_freq(&cpuM, &cpuN, &PCID);
    
    return (cpuN * EEEFSB_PLL_CONST_MUL * EEEFSB_CPU_MUL) / cpuM;
}

int eeefsb_pll_init(void)
{
    int i = 0;
    int found = 0;
    struct i2c_adapter *_adapter;
    
    while ((_adapter = i2c_get_adapter(i)) != NULL) {
        if (strstr(_adapter->name, "I801")) {
            printk("eeefsb: Found i2c adapter %s\n", _adapter->name);
            found = 1;
            break;
        }
        i++;
    }
    
    if (found)
        eeefsb_pll_smbus_client.adapter = _adapter;
    else {
        printk("eeefsb: No i801 adapter found.\n");
        return -1;
    }

    /* Fill the eeefsb_pll_data buffer. */
    eeefsb_pll_read();
    
    return 0;
}

void eeefsb_pll_cleanup(void)
{
    i2c_put_adapter(eeefsb_pll_smbus_client.adapter);
}
