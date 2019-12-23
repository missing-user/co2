#define sensorCount  8
#define samples 5
//to make the max values independet of the samples (10* old values)
#define factor ((float)10)/samples
#define delayTime 50

#include <dht.h>
#define DHT11_PIN 3

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 4

#include <SoftwareSerial.h>

SoftwareSerial mySerial(8, 7); // RX, TX
//mh-Z19 sensor stuff
byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
char response[9];

int prevVal = LOW;
long th, tl, h, l, ppm, ppm2 = 0.0;


dht DHT;
int lastDhtTemp, lastDhtHum;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float Celcius = 0;

unsigned long a[sensorCount];
int previous;

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  for (int j = 0; j < sensorCount; j++)
  {
    pinMode(14 + j, INPUT);
  }
  pinMode(2, OUTPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  previous = analogRead(14);
}

void loop() {

  mySerial.write(cmd, 9);
  mySerial.readBytes(response, 9);
  int responseHigh = (int) response[2];
  int responseLow = (int) response[3];
  ppm = (256 * responseHigh) + responseLow;

  for (int i = 0; i < sensorCount; i++)
  {
    a[i] = 0;
  }

  //sample analog
  for (int i = 0; i < samples; i++)
  {
    for (int j = 0; j < sensorCount; j++)
    {
      a[j] += analogRead(14 + j);
      delay(delayTime);
    }
  }

  for (int j = 0; j < sensorCount; j++)
  {
    a[j] = a[j] * factor;
    Serial.print(a[j]);
    Serial.print(",");
  }
  //a[sensors - 1] = a[sensors - 1] / samples;
  //Serial.print(a[sensors - 1]);
  //Serial.print(",");
  Serial.print(samples * 100);
  Serial.print(",");
  Serial.print(delayTime);
  Serial.print(",");
  temperature();
  Serial.print(ppm);
  Serial.println();

  if (a[0] > previous + 5)
    alarm();
  previous = a[0];
  delay(delayTime);
}

void alarm()
{
  for (int j = 0; j < 10; j++) {
    tone(2, 1980);
    delay(100);
    tone(2, 4500);
    delay(100);
    noTone(2);
    delay(50);
  }
}

void temperature()
{
  int fac = 10;
  //dht11
  int chk = DHT.read11(DHT11_PIN);
  if (chk > -2) {
    lastDhtTemp = DHT.temperature;
    lastDhtHum = DHT.humidity;
  }
  Serial.print(fac * lastDhtTemp);
  Serial.print(",");
  Serial.print(fac * lastDhtHum);
  Serial.print(",");

  //one wire
  sensors.requestTemperatures();
  Celcius = sensors.getTempCByIndex(0);
  Serial.print(fac * Celcius);
  Serial.print(",");
}

