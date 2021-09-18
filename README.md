# Smarthome firmware for ESP8266 - PZEM Power Meter
Firmware for my board named **ESP8266MCU12**, based on **ESP8266 microcontroller**.

This device use a **PZEM-004T board** as power meter device and use **MQTT protocol** to send data to **OpenHAB Smart Home Automation System**.

Built with **platformio** (https://platformio.org) and **visual studio code** (https://code.visualstudio.com)

To clone the project you need to use the *recursive* flag, to update libraries.

Depends on the following *dd libraries*:

 - ddcommon
 - ddwifi
 - ddmqtt
 - ddpzem004t

To build your firmware bin file, rename file *src/user-config-template.h* in *src/user-config.h* and set your **WIFI** and **MQTT** settings.

Firmware file will be in ***.pio/build/esp12e/firmware.bin***