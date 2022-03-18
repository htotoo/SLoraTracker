# SLoraTracker

This project is to demonstrate the SProto, SWifi, SMqtt libs.

It uses 2 ESP32-s, one TTGO Lora32 (newer edition with AXP20X power chip) with an accelerometer ( ADXL345 ), and one with a Semtech sx1276.
The TTGO sleeps, until the accelerator sends an IRQ, then it starts to send it's coordinates with Lora packed with the SProto protocol. (added some extra parameters, eg battery voltage). Will go to sleep after it nt getting more IRQs for some minutes.
The other ESP32 will connect to Home Assistant server via MQTT. It automaticaly registers the sensors and device trackers it sees via SProto, and updates the data for them.

## These are fully functional, but mostly for demonstration. I use it as a smart car alarm system, but the code is not pretty. It's separated to multiple files, to prevent using a single big unreadeable file.

Libraries needed:
Lora: https://github.com/sandeepmistry/arduino-LoRa
ADXL345: https://github.com/adafruit/Adafruit_ADXL345
Power AXP20X: https://github.com/lewisxhe/AXP202X_Library
SWifi: https://github.com/htotoo/SWifi
SMqtt: https://github.com/htotoo/SMqtt
SProto: https://github.com/htotoo/SProto
