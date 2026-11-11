# HandHeld Car
this is an advanced handheld controller that drives a four wheel drive car by tilting it. pitching the controller forward drives the car forward, pitching it back reverses, and rolling it left or right steers it. the harder i tilt, the faster the car goes, and a dead zone around level keeps the car still when my hand is flat. an ESP32 inside the controller reads an IMU and sends the tilt values to a second ESP32 on the car over ESP NOW. if the car stops hearing from the controller, it stops. A button arms the car and another cycles between crawl, normal and turbo speeds. the car brakes on its own for obstacles and reports the distance back, so the controller vibrates faster as I get closer to a wall. shaking the controller makes the car spin, underglow lights follow its speed and steering, and a status LED shows whether the controller is armed and linked. There’s also a follow-me mode where the car trails me at a set distance. 

# Bill Of Materials

| Component | Quantity | Price/Unit (USD) | Purpose | Purchase Link |
| --- | ---: | ---: | --- | --- |
| ESP32 dev board, 38 pin | 2 | 3.65 | car and handheld brain | [Link](https://www.electropi.in/esp32-development-board-with-wifi-bluetooth-india?gad_source=1&gad_campaignid=23329895947&gbraid=0AAAAA-s6SdWARivrgrmTVY49l-2nRtMR1&gclid=Cj0KCQiAwdDXBhDuARIsAFdLukmY-CL1V-CILhUB4qX3MZulMJIu2CTsH-2Qe_Orw-h2z3_UO7KdbXgaAnA1EALw_wcB) |
| Ai thinker BU03 kit UWB board | 4 | 34.90 | 3 car anchors and 1 handheld tag for follow me | [Link](https://robosap.in/product/ai-thinker-bu03-kit-development-board-uwb-module/?srsltid=AYCX9vLzLQwtqSO5vIl1zXNk3p9wGnJhZ2dLUPu5aMqhmjjbJXnJL5poIUc) |
| USB cables | 1 | 6.40 | flashing configuring and charging | [Link](https://www.flipkart.com/portronics-usb-type-c-cable-3-1-m-konnect-b/p/itm520c0fac46ed5?pid=ACCGKZNAK7DUYE7Z&lid=LSTACCGKZNAK7DUYE7ZLJJCFY&marketplace=FLIPKART&cmpid=content_data-cable_8965229628_gmc) |
| header pins | 1 | 0.31 | mounting boards and modules | [Link](https://probots.co.in/40-x-1-male-header-pins-long.html) |
| geared DC motor 6V TT style | 4 | 5.60 | wheel drive for the 4WD car | [Link](https://robu.in/product/n20-6v-65rpm-metal-gear-motor-with-encoder-d-type/) |
| wheel 65mm | 4 | 1.70 | drive wheels | [Link](https://robu.in/product/65mm-robot-smart-car-12-rim-wheel-silver/?gad_source=4&gad_campaignid=17416544847&gbraid=0AAAAADvLFWdI8qQIjqOkFagVXEiOUM64R&gclid=Cj0KCQiAwdDXBhDuARIsAFdLuklfXMTYcRklgIB2Ef0n9BGI0t6Ji3WAOcUyUQc6Cid9sCrsW9M2bXkaAqNMEALw_wcB) |
| TB6612FNG dual motor driver module | 2 | 1.70 | one driver per side | [Link](https://www.flyrobo.in/tb6612fng-dual-motor-driver-module-1a-for-arduino-micro-controller?tracking=ads&tracking=feed1505&gad_source=4&gad_campaignid=23845158606&gbraid=0AAAAAC6AkE931gtmV3xZ6HmdBOTHvMj3h&gclid=Cj0KCQiAwdDXBhDuARIsAFdLukkTo-1rcX1c2Q9G619mjgqT5MjronApQXDWgamQl6jCiqsG3JytomkaAn7eEALw_wcB) |
| VL53L1X ToF distance sensor module | 1 | 6.20 | front obstacle sensing and auto brake | [Link](https://robu.in/product/gy-53-vl53l0x-laser-tof-flight-time-ranging-sensor-module-serial-port-pwm-output/?gad_source=1&gad_campaignid=20387462343&gbraid=0AAAAADvLFWfoQA8hLsamOmHdCxHUEsFA8&gclid=Cj0KCQiAwdDXBhDuARIsAFdLukm9ShQxBQtZIhldVqDCweAvLE_admxJssl9mF6ccOD2riVXH4Gm4m0aAsRzEALw_wcB) |
| HC SR04 ultrasonic sensor | 1 | 2.0 | rear obstacle sensing while reversing | [Link](https://www.amazon.in/Robocraze-Ultrasonic-Non-Contact-Resolution-Automation/dp/B01I1ZTPJC/ref=asc_df_B01I1ZTPJC?mcid=664407c1f3b5342199547c31df5b811b&tag=googleshopdes-21&linkCode=df0&hvadid=709963085501&hvpos=&hvnetw=g&hvrand=500928013182251341&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=9152158&hvtargid=pla-406302943145&psc=1&hvocijid=500928013182251341-B01I1ZTPJC-&hvexpln=0&gad_source=4) |
| WS2812B LED strip 5V | 1 | 2.70 | underglow. about 10 pixels fit the plate recess, so a 60 LEDs per metre strip cut to 17 cm is enough | [Link](https://www.amazon.in/InLightss-Ws2812b-Flexible-Individual-Addressable/dp/B0CXVKBRM8/ref=asc_df_B0CXVKBRM8?mcid=e63d5728278a3bdaa28ba1d1e5d01e71&tag=googleshopdes-21&linkCode=df0&hvadid=709931555215&hvpos=&hvnetw=g&hvrand=1181299470684957686&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=9152158&hvtargid=pla-2292047082166&hvocijid=1181299470684957686-B0CXVKBRM8-&hvexpln=0&gad_source=1&th=1) |
| 74AHCT125 level shifter IC | 1 | 26.15 | clean 5V data signal for the LED strip from a 3.3V pin | [Link](https://www.amazon.in/TQDLYKHS-SN74AHCT125N-MM74ACT125N-MC74ACT125N-MC74ACT125NG/dp/B0CKV2Q71Z) |
| 18650 Li-ion cell | 2 | 0.50 | car battery | [Link](https://quartzcomponents.com/products/18650-li-ion-rechargeable-battery?variant=31898092732551&country=IN&currency=INR&utm_medium=product_sync&utm_source=google&utm_content=sag_organic&utm_campaign=sag_organic&srsltid=AYCX9vLlxmyYjGR-ov-pKgMZMf46eWoKBt6bOSrUQ-8MS_fsKFiV0UEJvXc) |
| 2S 18650 battery holder | 1 | 1 | holds the two car cells | [Link](https://www.amazon.in/Electro-Electron-18650-Battery-Holder/dp/B0GJ61WB5H) |
| 2S BMS board with balancing | 1 | 1 | car battery protection and balancing | [Link](https://www.flyrobo.in/2s-10a-8.4v-li-ion-battery-protection-board-with-balance?tracking=ads&tracking=feed1505&gad_source=4&gad_campaignid=23845158606&gbraid=0AAAAAC6AkE931gtmV3xZ6HmdBOTHvMj3h&gclid=Cj0KCQiAwdDXBhDuARIsAFdLukk9tMeCVc2B79oaKBjyYMf-ZpYof99POuLB7m_Y0J_-KFfFj0yoAW4aAlsQEALw_wcB) |
| 2S 18650 charger | 1 | 1.55 | Charges the car cells. | [Link](https://techdelivers.com/18650-dual-battery-fast-charger-5volt-1amp?tag=220V%20AC%20input%20battery%20charger%20for%2018650%20batteries&srsltid=AYCX9vJiPWNHYfwyby6q_dClRSsXb-BhUUOTq8fs8vcfxt07mui1NoAV6D4) |
| Buck converter 6V motor rail | 1 | 2.50 | Regulated motor supply | [Link](https://robu.in/product/dc-dc-5-40v-to-6v-1a-buck-converter-module/) |
| Buck converter 5V logic rail 5A | 1 | 1.25 | separate supply for the ESP32 3 UWB anchors sensors and LED strip | [Link](https://robocraze.com/products/5a-dc-to-dc-cc-cv-lithium-battery-step-down-charging-board-led-power-converter-lithium-charger-step-down-module-xl4015-red?variant=40193584627865&country=IN&currency=INR&utm_medium=product_sync&utm_source=google&utm_content=sag_organic&utm_campaign=sag_organic&utm_source=googleads&utm_medium=ppc&utm_campaign=24413206508&utm_content=_&utm_term=&campaignid=24413206508&adgroupid=&campaign=24413206508&gad_source=4&gad_campaignid=24413209877&gbraid=0AAAAADgHQvZBeFFL3DgXfxBpvo-D26M71&gclid=Cj0KCQiAwdDXBhDuARIsAFdLukkzWbX21UHinh1_2RjybQaICruS6K1hayY5-a4vuF13ZabzstKfieQaAsIuEALw_wcB) |
| Rocker switch 21 x 15 mm rated for motor current | 1 | 0.25 | car power on/off | [Link](https://roboticsdna.in/product/black-rocker-switch-on-off-on-3-pin-21x15mm-dpdt-kcd1-6a-250vac-10a-125vac/?srsltid=AYCX9vLMLIz86e5fsEZano-a_LrXzf9e6jw3X1dPVp8SXRuG7BtQcvTJGVA) |
| inline fuse holder and 5A fuse | 1 | 1.65 | protects the car battery line from shorts | [Link](https://www.amazon.in/Ovicart-6-35x25mm-Fuse-5A-Glass/dp/B09YHLYWBZ) |
| electrolytic capacitor 1000 uF 16V | 2 | 0.087 | motor rail and LED strip smoothing | [Link](https://www.ktron.in/product/16v-1000uf-electrolytic-capacitor-keltron/?srsltid=AYCX9vJcxFTIHDQ2Ly7RgmF-mX7WzHQeSrYXQ6X5hOThjEf3a86VzSHYQqA) |
| ceramic capacitor 100 nF | 1 | 2.45 | across motor terminals to reduce noise and the handheld battery divider | [Link](https://kitsguru.com/products/100nf-50v-monolithic-ceramic-capacitor-104-100pcs?srsltid=AYCX9vLDutLDwYmq3oLevVGC4t7QXdDcBdqR8VNdSj7uimYYBgbEVwxSNFw) |
| JST connector set | 1 | 2.07 | battery and rail connections | [Link](https://www.amazon.in/Electronic-Spices-Connection-Installation-Electronics/dp/B07NBV19QQ) |
| screw terminal blocks (pack) | 1 | 0.90 | motor and power wiring | [Link](https://zbotic.in/product/4-pin-2-54mm-pitch-pluggable-screw-terminal-block-pack-of-3/?srsltid=AYCX9vKdhMqhED46L_01QS9g0CnB6WdxC0M4_jygRnLZZ1ZAF0q1hodXbjk) |
| MPU6050 IMU module | 1 | 2 | handheld tilt sensing | [Link](https://techdelivers.com/mpu-6050-gy-521-6-axis-accelerometer-gyroscope-module-i2c-interface-motion-tracking-arduino-compatible-robotics-sensor-embedded-system?tag=Arduino%20motion%20sensor&srsltid=AYCX9vJTI9NBqdqDEBCW80XZygur-nNms_w9V9YQfhBAuWaJQInDHxNdE4U) |
| SSD1306 0.96 inch OLED I2C | 1 | 1.78 | handheld status display | [Link](https://robocraze.com/products/0-96in-oled-display-module-4pin?variant=40192586645657&country=IN&currency=INR&utm_medium=product_sync&utm_source=google&utm_content=sag_organic&utm_campaign=sag_organic&utm_source=googleads&utm_medium=ppc&utm_campaign=24421808816&utm_content=_&utm_term=&campaignid=24421808816&adgroupid=&campaign=24421808816&gad_source=1&gad_campaignid=24427506883&gbraid=0AAAAADgHQvaw_WKBq2SIrpmvRqEwiSoDb&gclid=Cj0KCQiAwdDXBhDuARIsAFdLuknjv9s6FgVpbVTh8xsB0oKghC9cv_tzBIN1QXT-sEoqGCXFyXHrSPsaAjekEALw_wcB) |
| tactile push buttons 6 x 6 mm (pack) | 1 | 2 | arm and mode buttons | [Link](https://www.amazon.in/ElectroBot-Momentary-Tactile-Push-Button/dp/B07PRRRBRY?source=ps-sl-shoppingads-lpcontext&ref_=fplfs&psc=1&smid=A1P9ZW4IHIOLEJ) |
| RGB LED 5mm common cathode | 1 | 0.45 | handheld status LED | [Link](https://robocraze.com/products/5mm-common-anode-rgb-led-4pin-pack-of-10?variant=45740181946592&country=IN&currency=INR&utm_medium=product_sync&utm_source=google&utm_content=sag_organic&utm_campaign=sag_organic&campaignid=20031834624&adgroupid=&keyword=&device=c&gad_source=4&gad_campaignid=20040036619&gbraid=0AAAAADgHQvZ7zdqnu6gXRf0ET7EbeklYj&gclid=Cj0KCQiAwdDXBhDuARIsAFdLukmEy4s3NHl11yUZ_B_PB-g58GMxJsz9yyXWUSySW4V1LHJixMKjgbMaArTiEALw_wcB) |
| coin vibration motor 3V 10 mm | 1 | 0.85 | haptic feedback | [Link](https://robu.in/product/erm-coin-vibration-motor-d10mm-w-3-4mm/) |
| NPN transistor BC547 (pack) | 1 | 0.15 | switches the vibration motor | [Link](https://robocraze.com/products/bc547-transistor-ic-pack-of-5?variant=40192360448153&country=IN&currency=INR&utm_medium=product_sync&utm_source=google&utm_content=sag_organic&utm_campaign=sag_organic&utm_source=googleads&utm_medium=ppc&utm_campaign=24427476646&utm_content=_&utm_term=&campaignid=24427476646&adgroupid=&campaign=24427476646&gad_source=4&gad_campaignid=24421821569&gbraid=0AAAAADgHQvYPrRwgBnrZH-9Qzs83fURyP&gclid=Cj0KCQiAwdDXBhDuARIsAFdLuknXiSmVHTldu4VVpePRvwZ8ZawgU2OxqTyrCCjoUO5tV-otSHaZjmMaAiFTEALw_wcB) |
| diode 1N4148 (pack) | 1 | 0.40 | flyback protection for the vibration motor | [Link](https://robocraze.com/products/1n4148-diode-pack-of-10?variant=40194274263193&country=IN&currency=INR&utm_medium=product_sync&utm_source=google&utm_content=sag_organic&utm_campaign=sag_organic&campaignid=20031834624&adgroupid=&keyword=&device=c&gad_source=4&gad_campaignid=20040036619&gbraid=0AAAAADgHQvZ7zdqnu6gXRf0ET7EbeklYj&gclid=Cj0KCQiAwdDXBhDuARIsAFdLuklG2JqebRTEXxcc2d6D92zG0SLDSBSctLgA0h_VfjRYozjpgEsbBwgaAhCwEALw_wcB) |
| 1S LiPo pouch cell 703450 class (about 7 x 34 x 50 mm) 1500 to 1800 mAh with protection board and JST-PH 2.0 plug | 1 | 49 | Handheld battery. Check the plug polarity with a multimeter before connecting | [Link](https://www.amazon.in/EEMB-Rechargeable-Connector-Wireless-Polarity/dp/B09DPL8RZH/ref=asc_df_B09DPL8RZH?mcid=01cd65400d2f34cfab674d5117acf47a&tag=googleshopdes-21&linkCode=df0&hvadid=710055740841&hvpos=&hvnetw=g&hvrand=17338530325152052939&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=9152158&hvtargid=pla-1608082842470&psc=1&hvocijid=17338530325152052939-B09DPL8RZH-&hvexpln=0&gad_source=4) |
| TP4056 charging module with protection | 1 | 0.60 | charges the handheld battery | [Link](https://electroglobal.in/store/tp4056-charging-module-battery-protection?gad_source=4&gad_campaignid=24202967268&gbraid=0AAAAAqZ1s0FoFYKJxCBPO8dcTk1V9jjs-&gclid=Cj0KCQiAwdDXBhDuARIsAFdLukmmcP3cqk1YUYaM2gcEphQwuCxsotqwEIHoloTAEDSyNOS6IQZkvjUaAuRfEALw_wcB) |
| 5V boost converter module 1S input 2A or more | 1 | 1.30 | steps the handheld battery up to 5V for the ESP32 and BU03 tag | [Link](https://robu.in/product/ip5310-5v-3a-type-c-boost-power-module/?gad_source=4&gad_campaignid=17427802559&gbraid=0AAAAADvLFWdN5s74L0XGT7f-0pE1oAqpM&gclid=Cj0KCQiAwdDXBhDuARIsAFdLukmThpsMj0Onp5d3BLxRT-GEfU8qBN01nq32Tt4wV69b1fkwQI87GS4aAtiDEALw_wcB) |
| slide switch about 8 x 4 x 4 mm | 1 | 0.35 | handheld power on/off | [Link](https://shokitech.com/product/spdt-slide-switch-pack-of-5-for-electronic-projects/?srsltid=AYCX9vIExJUZ794N1LbStErgVsyI8fZ_O_Rf_ULy30zu-JWMu4eVZ7r3slk) |
| resistor assortment | 1 | 2.40 | dividers LED resistors transistor base and the HC SR04 echo divider | [Link](https://amazon.in/JukrTM-Assorted-Carbon-Resistors-Package/dp/B081LJTFW5) |
| perf board about 70 x 50 mm for the car and a small one for the handheld | 2 | 0.45 | one for the car and one for the handheld | [Link](https://rajivelectronics.com/product/70-x-50-mm-double-side-2-54-mm-pitch-general-purpose-dotted-pcb?gad_source=4&gad_campaignid=24107160685&gbraid=0AAAABC43kOVR8qDunkQvzEGHUXJK4tOnr&gclid=Cj0KCQiAwdDXBhDuARIsAFdLuknEEF4Z1C4UoZ4Ab-hUJePvGpG2w6BVTiiak60CKXY7MiL1spxKWyoaAv3FEALw_wcB) |
| silicone hookup wire | 1 | 0.30 | general wiring | [Link](https://robu.in/product/high-quality-ultra-flexible-22awg-silicone-wire-1000m-black/) |
| heat shrink tubing assortment | 1 | 1.75 | insulating joints | [Link](https://www.amazon.in/Rpi-shop-Polyolefin-Insulated-Multicolour/dp/B08S3TP2Q6/ref=asc_df_B08S3TP2Q6?mcid=b72b9ea37df73502a1a0f900ddf88345&tag=googleshopdes-21&linkCode=df0&hvadid=709963085708&hvpos=&hvnetw=g&hvrand=16242052140234366210&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=9152158&hvtargid=pla-1147527768165&psc=1&hvocijid=16242052140234366210-B08S3TP2Q6-&hvexpln=0&gad_source=4) |
| double sided foam tape and cable ties | 1 | 0.40 | holds the LiPo vibration motor and loose wiring | [Link](https://www.zepto.com/pn/thyone-ty-tp23-1pcs-double-sided-foam-tape-3-meter-length-24-mm-width/pvid=954c6ee8-4868-46e8-a52e-0dd70d01cc2a?srsltid=AYCX9vKDrWa4oCSiKSV6wqthuEgH0CxNc7E8pXG2itvzm46_TBriMXXKyR8) |
| M3 screws nuts and standoffs kit | 1 | 15.16 | chassis and deck mounting | [Link](https://amazon.in/350PCS-M3-Female-Standoff-Assortment/dp/B0BXT71WJK?source=ps-sl-shoppingads-lpcontext&ref_=fplfs&psc=1&smid=A1YZ9MMK17G6RH) |
| M3 heat set inserts and M3 screws 8 to 10 mm | 1 | 1.10 | six deck posts on the car chassis | [Link](https://robu.in/product/m3-x-8-mm-brass-heat-set-threaded-round-insert-nut-25-pcs/) |
| PLA filament 1 kg | 1 | 13.90 | chassis deck and handheld enclosure printing | [Link](https://ideal3d.in/products/esun-pla-filament-white?srsltid=AYCX9vIbQXf4kmvjAZsr-c4zZwCzzGXoIBBy8v6UrFiig5aEf0CwUShp) |

<b>Estimated total: $333.84</b>

# Circuit Diagram 

## handheld diagram

## handheld pin map

<b>Board: 38 pin ESP32 dev board.</b>

## Sensor and display (shared I2C bus)

| Component | Component Pin | MCU Pin |
|---|---|---|
| MPU6050 | VCC | 3V3 |
| MPU6050 | GND | Star ground |
| MPU6050 | SCL | GPIO22 |
| MPU6050 | SDA | GPIO21 |
| MPU6050 | AD0 | GND (fixes address at 0x68) |
| MPU6050 | INT | Not connected |
| SSD1306 OLED | VCC | 3V3 |
| SSD1306 OLED | GND | Star ground |
| SSD1306 OLED | SCL | GPIO22 |
| SSD1306 OLED | SDA | GPIO21 |

## Inputs and outputs

| Component | Component Pin | MCU Pin |
|---|---|---|
| Arm button | Pin 1 | GPIO25 (internal pull up) |
| Arm button | Pin 2 | GND |
| Mode button | Pin 1 | GPIO26 (internal pull up) |
| Mode button | Pin 2 | GND |
| RGB LED (common cathode) | Red via 220 ohm | GPIO16 |
| RGB LED (common cathode) | Green via 220 ohm | GPIO17 |
| RGB LED (common cathode) | Blue via 220 ohm | GPIO18 |
| RGB LED (common cathode) | Common cathode | GND |
| NPN transistor | Base via 1k ohm | GPIO19 |
| NPN transistor | Emitter | GND |
| NPN transistor | Collector | Vibration motor negative terminal |
| Vibration motor | Positive terminal | 3V3 |
| Diode 1N4148 | Cathode (stripe) | 3V3 (motor positive) |
| Diode 1N4148 | Anode | Transistor collector (motor negative) |
| Battery divider | Top 100k from battery positive (after the slide switch) | Midpoint node |
| Battery divider | Midpoint node (100 nF to GND) | GPIO34 (ADC1, input only) |
| Battery divider | Bottom 100k | GND |

## Power chain

| Component | Component Pin | MCU Pin |
|---|---|---|
| LiPo cell | Positive and negative | TP4056 module B+ and B- |
| TP4056 module | OUT+ | Slide switch, then boost converter IN+ |
| TP4056 module | OUT- | Boost converter IN- and star ground |
| Boost converter 5V | OUT+ | ESP32 VIN (5V pin) and BU03 tag 5V (pin 20) |
| Boost converter 5V | OUT- | Star ground |
| BU03 tag | GND (pin 16 or 22) | Star ground |
| BU03 tag | TX1 and RX1 | Not connected |

## I2C addresses 

| Device | Address |
|---|---|
| MPU6050 | 0x68 |
| SSD1306 OLED | 0x3C |
## car diagram

## car pin map

<b>Board: 38 pin ESP32 dev board.</b>

## Left driver (TB6612FNG module 1)

| Component | Component Pin | MCU Pin |
|---|---|---|
| TB6612FNG #1 | AIN1 | GPIO25 |
| TB6612FNG #1 | BIN1 | GPIO25 |
| TB6612FNG #1 | AIN2 | GPIO26 |
| TB6612FNG #1 | BIN2 | GPIO26 |
| TB6612FNG #1 | PWMA | GPIO27 |
| TB6612FNG #1 | PWMB | GPIO27 |
| TB6612FNG #1 | STBY | GPIO23 (shared net, 10k pull down to GND) |
| TB6612FNG #1 | VCC | 3V3 |
| TB6612FNG #1 | VM | 6V motor rail |
| TB6612FNG #1 | GND | Star ground |
| Front left motor | Terminals | TB6612FNG #1 channel A outputs |
| Rear left motor | Terminals | TB6612FNG #1 channel B outputs |

## Right driver (TB6612FNG module 2)

| Component | Component Pin | MCU Pin |
|---|---|---|
| TB6612FNG #2 | AIN1 | GPIO32 |
| TB6612FNG #2 | BIN1 | GPIO32 |
| TB6612FNG #2 | AIN2 | GPIO33 |
| TB6612FNG #2 | BIN2 | GPIO33 |
| TB6612FNG #2 | PWMA | GPIO13 |
| TB6612FNG #2 | PWMB | GPIO13 |
| TB6612FNG #2 | STBY | GPIO23 |
| TB6612FNG #2 | VCC | 3V3 |
| TB6612FNG #2 | VM | 6V motor rail |
| TB6612FNG #2 | GND | Star ground |
| Front right motor | Terminals | TB6612FNG #2 channel A outputs |
| Rear right motor | Terminals | TB6612FNG #2 channel B outputs |

## Power nets

| Component | Component Pin | MCU Pin |
|---|---|---|
| 2S battery + BMS | P+ via main switch and fuse | Buck 6V input and Buck 5V input |
| Buck converter 6V | Output | TB6612FNG VM (both drivers) |
| Buck converter 5V | Output | ESP32 VIN (5V pin) |
| Buck converter 5V | Output | Reserved for 3 BU03 anchors, ultrasonic, LED strip |
| ESP32 | GND | Star ground |
| Battery pack | Negative (BMS P-) | Star ground |

## Car sensors, LEDs and UWB 

| Component | Component Pin | MCU Pin |
|---|---|---|
| VL53L1X ToF | SDA | GPIO21 |
| VL53L1X ToF | SCL | GPIO22 |
| VL53L1X ToF | VIN | 3V3 |
| VL53L1X ToF | GND | Star ground |
| VL53L1X ToF | XSHUT and GPIO1 | Not connected |
| HC-SR04 | VCC | 5V rail |
| HC-SR04 | TRIG | GPIO18 |
| HC-SR04 | ECHO | GPIO19 via divider (1k in series, 2k from GPIO19 to GND) |
| HC-SR04 | GND | Star ground |
| 74AHCT125 (one gate) | A input | GPIO4 |
| 74AHCT125 (one gate) | Y output | LED strip DIN via 330 ohm resistor |
| 74AHCT125 | VCC | 5V rail |
| 74AHCT125 | GND, all OE pins, all unused inputs | Star ground |
| WS2812B strip | 5V (1000 uF across 5V and GND at the strip start) | 5V rail |
| WS2812B strip | GND | Star ground |
| Battery divider | Top 20k from battery positive (after fuse and main switch) | Midpoint node |
| Battery divider | Midpoint node (100 nF to GND) | GPIO34 (ADC1, input only) |
| Battery divider | Bottom 10k | Star ground |
| BU03 anchor 0 | TX1 (pin 6) | GPIO16 (UART2 RX) |
| BU03 anchor 0 | RX1 (pin 7) | GPIO17 (UART2 TX) |
| BU03 anchor 0 | 5V (pin 20) | 5V rail |
| BU03 anchor 0 | GND (pin 16 or 22) | Star ground |
| BU03 anchors 1 and 2 | 5V (pin 20) and GND (pin 16 or 22) | 5V rail and star ground |

## I2C addresses

| Device | Address |
|---|---|
| VL53L1X ToF sensor | 0x29 |