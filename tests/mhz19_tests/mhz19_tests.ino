#include <SoftwareSerial.h>
SoftwareSerial co2Serial(8, 7); // define MH-Z19 RX TX
unsigned long startTime = millis();

#include <dht.h>
#define DHT11_PIN 3

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 4


dht DHT;
int lastDhtTemp, lastDhtHum;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float Celcius = 0;
 
void setup() {
  Serial.begin(115200);
  co2Serial.begin(9600);
  pinMode(9, INPUT);
}
 
void loop() {
  Serial.println("------------------------------");
  Serial.print("Time from start: ");
  Serial.print((millis() - startTime) / 1000);
  Serial.println(" s");
  int ppm_uart = readCO2UART();
  int ppm_pwm = readCO2PWM();
  temperature();
  delay(5000);
}

int readCO2UART(){
  byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
  byte response[9]; // for answer

  Serial.println("Sending CO2 request...");
  co2Serial.write(cmd, 9); //request PPM CO2

  // clear the buffer
  memset(response, 0, 9);
  int i = 0;
  while (co2Serial.available() == 0) {
//    Serial.print("Waiting for response ");
//    Serial.print(i);
//    Serial.println(" s");
    delay(1000);
    i++;
  }
  if (co2Serial.available() > 0) {
      co2Serial.readBytes(response, 9);
  }
  // print out the response in hexa
  for (int i = 0; i < 9; i++) {
    Serial.print(String(response[i], HEX));
    Serial.print("   ");
  }
  Serial.println("");

  // checksum
  byte check = getCheckSum(response);
  if (response[8] != check) {
    Serial.println("Checksum NOT OK!");
    Serial.print("Received: ");
    Serial.println(response[8]);
    Serial.print("Should be: ");
    Serial.println(check);
  }
  
  // ppm
  int ppm_uart = 256 * (int)response[2] + response[3];
  Serial.print("PPM UART: ");
  Serial.println(ppm_uart);

  // temp
  byte temp = response[4] - 40;
  Serial.print("Temperature? \t\t\t\t");
  Serial.println(temp);

  // status
  byte status = response[5];
  Serial.print("Status? ");
  Serial.println(status); 
  if (status == 0x40) {
    Serial.println("Status OK"); 
  }
  
  return ppm_uart;
}

byte getCheckSum(char *packet) {
  byte i;
  unsigned char checksum = 0;
  for (i = 1; i < 8; i++) {
    checksum += packet[i];
  }
  checksum = 0xff - checksum;
  checksum += 1;
  return checksum;
}

int readCO2PWM() {
  unsigned long th, tl, ppm_pwm = 0;
  do {
    th = pulseIn(9, HIGH, 1004000) / 1000;
    tl = 1004 - th;
    ppm_pwm = 5000 * (th-2)/(th+tl-4);
  } while (th == 0);
  Serial.print("PPM PWM: ");
  Serial.println(ppm_pwm);
  return ppm_pwm;  
}

void temperature()
{
  int fac = 1;
  //dht11
  int chk = DHT.read11(DHT11_PIN);
  if (chk > -2) {
    lastDhtTemp = DHT.temperature;
    lastDhtHum = DHT.humidity;
  }
  Serial.print("dht 11 temperature: \t\t\t");
  Serial.println(fac * lastDhtTemp);
  Serial.print("dht 11 humidity:    ");
  Serial.println(fac * lastDhtHum);

  //one wire
  sensors.requestTemperatures();
  Celcius = sensors.getTempCByIndex(0);
  Serial.print("one wire sensor temperature: \t\t");
  Serial.println(fac * Celcius);
}
