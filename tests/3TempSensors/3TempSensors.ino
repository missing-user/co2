#include <dht.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Encoder.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define DHT11_PIN 6
#define ONE_WIRE_BUS 11
#define BTN_PIN 12
#define TRIGGER_PIN 10
#define ENC1 8
#define ENC2 9

boolean text = false;
boolean overLimit = false;
//dht 11 variables
dht DHT;

//one wire sensor initialization
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float Celcius = 0;

//timer variables to avoid delays, interrupt capabilities are getting preserved
unsigned long previousMillis = 0;
long interval = 3000;
unsigned long previousMillisL = 0;
unsigned long intervalL = 500;
unsigned long prevFrameEnd = 0;

//rotary encoder variables
long oldPosition  = -999;                                                 //to check for changes
long newPosition = 0;                                                     //the actually relevant data
Encoder myEnc(ENC1, ENC2);
boolean recentlyChanged = false;
unsigned long prevChngMillis = 2000;
unsigned int multi = 10;


//mh z19 variables
byte com[] = {0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};      //command to start the reaading
byte returnMHZ[9];                                                        //the return bytes of the sensor
int concentrationMHZ;                                                     //bin to store the concentration value returned
int minMHZ;
int temperatureMHZ;
byte statuesMHZ;                                                          //the status byte, check to see if valid readings are available
SoftwareSerial softSerial(4, 3);                                          // RX, TX

void setup() {
  Serial.begin(115200);
  softSerial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  interval = constrain(EEPROMReadlong(123), 10, 3600000);   //get the saved data
  writeLeds(0);
  delay(200);
  writeLeds(500);
  delay(200);
  writeLeds(900);
  delay(200);
  writeLeds(2000);
  delay(200);
  writeLeds(900);
  delay(200);
  writeLeds(500);
  delay(200);
  writeLeds(0);
  delay(200);
  
  newPosition = myEnc.read();
  oldPosition = newPosition;
}


void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (text) {
      getAllReadings();
    }
    else {
      getAllValueReadings();
    }
    prevFrameEnd = currentMillis;
  }

  if (overLimit) {
    if (currentMillis - previousMillisL >= intervalL) {
      previousMillisL = currentMillis;
      if (analogRead(A3) < 10)
        analogWrite(A3, 348);
      else
        analogWrite(A3, 0);
    }
  }

  if (recentlyChanged) {
    if (currentMillis - prevChngMillis >= 2000) {
      prevChngMillis = millis();
      recentlyChanged = false;
      EEPROMWritelong(123, interval);
    }
  }
  digitalWrite(13, recentlyChanged);

  if (!digitalRead(BTN_PIN))
  {
    while (!digitalRead(BTN_PIN)) {}
    text = !text;
  }

  if (!digitalRead(TRIGGER_PIN))
  {
    while (!digitalRead(TRIGGER_PIN)) {}
    nextMulti();
  }

  newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    interval += (newPosition-oldPosition) * multi;
    interval = constrain(interval, 10, 3600000);
    oldPosition = newPosition;

    if (text) {
      Serial.print("interval \t");
      Serial.println(interval);
    } else {
      Serial.println(interval);
    }
    change();
  }

  if (Serial.available() > 0)
  {
    Serial.print(Serial.read() - 48);
  }
}

void getOneWireReadings()
{
  sensors.requestTemperatures();
  Celcius = sensors.getTempCByIndex(0);
  if (text) {
    Serial.println("one wire digitalSensor");
    Serial.print("temperature: \t\t");
    Serial.print(Celcius);
    Serial.println(" °C");
  }
  else {
    Serial.print(Celcius);
  }
}

