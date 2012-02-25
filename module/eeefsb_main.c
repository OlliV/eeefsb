/*
 *  eeefsb.c - eee PC CPU speed control tool
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

/* 
 * Standard in kernel modules 
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>      /* Necessary because we use the proc fs */
#include <asm/uaccess.h> 
#include "options.h"            /* FSB tuning options */
#include "ec.h"
#include "pll.h"
#include "eeefsb_wq.h"

/*
 * Module info
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Olli Vanhoja, Andrew Wyatt, Andrew Tipton");
MODULE_DESCRIPTION("eee PC CPU speed control tool.");
#define EEEFSB_VERSION "0.1"
MODULE_VERSION(EEEFSB_VERSION);
MODULE_INFO(module_depends, "i2c-i801");

/*** Voltage functions *******************************************************
 * ICS9LPR426A                                                               */

#define EC_VOLTAGE_PIN 0x66
enum eeefsb_voltage { Low=0, High=1 };
static enum eeefsb_voltage eeefsb_get_voltage(void)
{
    return eeefsb_ec_gpio_get(EC_VOLTAGE_PIN);
}

static void eeefsb_set_voltage(enum eeefsb_voltage voltage)
{
    eeefsb_ec_gpio_set(EC_VOLTAGE_PIN, voltage);
}

/*** /proc file functions *****************************************************
 * eeefsb proc files put under /proc/eeefsb:                                  *
 * bus_control =                                                              *
 * cpu_freq    =                                                              *
 * fan_speed   =                                                              *
 * fan_rpm     =                                                              *
 * fan_control =                                                              *
 */
static struct proc_dir_entry *eeefsb_proc_rootdir;
#define EEEFSB_PROC_READFUNC(NAME) \
    void eeefsb_proc_readfunc_##NAME (char *buf, int buflen, int *bufpos)
#define EEEFSB_PROC_WRITEFUNC(NAME) \
    void eeefsb_proc_writefunc_##NAME (const char *buf, int buflen, int *bufpos)
