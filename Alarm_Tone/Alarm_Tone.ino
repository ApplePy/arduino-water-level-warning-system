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

int ledPin = 13;
int speakerOut = 9;               
byte names[] = {'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C'};  
int tones[] = {1915, 1700, 1519, 1432, 1275, 1136, 1014, 956};
byte melody[] = "9b6d9b6d9b6d9b6d";
// count length: 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
//                                10                  20                  30
int count = 0; //Iterates through the notes in the playlist
int count2 = 0; //The counter that iterates through the names, looking for a match with the requested note
int count3 = 0; //Controls how long a "beat" is, via its for loop
int MAX_COUNT = 8; //How many notes are in the melody
int statePin = LOW;

void setup() {
 pinMode(ledPin, OUTPUT);
 //Speaker is already set up by default as output
}

void loop() {
  analogWrite(speakerOut, 0); //Ensure speaker starts off 
  for (count = 0; count < MAX_COUNT; count++) { //Iterates through the notes requested
    statePin = !statePin;
    digitalWrite(ledPin, statePin);
    for (count3 = 0; count3 <= (melody[count*2] - 48) * 30; count3++) { //controls how long a note plays
      for (count2=0;count2<8;count2++) { //Iterate through tones
        if (names[count2] == melody[count*2 + 1]) { //if you find a tone that matches the tone requested       
          analogWrite(speakerOut,255); //pulse speaker accordingly (does not control volume well)
          delayMicroseconds(tones[count2]);
          analogWrite(speakerOut, 0);
          delayMicroseconds(tones[count2]); //there is a timing issue here where higher notes are played slightly shorter
        } 
        if (melody[count*2 + 1] == 'p') {
          // make a pause of a certain size
          analogWrite(speakerOut, 0);
          delayMicroseconds(500);
        }
      }
    }
  }
}

//Code from http://www.arduino.cc/en/Tutorial/PlayMelody
