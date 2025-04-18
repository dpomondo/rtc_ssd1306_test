```
libraries/
├── libraries/bmp280_i2c/
├── libraries/eeprom/
├── libraries/lcd1602/
├── libraries/ntp_request/
├── libraries/pico-ssd1306/
├── libraries/README.md
└── libraries/wireless/
```


##### Libraries for projects!
Libraries are symbolically linked into the `libraries` folder; currently they are then built by this project. This
is primarly because of non-standardized interafce -- some libraries require explicit i2c or spi declarations before building,
while others can make due with pointers.

Once this is fixed, libraries will likely be prebuilt and linked.
