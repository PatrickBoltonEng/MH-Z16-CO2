/*
 * Project MH-Z16-CO2
 * Description: Seeed Studio MH-Z16 NDIR CO2 Sensor 0-5000ppm, 8-bit (0-255)
 *   Starting Point:  https://community.particle.io/t/problem-mh-z16-and-particle-software-serial/56664/2 by juansezoh & ScruffR
 * Author:  PJB
 * Date:  05-14-20 begin date
 * Tech info:  Typical geographic distribution of CO2 is 400 ppm.  Human exhales 4-5% (40-50,000 ppm)
 */

#include "Particle.h"
#include "JsonParserGeneratorRK.h"

SYSTEM_THREAD(ENABLED);

const unsigned char cmd_get_sensor[] = {0xff,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};

const unsigned char cmd_calibratezero[] = {0xff,0x01,0x87,0x00,0x00,0x00,0x00,0x00,0x78};

const unsigned char cmd_calibratespan[] = {0xff,0x01,0x88,0x07,0xD0,0x00,0x00,0x00,0xA0};

const uint32_t  READ_DELAY  = 15000;
static uint32_t msDelay     = millis();

SerialLogHandler logHandler(LOG_LEVEL_INFO);

int CO2, CO2TC, min_time, CO2cloud, CO2TCcloud;
int min_last = 1;

void setup()
{
  Particle.variable("CO2(ppm)", CO2cloud);
  Particle.variable("CO2Temp(C)", CO2TCcloud);
    
  Serial1.begin(9600,SERIAL_8N1);
  Serial1.setTimeout(500);                                                       // Wait up to 500ms for the data to return in full

  Serial.begin(9600);
  delay(1000);

  Log.info("Setup Complete");
  Log.info("System version: %s", (const char*)System.version());
  //Log.info("WiFi RSSI-----: %d", (int8_t) WiFi.RSSI());
  //Serial1.write(cmd_calibratezero, sizeof(cmd_calibratezero));
  //Serial1.write(cmd_calibratespan, sizeof(cmd_calibratespan));
}

void loop()
{
  if(millis() - msDelay > READ_DELAY)
  {
    if(GetCO2(CO2, CO2TC)) Log.info("CO2(ppm): %d, CO2-T(°C): %d", CO2, CO2TC);
    CO2cloud=CO2;
    CO2TCcloud=CO2TC;

    min_time=Time.minute();
    if((min_time!=min_last)&&(min_time==0||min_time==15||min_time==30||min_time==45))
    {
      createEventPayload(CO2cloud, CO2TCcloud);
      min_last = min_time;
      Log.info("Last Update: %d", min_last);
    }
    msDelay = millis();
  }
}

bool GetCO2(int& CO2, int& CO2TC)
{
  byte data[9];
  char chksum = 0x00;
  int len = -1, ntry=0, nmtry=10, i =0;
  while((len < 9 && ntry < nmtry))
    {
    Serial1.flush();                                                             // Flush TX buffer
    while(Serial1.read() >= 0);                                                  // Flush RX buffer
    Serial1.write(cmd_get_sensor, 9);
    for(uint32_t msTimeout=millis(); (len=Serial1.available())<9 && (millis()-msTimeout<1000); Particle.process());
    Log.trace("[MHZ] Available: %d", len);
    Log.trace("[MHZ] Num. try-: %d", ntry);
    ntry++; 
    }
  if (len != 9) return false;
  for(i = 0; i < len; i++) data[i] = Serial1.read();
  for(i = 1; i < 9; i++) chksum += data[i];
  if(chksum==0) CO2 = (int)data[2] * 256 + (int)data[3];
  if(chksum==0) CO2TC = (int)data[4] - 40;
  return true;
}

void createEventPayload(int &CO2cloud, int &CO2TCcloud)
{
  JsonWriterStatic<256> jw;
  {
    JsonWriterAutoObject obj(&jw);
    jw.insertKeyValue("CO2(ppm): %d", CO2cloud);
    jw.insertKeyValue("CO2-T(°C): %d", CO2TCcloud);
  }
  Particle.publish("COO-COO-CACHOO", jw.getBuffer(), PRIVATE);
}