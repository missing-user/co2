#define sensors  8
#define delayTime 500
unsigned long a[sensors];

void setup() {
  Serial.begin(115200);
  for (int j = 0; j < sensors; j++)
  {
    pinMode(14 + j, INPUT);
  }
  pinMode(5, OUTPUT);
}

void loop() {
  for (int j = 0; j < sensors; j++)
  {
    a[j] = analogRead(14 + j);
    delay(delayTime);
  }
  
  for (int j = 0; j < sensors; j++)
  {
    Serial.print(a[j]);
    if(j != sensors-1)
    Serial.print(",");
  }
  Serial.println();
  delay(delayTime);
}
