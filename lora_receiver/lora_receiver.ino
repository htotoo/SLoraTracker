
bool hasSometingToParse = false;
uint8_t packet[300];

#include "config.h"
#include <sproto.h>
#include <swifi.h>
#include "lora.h"
#include <smqtt.h>
#include <sprotoserver.h>

SProtoServer serverRegistry;
uint32_t lastMillisHit = 0;

String CreateDevUid(char* hostname, char* stationId, uint16_t measTypeId, uint8_t serial = 0, uint8_t timeFrame = 0)
{
  String devuid = String(hostname) + "-" + String(stationId) + "-" + SProto::GetDataTypeStr(measTypeId) + "-" + String(serial) + "-" + String(timeFrame);
  return devuid;
}

String CreateDiscoveryTopic(char* discoveryPrefix, uint16_t measTypeId, String devuid)
{
  String t = String(SProto::GetHADevStringByHaDevType( SProto::GetHADeviceTypeId(measTypeId) ) );
  String toTopic = String(discoveryPrefix) + "/"+t+"/" + devuid + "/config";
  return toTopic;
}

bool CreateHASensor(char* discoveryPrefix, char* hostname, char* stationId, char* mqttPrefix, uint16_t measTypeId, uint8_t serial, uint8_t timeFrame, uint32_t expireAfter = 180, char* unitom = NULL)
{
  if (!SMqtt::IsConnected()) return false;
  String stateTopic = SProto::GetMQTTDataTopic(hostname, stationId, mqttPrefix, measTypeId, serial, timeFrame);
  String devuid = CreateDevUid(hostname, stationId, measTypeId, serial, timeFrame);
  String uom;
  if (unitom == NULL) uom = String( SProto::GetDataTypeUnitStr(measTypeId) ); else uom = String(unitom);
  String payload = "{ \"state_topic\":\"" + stateTopic + "\", \"name\":\"" + devuid + "\", \"unique_id\":\"" + devuid + "\", \"unit_of_measurement\":\"" + uom + "\", \"expire_after\":" + String(expireAfter) + " }" ;
  String toTopic = CreateDiscoveryTopic(discoveryPrefix, measTypeId,  devuid);
  return SMqtt::SendMessage(toTopic.c_str(), payload.c_str(), true, 1);
}

bool CreateHAGPSDevTracker(char* discoveryPrefix, char* hostname, char* stationId,  char* mqttPrefix, uint16_t measTypeId, uint8_t serial)
{
  if (!SMqtt::IsConnected()) return false;
  String stateTopic = SProto::GetMQTTDataTopic(hostname, stationId, mqttPrefix, measTypeId, serial, 0);
  String attrTopic = SProto::GetMQTTAttributesTopic(hostname, stationId, mqttPrefix, measTypeId, serial, 0);
  String devuid = CreateDevUid(hostname, stationId, measTypeId, serial);
  String payload = "{ \"json_attributes_topic\":\"" + attrTopic + "\", \"state_topic\":\"" + stateTopic + "\", \"name\":\"" + devuid + "\", \"unique_id\":\"" + devuid + "\", \"source_type\":\"gps\" }" ;
  String toTopic = CreateDiscoveryTopic(discoveryPrefix, measTypeId,  devuid);
  SMqtt::SendMessage(toTopic.c_str(), payload.c_str(), true, 1);
  SMqtt::SetWill(stateTopic.c_str(), "not present", false, 0);
  return SMqtt::SendMessage(stateTopic.c_str(), "present", false, 0);
}


bool RegisterInHa(char* stationId, SPROTO_MEASHEADERSTRUCT * dataHead)
{
  uint8_t sensorType = SProto::GetHADeviceTypeId(dataHead->measTypeId);
  if (sensorType == SPROTO_MQTT_DEVTYPEID_SENSOR) return CreateHASensor(MQTTDICSOVERYPREFIX, HOSTNAME, stationId, MQTTPREFIX, dataHead->measTypeId, dataHead->serial, dataHead->timeFrame, 600);
  if (sensorType == SPROTO_MQTT_DEVTYPEID_DEVTRACKER) return CreateHAGPSDevTracker(MQTTDICSOVERYPREFIX, HOSTNAME, stationId, MQTTPREFIX, dataHead->measTypeId, dataHead->serial);
  return false;
}


void onMqttConnect()
{
}


