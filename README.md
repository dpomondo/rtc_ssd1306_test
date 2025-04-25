```
rtc_ssd1306_test
├── CMakeLists.txt
├── compile_commands.json <- linked from build/compile_commands.json
├── lwipopts.h <- copied from wifi examples in pico-examples
├── pico_sdk_import.cmake <- copied from pico-sdk/external/
├── README.md
├── .gitignore
├── build/
│   ├── open.sh*
├── include/
│   ├── CMakeLists.txt
│   └── rtc_ssd1306_test.h
├── libraries/
│   ├── README.md
│   ├── bmp280_i2c -> {place where libraries live}/shared/libraries/bmp280_i2c//
│   ├── eeprom -> {place where libraries live}/shared/libraries/eeprom//
│   ├── lcd1602 -> {place where libraries live}/shared/libraries/lcd1602//
│   ├── ntp_request -> {place where libraries live}/shared/libraries/ntp_request/
│   ├── pico-ssd1306 -> {place where libraries live}/shared/libraries/pico-ssd1306//
│   └── wireless -> {place where libraries live}/shared/libraries/wireless//
└── src/
    ├── CMakeLists.txt
    └── rtc_ssd1306_test.c
```
