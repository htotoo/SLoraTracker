#define RADIO_SCLK_PIN               5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DI0_PIN               26
#define RADIO_RST_PIN               23
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              32

#include <LoRa.h>

bool SetupLora()
{
 SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
  LoRa.setPins(RADIO_CS_PIN,RADIO_RST_PIN,RADIO_DIO1_PIN);
  int o = LoRa.begin(433E6);
  if (!o) {
    Serial.println("Starting LoRa failed!");
    return false;
  }
  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(12);
  LoRa.setCodingRate4(5);
  LoRa.setSignalBandwidth(125000); //7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, and 250E3.
  LoRa.setPreambleLength(8);
  LoRa.enableCrc();
  return true;
}
