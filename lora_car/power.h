
#include <axp20x.h>
AXP20X_Class axp;

bool PowerInit()
{
   return (!axp.begin(Wire, AXP192_SLAVE_ADDRESS));
}

void PowerUp()
{
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  axp.setLDO2Voltage(3300); 
}


void PowerOff()
{
  WiFi.mode(WIFI_OFF);
  btStop();
  axp.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_OFF);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_OFF);
}

void SleepCPU()
{
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 1);
  esp_sleep_enable_timer_wakeup(3600000000*3);
  esp_deep_sleep_start();
}


float GetBatteryVoltage()
{
  return axp.getBattVoltage();
}

float GetBattTemp()
{
  return axp.getTemp();
}

void PowerDebug()
{
  
    float temp = GetBattTemp();
    Serial.print(temp);    Serial.println("*C");
    
    int cur = axp.getChargeControlCur();
    Serial.printf("Current charge control current = %d mA \n", cur);

    Serial.print("BATTERY STATUS: ");
   if (axp.isBatteryConnect()) {
        Serial.println("CONNECT");
        Serial.print("BAT Volate:");        Serial.print(GetBatteryVoltage());        Serial.println(" mV");
        if (axp.isChargeing()) {
            Serial.print("Charge:");
            Serial.print(axp.getBattChargeCurrent());
            Serial.println(" mA");
        } else {
            Serial.print("Discharge:");
            Serial.print(axp.getBattDischargeCurrent());
            Serial.println(" mA");
        }
    } else {
        Serial.println("DISCONNECT");
    }
    Serial.println();
}
