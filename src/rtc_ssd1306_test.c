#include "hardware/uart.h"
#include "pico/cyw43_arch.h"
#include "pico/platform.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/datetime.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

/* #include "lwip/dns.h" */
/* #include "lwip/pbuf.h" */
/* #include "lwip/udp.h" */

#include "hardware/i2c.h"
#include "hardware/rtc.h"
/* #include "hardware/watchdog.h" */
#include "25LC320A.h"
#include "bmp280_i2c.h"
#include "ntp_request.h"
#include "rtc_ssd1306_test.h"
#include "ssd1306.h"
#include "wireless.h"

struct bmp280_calib_param params;
struct temperature_struct bmp_readings;
eeprom_t eeprom;
enum print_flag_types { PRINT_OFF, PRINT_ON, PRINT_CSV };
uint8_t print_flag = PRINT_ON;

int make_current_timestamp(void) {
  datetime_t t;
  rtc_get_datetime(&t);
  return approx_epoch(&t);
}

struct tm *tm_from_timestamp(int timestamp) {
  time_t temp_time_t =
      (time_t)(timestamp - MOUNTAIN_STANDARD_OFFSET + DAYLIGHT_SAVINGS_OFFSET);
  return localtime(&temp_time_t);
}

int main() {
  stdio_init_all();
  puts(
      "\n\t*************++****\n\t*  Hello, world!  *\n\t******************* ");

  setup_i2c();
  ssd1306_t display;
  display.external_vcc = false;
  ssd1306_init(&display, 128, 32, 0x3C, I2C_PORT);
  ssd1306_clear(&display);

  setup_spi();

  // bmp280
  ssd1306_draw_string(&display, 0, 0, 1, "Set up BMP280");
  ssd1306_show(&display);
  bmp280_init();
  sleep_ms(500);
  bmp280_get_calib_params(&params);

  bmp280_read_raw(&bmp_readings.raw_temperature, &bmp_readings.raw_pressure);
  bmp_readings.temperature =
      bmp280_convert_temp(bmp_readings.raw_temperature, &params);
  bmp_readings.pressure = bmp280_convert_pressure(
      bmp_readings.raw_pressure, bmp_readings.raw_temperature, &params);

  // start the wirelss
  ssd1306_clear(&display);
  ssd1306_draw_string(&display, 0, 0, 1, "Starting wireless...");
  ssd1306_show(&display);
  int8_t wir_err = start_wireless();
  // and here we do something with wir_err?

  // get the time from the internet
  ssd1306_clear(&display);
  ssd1306_draw_string(&display, 0, 0, 1, "Making NTP request...");
  ssd1306_show(&display);
  int time_err = set_rtc_using_ntp_request_blocking();
  if (time_err != EXIT_SUCCESS) {
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "NTP request went south");
    ssd1306_show(&display);
    printf("NTP request returned but something bad happened\r\n");
    return -1; // something? happened
  }
  if (time_err == EXIT_SUCCESS) {
    printf("ntp request went well, I think!?!\n");
  }
  if (!rtc_running()) {
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "RTC not running");
    ssd1306_show(&display);
    printf("RTC should be runing but isn't\r\n");
    return -1;
  }
  printf("rtc running: %s", rtc_running() ? "true" : "false");

  // start the clock, time & info drawn every second by alarm callback
  add_alarm_in_ms(400, ssd1306_rtc_generic_callback, &display, true);
  // read from the BMP every seconf
  add_alarm_in_ms(1000, bmp_take_readings_callback, &bmp_readings, true);
  // write current bmp readings to the eeprom; the next write depends on the
  // previous Write time; the setup function returns next write time so we kill
  // 2 birds with one call
  add_alarm_in_ms(setup_eeprom_with_callback_time(&display),
                  bmp_reading_to_eeprom, &display, true);

  uint8_t c; // character to read from uart
  while (1)  {  // FOREVER
    sleep_ms(500);
    static uint8_t _menu_state = 0;
    while (uart_is_readable(uart0)) {
      uart_read_blocking(uart0, &c, 1);
    }
    if (c) {
      if (print_flag == PRINT_ON) {
        printf("\n\treceived from uart0: %d [%c]\n", c, (char)c);
      }
      switch (c) {
      case 'i' + 0:
      case 'I' + 0:
        if (print_flag == PRINT_ON) {
          printf("high: %d\tlow:%d\t", bmp_readings.high_temperature,
                 bmp_readings.low_temperature);
        }
        break;
      case 'd' + 0:
      case 'D' + 0:
        // if (print_flag == PRINT_CSV) {
        //   read_eeprom_as_CSV(&eeprom);
        // } else 
        if (print_flag == PRINT_ON) {
          printf("****** here's the eeprom! *****\n");
          print_flag = PRINT_OFF;
          eeprom_dump_all(&eeprom);
          print_flag = PRINT_ON;
        }
        break;
      case 'p' + 0:
      case 'P' + 0:
        if (print_flag == PRINT_CSV) {
          // do not need to toggle print_flag since nothing extraneous will print anyway
          read_eeprom_as_CSV(&eeprom);
        } else {
          print_flag = PRINT_OFF;
          read_convert_eeprom(&eeprom);
          print_flag = PRINT_ON;
        }
        break;
      case 'r' + 0:
      case 'R' + 0:
        printf("Reset \t[1] high temperature\n");
        printf("\t\t[2] low temperatures\n"); 
        printf("\t\t[3] nothing?\n");
        _menu_state = 1;
        print_flag = PRINT_OFF;
        break;
      case '1' + 0:
        if (_menu_state == 1) {
          print_flag = PRINT_ON;
          printf("reset high ");
          bmp_eeprom_set_high_temp(eeprom);
        }
        _menu_state = 0;
        print_flag = PRINT_ON;
        break;
      case '2' + 0:
        if (_menu_state == 1) {
          print_flag = PRINT_ON;
          printf("reset low ");
          bmp_eeprom_set_low_temp(eeprom);
        }
        _menu_state = 0;
        print_flag = PRINT_ON;
        break;
      case '3' + 0:
        printf("do nothing ");
        _menu_state = 0;
        print_flag = PRINT_ON;
        break;
      case 'h' + 0:
      case 'H' + 0:
        if (print_flag != PRINT_OFF) {
          printf("[I]nfo on high and low temps\n");
          printf("[D]ump eeprom contents\n");
          printf("[P]rint eeprom contents in human-readable form\n");
          printf("[R]eset high and low temperatures\n");
          printf("[V] Change print format to CSV\n");
          printf("[X] Turn on normal printing\n");
          printf("[T]oggle normal printing on or off\n");
          }
        // print_flag = PRINT_OFF;
        break;
      case 'V' + 0:
      case 'v' + 0:
        if (print_flag != PRINT_CSV) {
          printf("change print mode to CSV\n");
        }
        print_flag = PRINT_CSV;
        break;
      case 'X' + 0:
      case 'x' + 0:
        if (print_flag != PRINT_ON) {
          printf("change print mode to ON\n");
        }
        print_flag = PRINT_ON;
        break;
      case 't' + 0:
      case 'T' + 0:
        if (print_flag == PRINT_ON) {
            print_flag = PRINT_OFF;
          } else if (print_flag == PRINT_OFF) {
            print_flag = PRINT_ON;
            printf("change print mode to ON\n");
          }
        break;
      case 0:
      default:
        break;
      } // end switch
      c = 0;
    }

  } // END FOREVER
  // begin shutdown...this should never get reached
  ssd1306_deinit(&display);
  cyw43_arch_deinit();

  return EXIT_SUCCESS;
} // END main

