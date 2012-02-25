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