#define EEEFSB_PROC_PRINTF(FMT, ARGS...) \
    *bufpos += snprintf(buf + *bufpos, buflen - *bufpos, FMT, ##ARGS)
#define EEEFSB_PROC_SCANF(COUNT, FMT, ARGS...) \
    do { \
        int len = 0; \
        int cnt = sscanf(buf + *bufpos, FMT "%n", ##ARGS, &len); \
        if (cnt < COUNT) { \
            printk(KERN_DEBUG "eeefsb:  scanf(\"%s\") wanted %d args, but got %d.\n", FMT, COUNT, cnt); \
            return; \
        } \
        *bufpos += len; \
    } while (0)
#define EEEFSB_PROC_MEMCPY(SRC, SRCLEN) \
    do { \
        int len = SRCLEN; \
        if (len > (buflen - *bufpos)) \
            len = buflen - *bufpos; \
        memcpy(buf + *bufpos, SRC, (SRCLEN > (buflen - *bufpos)) ? (buflen - *bufpos) : SRCLEN); \
        *bufpos += len; \
    } while (0)
#define EEEFSB_PROC_FILES_BEGIN \
    static struct eeefsb_proc_file eeefsb_proc_files[] = {
#define EEEFSB_PROC_RW(NAME, MODE) \
    { #NAME, MODE, &eeefsb_proc_readfunc_##NAME, &eeefsb_proc_writefunc_##NAME }
#define EEEFSB_PROC_RO(NAME, MODE) \
    { #NAME, MODE, &eeefsb_proc_readfunc_##NAME, NULL }
#define EEEFSB_PROC_FILES_END \
    { NULL, 0, NULL, NULL } };

struct eeefsb_proc_file
{
    char *name;
    int mode;
    void (*readfunc)(char *buf, int buflen, int *bufpos);
    void (*writefunc)(const char *buf, int buflen, int *bufpos);
};

EEEFSB_PROC_READFUNC(bus_control)
{
    int cpuM = 0;
    int cpuN = 0;
    int PCID = 0;
    int voltage = 0;

    eeefsb_get_freq(&cpuM, &cpuN, &PCID);
    voltage = (int)eeefsb_get_voltage();
    EEEFSB_PROC_PRINTF("%d %d %d %d\n", cpuM, cpuN, PCID, voltage);
}

EEEFSB_PROC_WRITEFUNC(bus_control)
{
    /* Sensible defaults */
    int cpuM = 18;
    int cpuN = 420;
    int PCID = 15;
    int voltage = 1;

    EEEFSB_PROC_SCANF(4, "%i %i %i %i", &cpuM, &cpuN, &PCID, &voltage);
    eeefsb_set_freq(cpuM, cpuN, PCID);
    eeefsb_set_voltage(voltage);
}

EEEFSB_PROC_READFUNC(cpu_freq)
{
    int cpuFreq;
    cpuFreq = eeefsb_get_cpu_freq();
    
    EEEFSB_PROC_PRINTF("%d\n", cpuFreq);
}

EEEFSB_PROC_WRITEFUNC(cpu_freq)
{
    /* Sensible defaults */
    int cpuFreq = 1600;

    EEEFSB_PROC_SCANF(1, "%i", &cpuFreq);
    eeefsb_wq_start(cpuFreq);
}

/*EEEFSB_PROC_READFUNC(pll)
{
    eeefsb_pll_read();
    EEEFSB_PROC_MEMCPY(eeefsb_pll_data, eeefsb_pll_datalen);
}*/

EEEFSB_PROC_READFUNC(fan_speed)
{
    int speed = eeefsb_fan_get_speed();
    EEEFSB_PROC_PRINTF("%d\n", speed);
}

EEEFSB_PROC_WRITEFUNC(fan_speed)
{
    unsigned int speed = 0;
    EEEFSB_PROC_SCANF(1, "%u", &speed);
    eeefsb_fan_set_speed(speed);
}

EEEFSB_PROC_READFUNC(fan_rpm)
{
    int rpm = eeefsb_fan_get_rpm();
    EEEFSB_PROC_PRINTF("%d\n", rpm);
}

EEEFSB_PROC_READFUNC(fan_control)
{
    EEEFSB_PROC_PRINTF("%d\n", eeefsb_fan_get_manual());
}

EEEFSB_PROC_WRITEFUNC(fan_control)
{
    int manual = 0;
    EEEFSB_PROC_SCANF(1, "%i", &manual);
    eeefsb_fan_set_control(manual);
}

#if 0
9LPr426A_PROC_READFUNC(fan_mode)
{
    enum eeefsb_fan_mode mode = eeefsb_fan_get_mode();
    switch (mode) {
        case Manual:    EEEFSB_PROC_PRINTF("manual\n");
                        break;
        case Automatic: EEEFSB_PROC_PRINTF("auto\n");
                        break;
        case Embedded:  EEEFSB_PROC_PRINTF("embedded\n");
                        break;
    }
}

EEEFSB_PROC_WRITEFUNC(fan_mode)
{
    enum eeefsb_fan_mode mode = Automatic;
    char inputstr[16];
    EEEFSB_PROC_SCANF(1, "%15s", inputstr);
    if (strcmp(inputstr, "manual") == 0) {
        mode = Manual;
    } else if (strcmp(inputstr, "auto") == 0) {
        mode = Automatic;
    } else if (strcmp(inputstr, "embedded") == 0) {
        mode = Embedded;
    }
    eeefsb_fan_set_mode(mode);
}
#endif

EEEFSB_PROC_READFUNC(temperature)
{
    unsigned int t = eeefsb_get_temperature();
    EEEFSB_PROC_PRINTF("%d\n", t);
}

EEEFSB_PROC_FILES_BEGIN
    EEEFSB_PROC_RW(bus_control,    0644),
    /*EEEFSB_PROC_RO(pll,            0400),*/
    EEEFSB_PROC_RW(cpu_freq,       0644),
    EEEFSB_PROC_RW(fan_speed,      0644),
    EEEFSB_PROC_RO(fan_rpm,        0444),
    EEEFSB_PROC_RW(fan_control,    0644),
    EEEFSB_PROC_RO(temperature,    0444),
EEEFSB_PROC_FILES_END
    

int eeefsb_proc_readfunc(char *buffer, char **buffer_location, off_t offset,
                      int buffer_length, int *eof, void *data)
{
    struct eeefsb_proc_file *procfile = (struct eeefsb_proc_file *)data;
    int bufpos = 0;

    if (!procfile || !procfile->readfunc)
    {
        return -EIO;
    }

    *eof = 1;
    if (offset > 0)
    {
        return 0;
    }

    (*procfile->readfunc)(buffer, buffer_length, &bufpos);
    return bufpos;
}

int eeefsb_proc_writefunc(struct file *file, const char *buffer,
                       unsigned long count, void *data)
{
    char userdata[129];
    int bufpos = 0;
    struct eeefsb_proc_file *procfile = (struct eeefsb_proc_file *)data;

    if (!procfile || !procfile->writefunc) {
        return -EIO;
    }

    if (copy_from_user(userdata, buffer, (count > 128) ? 128 : count)) {
        printk(KERN_DEBUG "eeefsb: copy_from_user() failed\n");
        return -EIO;
    }
    userdata[128] = 0; /* So that sscanf() doesn't overflow... */

    (*procfile->writefunc)(userdata, count, &bufpos);
    return count;
}

int eeefsb_proc_init(void)
{
    int i;

    /* Create the /proc/eeefsb directory. */
    eeefsb_proc_rootdir = proc_mkdir("eeefsb", NULL);
    if (!eeefsb_proc_rootdir)
    {
        printk(KERN_ERR "eeefsb: Unable to create /proc/eeefsb\n");
        return false;
    }

    /* Create the individual proc files. */
    for (i=0; eeefsb_proc_files[i].name; i++)
    {
        struct proc_dir_entry *proc_file;
        struct eeefsb_proc_file *f = &eeefsb_proc_files[i];

        proc_file = create_proc_entry(f->name, f->mode, eeefsb_proc_rootdir);
        if (!proc_file) {
            printk(KERN_ERR "eeefsb: Unable to create /proc/eeefsb/%s", f->name);
            goto proc_init_cleanup;
        }
        proc_file->read_proc = &eeefsb_proc_readfunc;
        if (f->writefunc) {
            proc_file->write_proc = &eeefsb_proc_writefunc;
        }
        proc_file->data = f;
        proc_file->mode = S_IFREG | f->mode;
        proc_file->uid = 0;
        proc_file->gid = 0;
    }
    return true;

    /* We had an error, so cleanup all of the proc files... */
    proc_init_cleanup:
    for (; i >= 0; i--)
    {
        remove_proc_entry(eeefsb_proc_files[i].name, eeefsb_proc_rootdir);
    }
    remove_proc_entry("eeefsb", NULL);
    return false;
}

void eeefsb_proc_cleanup(void)
{
    int i;
    for (i = 0; eeefsb_proc_files[i].name; i++)
    {
        remove_proc_entry(eeefsb_proc_files[i].name, eeefsb_proc_rootdir);
    }
    remove_proc_entry("eeefsb", NULL);
}

/*** Module initialization and cleanup ****************************************
*/
static int __init eeefsb_init(void)
{
    int retVal;
    
    retVal = eeefsb_pll_init();
    if (retVal) return retVal;
    eeefsb_proc_init();
    eeefsb_wq_init();
    printk(KERN_NOTICE "eee PC CPU speed control tool, version %s\n",
           EEEFSB_VERSION);
    
    return 0;
}

static void __exit eeefsb_exit(void)
{
    eeefsb_pll_cleanup();
    eeefsb_proc_cleanup();
    eeefsb_wq_cleanup();
    printk(KERN_INFO "/proc/eeefsb removed\n");
}

module_init(eeefsb_init);
module_exit(eeefsb_exit);