bool SendGPSCoordInfo(char* stationId, uint8_t serial, SPM_GPSCOORD* gps)
{
  String stId = String(stationId);
  String attrTopic = SProto::GetMQTTAttributesTopic(HOSTNAME, stationId, MQTTPREFIX, SPROTO_MEASID_GPSDETAILED, serial, 0);
  String attrJson = "{ \"latitude\": " + String(gps->latitude, 7) + ",  \"longitude\": " + String(gps->longitude, 7) + ",  \"altitude\": " + String(gps->altitude, 1) + " }";
  return SMqtt::SendMessage(attrTopic.c_str(), attrJson.c_str(), false, 1);
}

bool SendGPSDetailedInfo(char* stationId, uint8_t serial, SPM_GPSDETAILED* gps)
{
  String attrTopic = SProto::GetMQTTAttributesTopic(HOSTNAME, stationId, MQTTPREFIX, SPROTO_MEASID_GPSDETAILED, serial, 0);
  String stateTopic = SProto::GetMQTTDataTopic(HOSTNAME, stationId, MQTTPREFIX, SPROTO_MEASID_GPSDETAILED, serial, 0);
  SMqtt::SendMessage(stateTopic.c_str(), "present", false, 1);
  String attrJson = "{ \"latitude\": " + String(gps->latitude, 7) + ",  \"longitude\": " + String(gps->longitude, 7) + ",  \"altitude\": " + String(gps->altitude, 1) + ", \"speed\":" + String(gps->speed, 1) + ", \"sats\": " + gps->satelittes + "  }";
  return SMqtt::SendMessage(attrTopic.c_str(), attrJson.c_str(), false, 1);
}

void UpdateDataRegistry(uint16_t addr, String& stId, SPROTO_MEASHEADERSTRUCT* dataHead)
{
    if (SMqtt::IsConnected() && serverRegistry.ArriveMeasInRegistry(addr, dataHead))
    {   //new device, should be registered in HA
      if (!RegisterInHa((char*)stId.c_str(), dataHead))
      {
        //todo remove from registry, to try to add it again
      }
    }
}

void ParseDataPacket()
{
  SProto::DecryptData(packet);
  uint32_t dl = *(uint32_t*)&packet[SPROTO_HEADER_POS_DATALENGTH];
  uint16_t addr = *(uint16_t*)&packet[SPROTO_HEADER_POS_SRCADDR];
  String stId = String(addr);
  uint32_t offset = SPROTO_HEADER_LENGTH;
  dl += SPROTO_HEADER_LENGTH; //last data byte
  SPROTO_MEASHEADERSTRUCT dataHead;
  while (offset < dl)
  {
    SProto::MeasParseDataPart(packet, offset, &dataHead);
    UpdateDataRegistry (addr, stId, &dataHead);
    String mqttTopic = SProto::GetMQTTDataTopic(HOSTNAME, stId.c_str(), MQTTPREFIX, dataHead.measTypeId, dataHead.serial, dataHead.timeFrame);
    if (dataHead.measTypeId == SPROTO_MEASID_TEMPERATURE)
    {
      SPM_Temperature tmp;
      SProto::MeasGetDataPart(packet, offset, &tmp);
      Serial.print("Temperature: "); Serial.println(tmp);
      SMqtt::SendMessage(mqttTopic.c_str(), String(tmp, 2).c_str());
    }
    if (dataHead.measTypeId == SPROTO_MEASID_PRESSURE)
    {
      SPM_Pressure tmp;
      SProto::MeasGetDataPart(packet, offset, &tmp);
      Serial.print("Pressure: "); Serial.println(tmp);
      SMqtt::SendMessage(mqttTopic.c_str(), String(tmp, 1).c_str());
    }
    if (dataHead.measTypeId == SPROTO_MEASID_HUMIDITY)
    {
      SPM_Humidity tmp;
      SProto::MeasGetDataPart(packet, offset, &tmp);
      Serial.print("Humidity: "); Serial.println(tmp / 10.0);
      SMqtt::SendMessage(mqttTopic.c_str(), String(tmp / 10.0, 1).c_str());
    }
    if (dataHead.measTypeId == SPROTO_MEASID_VOLTAGE)
    {
      SPM_Voltage tmp;
      SProto::MeasGetDataPart(packet, offset, &tmp);
      Serial.print("Voltage: "); Serial.println(tmp);
      SMqtt::SendMessage(mqttTopic.c_str(), String(tmp).c_str());
    }
    if (dataHead.measTypeId == SPROTO_MEASID_GPSCOORD)
    {
      SPM_GPSCOORD tmp;
      SProto::MeasGetDataPart(packet, offset, &tmp);
      Serial.print("GPS: Longitude: ");
      Serial.print(tmp.longitude, 6);
      Serial.print(", Latitude: ");
      Serial.print(tmp.latitude);
      Serial.print(", Altitude: ");
      Serial.println(tmp.altitude);
      SendGPSCoordInfo((char*)stId.c_str(), dataHead.serial, &tmp);
    }
    if (dataHead.measTypeId == SPROTO_MEASID_GPSDETAILED)
    {
      SPM_GPSDETAILED tmp;
      SProto::MeasGetDataPart(packet, offset, &tmp);
      Serial.print("GPS: Longitude: "); Serial.print(tmp.longitude, 6);
      Serial.print(", Latitude: ");
      Serial.print(tmp.latitude, 6);
      Serial.print(", Altitude: ");
      Serial.print(tmp.altitude);
      Serial.print(", Sats: ");
      Serial.print(tmp.satelittes);
      Serial.print(", Speed: ");
      Serial.println(tmp.speed, 2);
      SendGPSDetailedInfo((char*)stId.c_str(), dataHead.serial, &tmp);
    }
    offset += dataHead.dataSizeWithHeader;
  }
  //send yet manual rssi
  dataHead.measTypeId = SPROTO_MEASID_RSSI;
  dataHead.serial = 0;
  dataHead.timeFrame = SPROTO_TIME_INSTANTANEOUS;
  UpdateDataRegistry (addr, stId, &dataHead);
  String mqttTopic = SProto::GetMQTTDataTopic(HOSTNAME, stId.c_str(), MQTTPREFIX, SPROTO_MEASID_RSSI, 0, SPROTO_TIME_INSTANTANEOUS);
  SPM_Rssi tmprssi = loraRssi;
  SMqtt::SendMessage(mqttTopic.c_str(), String(tmprssi).c_str());
  //send manual snr
  dataHead.measTypeId = SPROTO_MEASID_SNR;
  UpdateDataRegistry (addr, stId, &dataHead);
  mqttTopic = SProto::GetMQTTDataTopic(HOSTNAME, stId.c_str(), MQTTPREFIX, SPROTO_MEASID_SNR, 0, SPROTO_TIME_INSTANTANEOUS);
  SPM_Snr tmpSnr= loraSnr;
  SMqtt::SendMessage(mqttTopic.c_str(), String(tmpSnr,2).c_str());
}

