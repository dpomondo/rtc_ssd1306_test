#ifndef _rtc_ssd1306_h
#define _rtc_ssd1306_h

#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/rtc.h"
#include "pico/cyw43_arch.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "libraries/ntp_request/ntp_request.h"
#include "libraries/pico-ssd1306/ssd1306.h"
// #include "ntp_request.h"
// #include "ssd1306.h"
#include "libraries/pico-ssd1306/example/acme_5_outlines_font.h"
#include <stdint.h>

#include "include/credentials.h"

const char * weekdays[7] =
{       "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday"
};

const char * months[13] = 
{   "Jan", 
    "Feb", 
    "Mar", 
    "Apr", 
    "May", 
    "Jun", 
    "Jul", 
    "Aug", 
    "Sep", 
    "Oct", 
    "Nov", 
    "Dec"
};

struct temperature_struct
{
    int32_t raw_temperature;
    int32_t raw_pressure;
    int32_t temperature; 
    int32_t pressure; 
    int32_t high_temperature;
    int32_t high_temp_time;
    int32_t low_temperature;
    int32_t low_temp_time;
    int32_t low_5_min_temp;
    int32_t low_5_min_time;
    int32_t high_5_min_temp;
    int32_t high_5_min_time;

};

// I2C defines
#define I2C_PORT i2c1
#define I2C_SDA 6       // GPIO6 physical pin 9
#define I2C_SCL 7       // GPIO7 physical pin 10

// SPI defines
#define SPI_PORT                    spi1
#define EEPROM_CS_PIN               9       // GPIO9  physical pin 12
#define EEPROM_CLK_PIN              10      // GPIO10 physical pin 14
#define EEPROM_MOSI_PIN             11      // GPIO11 physical pin 15
#define EEPROM_MISO_PIN             12      // GPIO12 physical pin 16

#define EEPROM_BYTES_PER_PAGE       32

#define EEPROM_LOOPS_ADDRESS        0x000
#define EEPROM_HIGH_TEMP_ADDRESS    0x004
#define EEPROM_HIGH_T_TIME          0x008
#define EEPROM_LOW_TEMP_ADDRESS     0x00c
#define EEPROM_LOW_T_TIME           0x010
#define LOOPS_TWO_ADDRESS           0x014 
#define LOOPS_THREE_ADDRESS         0x018
#define CURRENT_EEPROM_ADDRESS      0x01c
#define EEPROM_DATA_START           0x020

void setup_i2c(void);
void setup_spi(void);
int8_t start_wireless(void);
void rtc_UART_alarm_callback(void);
int64_t ssd1306_rtc_generic_callback(alarm_id_t id, void *arg);
int64_t bmp_take_readings_callback(alarm_id_t id, void *arg);
/* int64_t bmp_take_readings_callback(alarm_id_t id); */

#endif
