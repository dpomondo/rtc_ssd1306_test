```
rtc_ssd1306_test
├── build/
├── CMakeLists.txt
├── include/
│   ├── {super_secret_file_with_wifi_credentials_do_not_open.h}   
│   ├── bmp280_i2c {link_to} -> {where_libraries_live}/shared/libraries/bmp280_i2c/
│   ├── eeprom {link_to} -> {where_libraries_live}/shared/libraries/eeprom/
│   ├── ntp_request {link_to} -> {where_libraries_live}/shared/libraries/ntp_request/
│   └── pico-ssd1306 {link_to} -> {where_libraries_live}/shared/libraries/pico-ssd1306/
├── lwipopts.h <- copied from wifi examples in pico-examples
├── pico_sdk_import.cmake <- copied from pico-sdk/external/
├── README.md
└── src/
    ├── CMakeLists.txt
    ├── rtc_ssd1306_test.c
    └── rtc_ssd1306_test.h
```