/* I2C Initialisation. Using it at 400Khz.
 * */
void setup_i2c(void) {
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
}

/* SPI Initialisation
 * */
void setup_spi(void) {
  spi_init(SPI_PORT, 1000 * 1000);
  spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(EEPROM_CLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(EEPROM_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(EEPROM_MISO_PIN, GPIO_FUNC_SPI);
}

/* start the wireless stuffs
 * */
// int8_t start_wireless(void) {
//   printf("Trying to connect to %s with password %s\r\n", WIFI_SSID,
//          WIFI_PASSWORD);
//   if (cyw43_arch_init()) {
//     printf("failed to initialise\n");
//   } else {
//     printf("Cyw43_arch initialized\r\n");
//   } // end if...else
//
//   cyw43_arch_enable_sta_mode();
//
//   while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
//                                             CYW43_AUTH_WPA2_AES_PSK, 10000)) {
//     printf("failed to connect\n");
//     sleep_ms(1000);
//   }
//   return EXIT_SUCCESS;
// } // end start_wireless

void read_convert_eeprom(eeprom_t *eeprom_ptr) {
  int32_t data;
  uint32_t timestamp;
  struct tm *temp_time;

  printf("\n*********************** converting eeprom contents\n");
  uint16_t start_address = eeprom_ptr->current_address;
  for (uint16_t index; index < ((4096 - EEPROM_DATA_START) / 8); index++) {
    if ((index % 3) == 0) {
      printf("\n");
    }
    if (start_address > EEPROM_LAST_ADDRESS) {
      start_address = EEPROM_DATA_START;
    }
    data = eeprom_get_four_bytes(eeprom_ptr, start_address);
    start_address += 4;
    timestamp = eeprom_get_four_bytes(eeprom_ptr, start_address);
    start_address += 4;

    temp_time = tm_from_timestamp(timestamp);
    printf("%d epoch: %d ", data, timestamp);
    printf("%2d:%02d, %s %d %d | ", temp_time->tm_hour, temp_time->tm_min,
           months[temp_time->tm_mon], temp_time->tm_mday,
           temp_time->tm_year + 1900);
  }
  printf("\n*********************** done converting eeprom contents\n\n");
}

void read_eeprom_as_CSV(eeprom_t *eeprom_ptr) {
  int32_t data;
  uint32_t timestamp;
  struct tm *temp_time;

  printf("CSV START\n");
  uint16_t start_address = eeprom_ptr->current_address;
  for (uint16_t index; index < ((4096 - EEPROM_DATA_START) / 8); index++) {
    data = eeprom_get_four_bytes(eeprom_ptr, start_address);
    start_address += 4;
    timestamp = eeprom_get_four_bytes(eeprom_ptr, start_address);
    start_address += 4;
    // data & timestamps are in 8 byte chunks, so unless things get really
    // destabilized somehow we should never go past the final address until
    // after both data & timestamp reads
    if (start_address > EEPROM_LAST_ADDRESS) {
      start_address = EEPROM_DATA_START;
    }

    temp_time = tm_from_timestamp(timestamp);
    printf("%s/%d/%d %2d:%02d:%02d,%.2f\n", months[temp_time->tm_mon],
           temp_time->tm_mday, temp_time->tm_year + 1900, temp_time->tm_hour,
           temp_time->tm_min, temp_time->tm_sec, ((float)data / 100));
  }
  printf("CSV END\n");
}

/* This is a zombie function
 * */
void rtc_UART_alarm_callback(void) {
  static uint loops = 0;
  datetime_t t = {0};
  char datetime_buf[256];
  char *datetime_str = &datetime_buf[0];
  rtc_get_datetime(&t);

  int appr = approx_epoch(&t);
  datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
  if (print_flag == PRINT_ON) {
    printf("\n*****\n--> %s\t epoch: %d %d bytes loops:%d\n", datetime_str,
           appr, sizeof(appr), ++loops);
  }
} // end rtc_UART_alarm_callback

void bmp_eeprom_set_high_temp(eeprom_t eeprom) {
  int now = make_current_timestamp();

  bmp_readings.high_temperature = bmp_readings.temperature;
  bmp_readings.high_temp_time = now;

  uint32_t temp_temp = eeprom_get_four_bytes(&eeprom, EEPROM_HIGH_TEMP_ADDRESS);
  uint32_t temp_time = eeprom_get_four_bytes(&eeprom, EEPROM_HIGH_T_TIME);
  eeprom_send_four_bytes(&eeprom, EEPROM_HIGH_TEMP_ADDRESS,
                         bmp_readings.high_temperature);
  eeprom_send_four_bytes(&eeprom, EEPROM_HIGH_T_TIME,
                         bmp_readings.high_temp_time);

  if (print_flag == PRINT_ON) {
    printf("\n\told high temp: %d\told time: %d\n\t"
           "new high temp: %d\tnew time: %d\n",
           temp_temp, temp_time, bmp_readings.high_temperature,
           bmp_readings.high_temp_time);
  }
}

void bmp_eeprom_set_low_temp(eeprom_t eeprom) {
  // datetime_t t = {0};
  // rtc_get_datetime(&t);
  // int now = approx_epoch(&t);
  int now = make_current_timestamp();

  bmp_readings.low_temperature = bmp_readings.temperature;
  bmp_readings.low_temp_time = now;

  uint32_t temp_temp = eeprom_get_four_bytes(&eeprom, EEPROM_LOW_TEMP_ADDRESS);
  uint32_t temp_time = eeprom_get_four_bytes(&eeprom, EEPROM_LOW_T_TIME);
  eeprom_send_four_bytes(&eeprom, EEPROM_LOW_TEMP_ADDRESS,
                         bmp_readings.low_temperature);
  eeprom_send_four_bytes(&eeprom, EEPROM_LOW_T_TIME,
                         bmp_readings.low_temp_time);

  if (print_flag == PRINT_ON) {
    printf("\n\told low temp: %d\told time: %d\n\t"
           "new low temp: %d\tnew time: %d\n",
           temp_temp, temp_time, bmp_readings.low_temperature,
           bmp_readings.low_temp_time);
  } // end if
}

/* sample the BMP once per second, update current, high & low and write them to
 * EEPROM if necessary
 * */
int64_t bmp_take_readings_callback(alarm_id_t id, void *arg) {
  int now = make_current_timestamp();

  bmp280_read_raw(&bmp_readings.raw_temperature, &bmp_readings.raw_pressure);
  bmp_readings.temperature =
      bmp280_convert_temp(bmp_readings.raw_temperature, &params);
  bmp_readings.pressure = bmp280_convert_pressure(
      bmp_readings.raw_pressure, bmp_readings.raw_temperature, &params);

  if (bmp_readings.temperature > bmp_readings.high_temperature) {
    bmp_eeprom_set_high_temp(eeprom);
  }

  if (bmp_readings.temperature < bmp_readings.low_temperature) {
    bmp_eeprom_set_low_temp(eeprom);
  }
  // return value in us
  return 1000 * 1000; // one thousand microseconds is a millisecond, and one
                      // thousand miliseconds is one second
}

/* Write current bmp temp reading to EEPROM, every 15 minutes
 * */
int64_t bmp_reading_to_eeprom(alarm_id_t id, void *usr_data) {
  int now = make_current_timestamp();
  if (print_flag == PRINT_ON) {
    printf("current temp to EEPROM: %d to address %X\n",
           bmp_readings.temperature, eeprom.current_address);
  }
  eeprom_send_four_bytes(&eeprom, eeprom.current_address,
                         bmp_readings.temperature);
  eeprom.current_address += 4;
  if ((eeprom.current_address > EEPROM_LAST_ADDRESS) |
      (eeprom.current_address < EEPROM_DATA_START)) {
    eeprom.current_address =
        EEPROM_DATA_START; // wrap around, but not to 0x000!
  }

  eeprom_send_four_bytes(&eeprom, eeprom.current_address, now);
  eeprom.current_address += 4;

  if ((eeprom.current_address > EEPROM_LAST_ADDRESS) |
      (eeprom.current_address < EEPROM_DATA_START)) {
    eeprom.current_address =
        EEPROM_DATA_START; // wrap around, but not to 0x000!
  }
  eeprom_send_four_bytes(&eeprom, CURRENT_EEPROM_ADDRESS,
                         eeprom.current_address);
  if (print_flag == PRINT_ON) {
    printf("\tnew address: %X\n", eeprom.current_address);
  }
  // return value in us
  return 1000 * 1000 * 60 * 15; // every 900 million microseconds
}

/* update the ssd1306 screen once per second
 * */
int64_t ssd1306_rtc_generic_callback(alarm_id_t id, void *arg) {
  static uint8_t _state = 0;
  static uint8_t _count = 0;
  static uint8_t _state_swap = 4;

  ssd1306_t *_disp = (ssd1306_t *)arg;
  uint8_t _scale = 2;
  char buf[60];
  char uart_buf[60];
  datetime_t t = {0};
  rtc_get_datetime(&t);

  struct tm *temp_time;
  time_t temp_time_t;
  sprintf(buf, "%02d:%02d:%02d", t.hour, t.min, t.sec);
  /* udp_send_message(buf); */
  ssd1306_clear(_disp);
  ssd1306_draw_string(_disp, 1, 0, _scale, buf);

  sprintf(uart_buf, ""); // no more ghost characters
  switch (_state) {
  case 0:
    sprintf(buf, "%3s %2d %d", months[t.month - 1], t.day, t.year);
    if (_count >= _state_swap) {
      _state++;
      _count = 0;
    }
    break;
  case 1:
    sprintf(buf, "%s", weekdays[t.dotw]);
    if (_count >= _state_swap) {
      _state++;
      _count = 0;
    }
    break;
  case 2:
    sprintf(buf, "%.2fC", bmp_readings.temperature / 100.f);
    sprintf(uart_buf, "temp reading: %s", buf);
    if (_count >= _state_swap) {
      _state++;
      _count = 0;
    }
    break;
  case 3:
    sprintf(buf, "low:  %.2fC", bmp_readings.low_temperature / 100.f);
    // temp_time_t = (time_t)(bmp_readings.low_temp_time -
    //                        MOUNTAIN_STANDARD_OFFSET +
    //                        DAYLIGHT_SAVINGS_OFFSET);
    // temp_time = localtime(&temp_time_t);
    temp_time = tm_from_timestamp(bmp_readings.low_temp_time);
    sprintf(uart_buf, "%s\tfrom %d seconds ago, at %2d:%02d, %s %d %d", buf,
            (approx_epoch(&t) - bmp_readings.low_temp_time), temp_time->tm_hour,
            temp_time->tm_min, months[temp_time->tm_mon], temp_time->tm_mday,
            temp_time->tm_year + 1900);
    _scale = 1;
    if (_count >= _state_swap) {
      _state++;
      _count = 0;
    }
    break;
  case 4:
    sprintf(buf, "high: %.2fC", bmp_readings.high_temperature / 100.f);
    // temp_time_t = (time_t)(bmp_readings.high_temp_time -
    //                        MOUNTAIN_STANDARD_OFFSET +
    //                        DAYLIGHT_SAVINGS_OFFSET);
    // temp_time = localtime(&temp_time_t);
    temp_time = tm_from_timestamp(bmp_readings.high_temp_time);
    sprintf(uart_buf, "%s\tfrom %d seconds ago, at %2d:%02d, %s %d %d", buf,
            (approx_epoch(&t) - bmp_readings.high_temp_time),
            temp_time->tm_hour, temp_time->tm_min, months[temp_time->tm_mon],
            temp_time->tm_mday, temp_time->tm_year + 1900);
    _scale = 1;
    if (_count >= _state_swap) {
      _state++;
      _count = 0;
    }
    break;
  case 5:
    sprintf(buf, "%.2f°F", ((bmp_readings.temperature / 100.f) * 1.8) + 32);
    if (_count >= (_state_swap + 2)) {
      _state++;
      _count = 0;
    }
    break;
  case 6:
    sprintf(buf, "%.3f kPa", bmp_readings.pressure / 1000.f);
    if (_count >= (_state_swap - 1)) {
      _state = 0;
      _count = 0;
    }
    break;
  default:
    sprintf(buf, "%s", "Uh Oh -- PANIC!");
  } // end switch _state

  if (print_flag == PRINT_ON) {
    printf("\ncase [%d]\t", _state);
    printf("%s", uart_buf);
  }
  /* free(buf2); */
  _count++;

  ssd1306_draw_string(_disp, 1, 16, _scale, buf);
  ssd1306_show(_disp);

  return 1000 * 1000; // recall this alarm in [return value] microseconds, so in
                      // 1 second
} // end rtc_LCD_alarm_callback

/* here we populate some of our good stuff with the data stored on the epprom.
 * So, this is less setting UP the eeprom and setting up FROM the eeprom,
 * but this is just semantics.
 * */
int setup_eeprom_with_callback_time(ssd1306_t *display_pnt) {
  int now = make_current_timestamp();

  ssd1306_clear(display_pnt);
  ssd1306_draw_string(display_pnt, 0, 0, 1, "set up EEPROM");
  ssd1306_show(display_pnt);

  eeprom_init(&eeprom, SPI_PORT, EEPROM_CS_PIN, EEPROM_BYTES_PER_PAGE);
  //* initialize eeprom CS pin:
  gpio_init(eeprom.cs_pin);
  gpio_set_dir(eeprom.cs_pin, GPIO_OUT);
  gpio_put(eeprom.cs_pin, 1);

  printf("Dump EEPROM contents:\n");
  eeprom_dump_all(&eeprom);

  // do we start fresh or is the eeprom already full of good juicy data?
  // this is where we try & find out
  int32_t loops_one = eeprom_get_four_bytes(&eeprom, EEPROM_LOOPS_ADDRESS);
  int32_t loops_two = eeprom_get_four_bytes(&eeprom, LOOPS_TWO_ADDRESS);
  int32_t loops_three = eeprom_get_four_bytes(&eeprom, LOOPS_THREE_ADDRESS);

  int32_t next_eeprom_write_time;

  /* If there's already data on the EEPROM we pick up
   *  where we left off
   *  */
  if ((loops_one != 0) & (loops_one == loops_two) &
      (loops_two == loops_three)) {
    bmp_readings.high_temperature =
        eeprom_get_four_bytes(&eeprom, EEPROM_HIGH_TEMP_ADDRESS);
    bmp_readings.high_temp_time =
        eeprom_get_four_bytes(&eeprom, EEPROM_HIGH_T_TIME);

    bmp_readings.low_temperature =
        eeprom_get_four_bytes(&eeprom, EEPROM_LOW_TEMP_ADDRESS);
    bmp_readings.low_temp_time =
        eeprom_get_four_bytes(&eeprom, EEPROM_LOW_T_TIME);

    eeprom.current_address =
        eeprom_get_four_bytes(&eeprom, CURRENT_EEPROM_ADDRESS);
    if ((eeprom.current_address > EEPROM_LAST_ADDRESS) |
        (eeprom.current_address < EEPROM_DATA_START)) {
      eeprom.current_address = EEPROM_DATA_START;
      eeprom_send_four_bytes(&eeprom, CURRENT_EEPROM_ADDRESS,
                             eeprom.current_address);
    }

    // when do we next write to the EEPROM?
    if (eeprom.current_address <= EEPROM_DATA_START) {
      next_eeprom_write_time =
          ((eeprom_get_four_bytes(&eeprom, EEPROM_LAST_ADDRESS - 4) + 900) -
           now) *
          1000;
    } else {
      next_eeprom_write_time =
          ((eeprom_get_four_bytes(&eeprom, eeprom.current_address - 4) + 900) -
           now) *
          1000;
    }
    if (next_eeprom_write_time <= 0) {
      next_eeprom_write_time = 100;
    }
    if (next_eeprom_write_time > 900 * 1000) {
      next_eeprom_write_time = 900 * 1000;
    }

    // time_t temp_time_t = (time_t)(loops_one - MOUNTAIN_STANDARD_OFFSET +
    //                               DAYLIGHT_SAVINGS_OFFSET);
    // struct tm *temp_time = localtime(&temp_time_t);
    struct tm *temp_time = tm_from_timestamp(loops_one);
    printf("data populated from EEPROM, current address is %X\n",
           eeprom.current_address);
    printf("next write to eeprom in is %d seconds\n",
           (next_eeprom_write_time / 1000));
    printf("EEPROM started at %2d:%02d, %s %d %d\n", temp_time->tm_hour,
           temp_time->tm_min, months[temp_time->tm_mon], temp_time->tm_mday,
           temp_time->tm_year + 1900);

  } else {
    /* if we're starting fresh we clear & set up a new EEPROM
     * */
    ssd1306_clear(display_pnt);
    ssd1306_draw_string(display_pnt, 0, 0, 1, "Clear EEPROM");
    ssd1306_show(display_pnt);

    eeprom_clear_all(&eeprom);

    bmp_readings.high_temperature = bmp_readings.temperature;
    bmp_readings.low_temperature = bmp_readings.temperature;
    bmp_readings.high_temp_time = now;
    bmp_readings.low_temp_time = now;

    eeprom.current_address = EEPROM_DATA_START;
    eeprom_send_four_bytes(&eeprom, CURRENT_EEPROM_ADDRESS,
                           eeprom.current_address);

    eeprom_send_four_bytes(&eeprom, EEPROM_HIGH_TEMP_ADDRESS,
                           bmp_readings.high_temperature);
    eeprom_send_four_bytes(&eeprom, EEPROM_HIGH_T_TIME,
                           bmp_readings.high_temp_time);

    eeprom_send_four_bytes(&eeprom, EEPROM_LOW_TEMP_ADDRESS,
                           bmp_readings.low_temperature);
    eeprom_send_four_bytes(&eeprom, EEPROM_LOW_T_TIME,
                           bmp_readings.low_temp_time);

    eeprom_send_four_bytes(&eeprom, EEPROM_LOOPS_ADDRESS, now);
    eeprom_send_four_bytes(&eeprom, LOOPS_TWO_ADDRESS, now);
    eeprom_send_four_bytes(&eeprom, LOOPS_THREE_ADDRESS, now);

    // next EEPROM write is in 15 minutes
    next_eeprom_write_time = 1000 * 60 * 15;

    ssd1306_clear(display_pnt);
    ssd1306_draw_string(display_pnt, 0, 0, 1, "EEPROM done!");
    ssd1306_show(display_pnt);
    printf("data populated from fresh reading, current address is %X, should "
           "be %X",
           eeprom.current_address, EEPROM_DATA_START);
    printf("next write to eeprom in is %d seconds\n",
           (next_eeprom_write_time / 1000));
  }
  return next_eeprom_write_time;
} // setup_eeprom_with_callback_time(ssd1306_t *display_pnt)
