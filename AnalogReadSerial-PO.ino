/*
  AnalogReadSerial

  Reads an analog input on pin 0, prints the result to the Serial Monitor.
  Graphical representation is available using Serial Plotter (Tools > Serial Plotter menu).
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogReadSerial
*/

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}


int minV = 1023;
int maxV = 0;
// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);
  // print out the value you read:
  if (sensorValue > maxV) {
    maxV= sensorValue;
  }
  if (sensorValue > 20) {
  if (sensorValue < minV) {
    minV = sensorValue;
  }
  // 191 233, w/ 1k 157-190, w/10k 172-238
    Serial.println(sensorValue + String(" ") + minV + String(" ") + maxV);
    delay(10);        // delay in between reads for stability
  }
  else {
    delay(1);
  }
}
