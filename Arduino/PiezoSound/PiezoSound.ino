#include "PiezoSound.h"

/************************************************************/
/*	Sound Generation routines								*/
/************************************************************/

/*
    Play melodies stored in an array, it requires you to know
      about timing issues and about how to play tones.

    The calculation of the tones is made following the mathematical
      operation:

      timeHigh = 1/(2 * toneFrequency) = period / 2

      where the different tones are described as in the table:

      note          frequency       period  PW (timeHigh)  
      ----          ---------       ------  -------------
      c             261 Hz          3830    1915    
      d             294 Hz          3400    1700    
      e             329 Hz          3038    1519    
      f             349 Hz          2864    1432    
      g             392 Hz          2550    1275    
      a             440 Hz          2272    1136    
      b             493 Hz          2028    1014   
      C             523 Hz          1912    956

      (cleft) 2005 D. Cuartielles for K3

	The original code, this routine is based on, was found at:
		http://www.arduino.cc/en/Tutorial/PlayMelody
*/
void playMelody(byte melody[]) {
  byte names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };  
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };

  int count = 0;
  int count2 = 0;
  int count3 = 0;

  int statePin = LOW;

  analogWrite(SPEAKER_OUT, 0);    

  for (count = 0; count < MAX_COUNT; count++) {
    statePin = !statePin;

    digitalWrite(HEARTBEAT_LED, statePin);

    for (count3 = 0; count3 <= (melody[count * 2] - 48) * 30; count3++) {
      for (count2=0; count2 < 8; count2++) {
        if (names[count2] == melody[count * 2 + 1]) {      
          analogWrite(SPEAKER_OUT, 500);
          delayMicroseconds(tones[count2]);
          analogWrite(SPEAKER_OUT, 0);
          delayMicroseconds(tones[count2]);
        }

        if (melody[count * 2 + 1] == 'p') {
          //  Make a pause of a certain size
          analogWrite(SPEAKER_OUT, 0);
          delayMicroseconds(500);
        }
      }
    }
  }
}
/*
	Play a single tone on a Piezo buzzer
*/
uint16_t playTone(uint8_t tone, uint8_t volume, uint8_t durationMS) {
	uint16_t errorStatus = 0;

	return errorStatus;
}

/*
	Play a sound - a sequence of pitches (or notes)
*/
uint16_t makeSound (uint8_t soundNr, uint8_t nrTimes, uint16_t durationMS) {
	uint16_t errorStatus = 0;
	uint8_t pitch = NOTE_C7;
	uint8_t volume = 100;
	uint8_t lengthMS;
	uint8_t count = 0;

	for (count = 0; count < nrTimes; count++) {
		switch (soundNr) {
			case 1:
				playTone(pitch, volume, lengthMS);
				delay(150);
				playTone(pitch, volume, lengthMS);
				break;

			default:
				errorStatus = 901;
				processError(errorStatus, F("Invalid sound number"));
				break;
		}

		delay(durationMS);
	}

	return errorStatus;
}

/*
	Sound an alarm when we need assistance
*/
uint16_t soundAlarm (uint8_t count) {
	uint16_t errorStatus = 0;
	uint8_t alarmCount;

	for (alarmCount = 0; alarmCount < count; alarmCount++) {
		makeSound(1, count, 250);
	}

	return errorStatus;
}

/*
	Call for help!

	We're in a situation we can't get out of on our own.
*/
uint16_t callForHelp (void) {
	uint16_t errorStatus = 0;
	uint8_t count;

	//	Send out a call for help every 20 seconds
	for (count = 0; count < 20; count++) {
		console.println(F("Help, help, help! I am stuck!"));
		xbee.println(F("Help, help, help! I am stuck!"));

		soundAlarm(5);

		//	20 second delay between calls for help
		delay(20000);
	}

	return errorStatus;
}
