please slelect one frontend_config.ini based on hisilicon demo board. or configure the frontend_config.ini based on own hardware board.
for example:
     Hi3796MV2DMA board, use command: cp frontend_config.hi3796mv2dma.ini frontend_config.ini

1. hisilicon external tuner board information:
    |  board type            | demod      | i2c  |  tuner     |  i2c  |  signal
 ------------------------------------------------------------------------------------
  1 | hi3130V200 + tda18250b | hi3130v200 | 0xa0 |  tda18250b |  0xc0 |  cable
  2 | hi3130v200 + r836      | hi3130v200 | 0xa0 |  r836      |  0x34 |  cable
  3 | hi3136v100 + av2012    | hi3136v100 | 0xb4 |  av2012    |  0xc6 |  satellite
  4 | hi3136v100 + rda5815m  | hi3136v100 | 0xb4 |  rda5815m  |  0x18 |  satellite
  5 | hi3137v100 + mxl608    | hi3137v100 | 0xb8 |  mxl608    |  0xc0 |  terrestrial
  6 | atbm8869t  + mxl608    | atbm8869t  | 0x80 |  mxl608    |  0xc0 |  dtmb
  7 | mxl214 (fullband)      | mxl214     | 0xa0 |  mxl214    |  0xa0 |  cable
  8 | mxl683 (fullband)      | mxl683     | 0xc0 |  mxl683    |  0xc0 |  isdbt
 ------------------------------------------------------------------------------------

2. hisilicon internal demod information:
    |  chipset     | demod      | i2c
 ----------------------------------------
  3 | hi3796mv200  | hi3130i    | 0xa0
  4 | hi3716mv450  | hi3130i    | 0xa0
  5 | hi3716mv430  | internal0  | 0xb8
 ----------------------------------------

3. hisilicon chipset demo board's default tuner config:
     | chipset board | demod            | tuner            | i2c chn | dmx port | reset   | port type
 ------------------------------------------------------------------------------ --------------------------
  5  | Hi3798CV2DMB  | external tuner board                |    2    |  32      | gpio8_7 |  serial_nosync
  6  | Hi3798MV2DMG  | hi3130V200[0xa0] |  mxl608   [0xc0] |    2    |  32      | gpio4_2 |  serial_nosync
  7  | Hi3796MV2DMA  | external tuner board                |    1    |  32      | gpio7_6 |  serial_nosync
  8  | Hi3796MV2DMB  | internal  [0xa0] |  r836     [0x34] |    5    |  0       | none    |  serial_nosync
  9  | Hi3716M43DMA  | internal  [0xb8] |  mxl608   [0xc0] |    2    |  0       | none    |  parallel
  10 | Hi3716M43DMB  | internal  [0xb8] |  rda5815  [0x18] |    2    |  0       | none    |  parallel
 ------------------------------------------------------------------------------ --------------------------




