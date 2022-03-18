#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

#define ACCEL_IRQPIN 25
unsigned long accelLastHit = 0;

bool accelInAlert = false;
bool accelPresent = true;
bool AccelIsInAlert()
{
  return accelInAlert;
}

void IRAM_ATTR  AccelMotion()
{
  accelLastHit = millis();
  accelInAlert = true;
}

bool AccelTimeOuted()
{
  unsigned long current = millis();
  return ( (current - accelLastHit)> (60000 * 5) );
}


void AccelResetEvent()
{
    accel.readRegister(0x30);
    accelInAlert = false;
}


bool SetupAccel()
{
  accelLastHit = millis();//fake to get 5 minutes of work
  pinMode(ACCEL_IRQPIN, INPUT);   
  pinMode(LEDINDICATOR, OUTPUT);   
  digitalWrite(LEDINDICATOR, LOW);
  
  if(!accel.begin())
  {
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    accelPresent = false;
    return false;
  }
  accel.setRange(ADXL345_RANGE_4_G);
  //set registers manualy, bc lib is not the best at it.
  accel.writeRegister(0x31, 0b00000001); //int invert, range
  accel.writeRegister(0x2d, 0b00001001); //0x2D—POWER_CTL
  accel.writeRegister(0x27, 0b11100000); //0x27—ACT_INACT_CTL
  accel.writeRegister(0x24,2); //THRESH_ACT
  accel.writeRegister(0x25, 1); //0x25—THRESH_INACT
  accel.writeRegister(0x2f, 0); //INT_MAP
  accel.writeRegister(0x2e, 16); //INT_ENABLE

  AccelResetEvent();//initial reset events
  attachInterrupt(digitalPinToInterrupt(ACCEL_IRQPIN), AccelMotion, RISING);
  return true;
}
