/* Play Melody
 * -----------
 *
 * Program to play melodies stored in an array, it requires to know
 * about timing issues and about how to play tones.
 *
 * The calculation of the tones is made following the mathematical
 * operation:
 *
 *       timeHigh = 1/(2 * toneFrequency) = period / 2
 *
 * where the different tones are described as in the table:
 *
 * note         frequency       period  PW (timeHigh)
 * c            261 Hz          3830    1915
 * d            294 Hz          3400    1700
 * e            329 Hz          3038    1519
 * f            349 Hz          2864    1432
 * g            392 Hz          2550    1275
 * a            440 Hz          2272    1136
 * b            493 Hz          2028    1014
 * C            523 Hz          1912    956
 *
 * (cleft) 2005 D. Cuartielles for K3
 */

#include <SoftwareSerial.h>

//General-use global variables
enum mode {sensor, alarm};
unsigned long time = millis();
mode opMode = sensor;
bool reconfiguration();

//Sensor global variables
const int sensorPin = 2;

//LED global variables
const int ledPin = 13;
int statePin = LOW;
const int undefinedBlinkRate = 50;
const int sensorBlinkRate = 1000;
const int alarmBlinkRate = 100;
int blinkRate = sensorBlinkRate;

//Speaker global variables
const int speakerOut = 9;
const byte names[] = {'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C'};
const int tones[] = {1915, 1700, 1519, 1432, 1275, 1136, 1014, 956};
const byte melody[] = "3b3d3b3d"; //"3b3d3b3d3b3d3b3d";
// count length: 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
//                                10                  20                  30
bool alarmSounding = false;
void speaker();

//Bluetooth/Serial global variables
const int receivePin = 10;
const int transmitPin = 11;
const int btBaud = 9600;
const int serialBaud = 9600;
SoftwareSerial bluetooth(receivePin, transmitPin);

void setup() {

  //Setup LED
  pinMode(ledPin, OUTPUT);

  //Setup Sensor
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH); //Sets the pullup resistor

  //Speaker is already set up by default as output

  //Setup Bluetooth/serial
  //Even though they might not be used (based on mode), activate them anyways so that the
  //mode can be changed on-the-fly at runtime
  bluetooth.begin(btBaud);
  Serial.begin(serialBaud);
}

void loop() {
  //Sensor has been triggered or serial has been triggered
  //Communications dictionary: '1' = alarm, '5' = reconfiguration
  //Reconfiguration protocol: "5 *mode*" ('5' signals that the next command is a reconfiguration, then a space, then the mode (0=sensor, 1=alarm, 2=query))
  //Todo: include fluctuation tolerance (e.g. 10 activations required before alarm)
  //Todo: heartbeat communications
  //Todo: communications on the serial interfaces may have varying length. Account for it
  if (digitalRead (sensorPin) == LOW && opMode == sensor) {
    bluetooth.write('1');
  }
  if (Serial.available()) { //DO NOT allow mode reconfiguration via bluetooth. Security hazard.

    if (opMode == alarm) { //What if the Serial command was triggered to reconfigure? (CHECK WITH EUGEN THAT POWER AND USB COMBINED WON'T FRY THE BOARD!)
      char input = Serial.read();

      if (input == '5') {
        reconfiguration(); //Start reconfiguration code
      }
      else if (input == '1') { //alarm has been rung
        speaker();
      }

    }
    else if (opMode == sensor) {
      char input = Serial.read();

      if (input == '5') { //order of these conditions is important so compiler checks available first!
        reconfiguration(); //Start reconfiguration code
      }

    }
    else {
      //OPMODE BROKE! WARN!
      blinkRate = 0;
    }
  }
  //oscillate light
  if (millis() - time >= blinkRate) {
    statePin = !statePin;
    digitalWrite(ledPin, statePin);
    time = millis();
  }
}

bool reconfiguration() {
  Serial.println ("Starting reconfiguration...");
  bool retVal = false;
  unsigned int counter = 0;
  
  delay(10); //Necessary to make this work
  
  while (Serial.available() && counter < 2) {
    char input = Serial.read();
    
    if (input == ' ' && counter == 0) {
      //NULL, keep going
    }
    else if (input == '0' && counter == 1) {
      opMode = sensor;
      blinkRate = sensorBlinkRate;
      retVal = true;
      Serial.print("Now a sensor. Reconfiguration successful");
    }
    else if (input == '1' && counter == 1) {
      opMode = alarm;
      blinkRate = alarmBlinkRate;
      retVal = true;
      Serial.print("Now an alarm. Reconfiguration successful.");
    }
    else if (input == '2' && counter == 1) {
      
      if (opMode == sensor) {
        Serial.println("I'm a sensor.");
        retVal = true;
      }
      else if (opMode == alarm) {
        Serial.println ("I'm an alarm.");
        retVal = true;
      }
      
    }
    else {
      retVal = false;
      break;
    }

    counter++;
  }

  if (retVal == false) {
    Serial.println("Reconfiguration failure.");
  }
  return retVal;
}

void speaker() {
  alarmSounding = true;
  analogWrite(speakerOut, 0); //Ensure speaker starts off
  for (int count = 0; count < (sizeof(melody) - 1) / 2; count++) { //Iterates through the notes requested
    statePin = !statePin;
    digitalWrite(ledPin, statePin);
    for (int count2 = 0; count2 < 8; count2++) { //Iterate through tones
      if (names[count2] == melody[count * 2 + 1]) { //if you find a tone that matches the tone requested
        unsigned long startTime = millis();
        while (millis() - startTime <= (melody[count * 2] - 48) * 100) { //rudimentary way of spacing out the length of a tone, and keeps tones at the same length
          analogWrite(speakerOut, 255); //pulse speaker accordingly (does not control volume well)
          delayMicroseconds(tones[count2]);
          analogWrite(speakerOut, 0);
          delayMicroseconds(tones[count2]); //there is a timing issue here where higher notes are played slightly shorter
        }
      }
      if (melody[count * 2 + 1] == 'p') {
        // make a pause of a certain size
        unsigned long startTime = millis();
        while (millis() - startTime <= (melody[count * 2] - 48) * 30) {
          analogWrite(speakerOut, 0);
          delayMicroseconds(500);
        }
      }
    }
  }
  alarmSounding = false;
}

//Base code from http://www.arduino.cc/en/Tutorial/PlayMelody
