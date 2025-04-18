```
libraries
├── bmp280_i2c -> {where you put your stuff}/libraries/bmp280_i2c
├── README.md
└── wireless -> {where you put your stuff}/libraries/wireless
```


##### Libraries for projects!
Libraries are symbolically linked into the `libraries` folder; currently they are then built by this project. This
is primarly because of non-standardized interafce -- some libraries require explicit i2c or spi declarations before building,
while others can make due with pointers.

Once this is fixed, libraries will likely be prebuilt and linked.