void getDHT11Readings() {
  int chk = DHT.read11(DHT11_PIN);
  if (text) {

    Serial.println("DHT11");
    if (chk > -2) {
      Serial.print("temperature = \t\t");
      Serial.print(DHT.temperature);
      Serial.println(" °C");
      Serial.print("humidity = \t\t");
      Serial.print(DHT.humidity);
      Serial.println(" %");
    } else {
      Serial.print("error N°: ");
      Serial.println(chk);
    }
  }
  else {
    if (chk > -2) {
      Serial.print(DHT.temperature);
      Serial.print(",");
      Serial.print(DHT.humidity);
    }
    else {
      Serial.print(-1);
      Serial.print(",");
      Serial.print(-1);
    }
  }
}

void getMH19Readings() {
  while (softSerial.available() > 0) {
    softSerial.read();
  }

  softSerial.write(com, 9);
  softSerial.readBytes(returnMHZ, 9);
  concentrationMHZ = returnMHZ[2] * 256 + returnMHZ[3];
  temperatureMHZ = returnMHZ[4] - 40;
  minMHZ = returnMHZ[6] * 256 + returnMHZ[7];
  statuesMHZ = returnMHZ[5];

  if (text) {
    Serial.println("MH-Z19 ");
    Serial.print("CO2 concentration: \t");
    Serial.print(concentrationMHZ);
    Serial.println(" ppm");
    Serial.print("temperature: \t\t");
    Serial.print(temperatureMHZ);
    Serial.println(" °C");
  }
  else {
    Serial.print(concentrationMHZ);
    Serial.print(",");
    Serial.print(temperatureMHZ);
  }

  writeLeds(concentrationMHZ);
}

void getAllReadings() {
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println("----------------------------------------");
  getDHT11Readings();
  Serial.println("----------------------------------------");
  getOneWireReadings();
  Serial.println("----------------------------------------");
  getMH19Readings();
  Serial.println("----------------------------------------");
  Serial.print("MQ-135: \t\t");
  Serial.println(avganalogRead(A0));
  Serial.print("MQ-5: \t\t\t");
  Serial.println(avganalogRead(A1));
  Serial.print("MQ-9: \t\t\t");
  Serial.println(avganalogRead(A2));
  Serial.println("----------------------------------------");
  Serial.print("interval: ");
  Serial.println(interval);
  Serial.print("frame time: ");
  Serial.println(millis() - prevFrameEnd);
}

void getAllValueReadings() {

  getDHT11Readings();
  Serial.print(",");
  getOneWireReadings();
  Serial.print(",");
  getMH19Readings();
  Serial.print(",");
  Serial.print(avganalogRead(A0));
  Serial.print(",");
  Serial.print(avganalogRead(A1));
  Serial.print(",");
  Serial.print(avganalogRead(A2));
  Serial.print(",");
  Serial.print(interval);
  Serial.print(",");
  Serial.println(millis() - prevFrameEnd);
}

void change() {
  prevChngMillis = millis();
  recentlyChanged = true;
}

void nextMulti() {
  switch (multi) {
    case 1: multi = 10; blink(2); break;
    case 10: multi *= 10; blink(3); break;
    case 100: multi *= 100; blink(4); break;
    case 1000: multi *= 1000; blink(5); break;
    case 10000: multi = 1; blink (1); break;
  }
}

void blink(int b) {
  for (int i = 0; i < b; i++) {
    digitalWrite(13, HIGH); delay(100); digitalWrite(13, LOW); delay(100);
  }
}

void EEPROMWritelong(int address, long value)
{
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

void writeLeds(int v) {
  if (v > 460)
    analogWrite(A5, 440);
  else
    analogWrite(A5, 0);


  if (v > 800)
    analogWrite(A4, 389);
  else
    analogWrite(A4, 0);

  if (v > 1200)
    analogWrite(A3, 348);
  else
    analogWrite(A3, 0);

  overLimit = v > 1600;
}

long EEPROMReadlong(int address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

double avganalogRead(int pin){
  long sum = 0;
  for (int i =0; i<5; i++){
    sum+= analogRead(pin);
    delayMicroseconds(200);
  }
  return sum/5.0;
}

