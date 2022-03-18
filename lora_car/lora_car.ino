#include <SPI.h>
#include <Wire.h>  

#include <TinyGPS++.h>
TinyGPSPlus gps;
HardwareSerial GPS(1);

#define LEDINDICATOR 4

uint8_t packet[300];


#include "config.h"
#ifdef USEWIFI
  #include <swifi.h>
#else
  #include "WiFi.h"
#endif
#include <sproto.h>
#include "power.h"
#include "lora.h"
#include "accel.h"

void setup()
{
  Serial.begin(115200);  
  Wire.begin(21, 22);
  if (PowerInit()) {
    PowerUp();
  } else {
    Serial.println("Power error, can't init. Maybe bad board?");
    ESP.restart();
  }
  if (!SetupAccel())
  {
    Serial.println("Accel error");
  }
  
  GPS.begin(9600, SERIAL_8N1, 34, 12);

  delay(1000);
  SetupLora();
  LoraSendGps(); //init send for alert
#ifdef USEWIFI
  SWifi::SetAP(HOSTNAME, WIFIPASS1);
  SWifi::AddWifi(WIFISSID1, WIFIPASS1);
  #ifdef WIFISSID2
    SWifi::AddWifi(WIFISSID2, WIFIPASS2);
  #endif
  SWifi::SetWifiMode(true, true);
  SWifi::SetHostname(HOSTNAME);
  SWifi::Connect();
  SWifi::InitOTA();
#else
  WiFi.mode(WIFI_OFF);
#endif
  btStop();
}


void LoraSendGps()
{
  if (LoRa.beginPacket() == 1)
  {
    SProto::CreateHeader(packet, SPROTO_CMD_CMEAS, 0 , SPROTO_ENCRYPTION_XOR,SRCADDR,DSTADDR);
    SPM_Temperature temp = GetBattTemp();
    SPM_Voltage volt = GetBatteryVoltage();
    SPM_GPSDETAILED tmp;
    tmp.latitude = gps.location.lat();
    tmp.longitude = gps.location.lng();
    tmp.altitude = gps.altitude.feet() / 3.2808;
    tmp.satelittes = gps.satellites.value();
    tmp.speed = gps.speed.kmph();
    tmp.hdop = 0;
    uint32_t len = SProto::SimpleMeasurementAdd(packet, SPROTO_MEASID_GPSDETAILED, &tmp);
    len = SProto::SimpleMeasurementAdd(packet, SPROTO_MEASID_TEMPERATURE, &temp);
    len = SProto::SimpleMeasurementAdd(packet, SPROTO_MEASID_VOLTAGE, &volt);
    SProto::EncryptData(packet);
    LoRa.write(packet, len);
    LoRa.endPacket(true);
  }
  else
  {
    Serial.println("Can't send lora packet");
  }

}

void loop()
{
 if (gps.location.isUpdated()) 
 {
  LoraSendGps();
 }
 smartDelay(5000);

 if (AccelTimeOuted())
 {
    //manage poweroff
    detachInterrupt(digitalPinToInterrupt(ACCEL_IRQPIN)); //sleepcpu will attach it to wake me up
    PowerOff();
    SleepCPU();
 } 
 AccelResetEvent();
}


static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (GPS.available()) gps.encode(GPS.read());
    digitalWrite(LEDINDICATOR, !digitalRead(ACCEL_IRQPIN));
    if (AccelIsInAlert()) AccelResetEvent();
    #ifdef USEWIFI
      SWifi::Loop();
    #endif
  } while (millis() - start < ms);
}