void ParseSProtoPacket()
{
  if (SProto::IsValidPacket(packet))
  {
    uint16_t cmd = SProto::GetCmdFromPacket(packet);
    switch(cmd)
    { 
      case SPROTO_CMD_CMEAS:
        ParseDataPacket();
        break;
      case SPROTO_CMD_SRESTART:
        ESP.restart();
        break;
      default:
        Serial.print("Got packet with cmd: ");
        Serial.println(cmd);
        break;
    }
  } else
  {
    Serial.println("Invalid packet received");
  }
}


void setup()
{
  Serial.begin(115200);
  SWifi::SetAP(HOSTNAME, WIFIPASS1);
  SWifi::AddWifi(WIFISSID1, WIFIPASS1);
  #ifdef WIFISSID2
    SWifi::AddWifi(WIFISSID2, WIFIPASS2);
  #endif
  SWifi::SetWifiMode(true, true);
  SWifi::SetHostname(HOSTNAME);
  SWifi::Connect();
  SWifi::InitOTA();

  SMqtt::SetParams(MQTTIP, MQTTPORT, MQTTUSER, MQTTPASS);
  SMqtt::Setup();
  SMqtt::SetOnConnectCallBack(onMqttConnect);
  delay(1000);
  SetupLora();

}

bool DeletedDeviceCallback(uint16_t stationId, uint16_t measTypeId, uint8_t serial, uint8_t timeFrame)
{
  //get config topic for HA, and delete it from HA registry too
  String stId = String(stationId);
  String devuid = CreateDevUid(HOSTNAME, (char*)stId.c_str(), measTypeId, serial, timeFrame);
  String toTopic = CreateDiscoveryTopic(MQTTDICSOVERYPREFIX, measTypeId,  (char*)devuid.c_str());
  return SMqtt::SendMessage(toTopic.c_str(), "", true, 1);  
}

void loop()
{
  delay(1);
  if (hasSometingToParse)
  {
    hasSometingToParse  = false;
    ParseSProtoPacket();
  }
  //maintain wifi + mqtt
  SWifi::Loop();
  SMqtt::Loop(SWifi::IsConnected()); //mqtt
  //timer
  if (millis() - lastMillisHit > 60000)
  {
    lastMillisHit = millis();
    serverRegistry.TickTime(1); //1 tick elpased
    serverRegistry.CollectExpired(60*24*3, DeletedDeviceCallback); //alive for 3 days
  }
}
