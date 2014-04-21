/*
	Program:      	SES Rover, Main.ino - Main sketch
	Date:         	21-Apr-2014
	Version:      	0.2.2 ALPHA

	Platform:		Arduino Mega 2560 R3,
						Lynxmotion's SSC-32 Servo Controller,
						and a 3DOF (Raise/Lower, Wrist Rotate, Open/Close) Little Grip gripper

	Purpose:		To experiment with various sensor configurations, tracking objects (heat or
						color based), course following, manipulation of the environment, and to
						test code that will later be used on W.A.L.T.E.R. 2.0.

					Change Log
					-------------------------------------------------------------------------------
					v0.0.1 ALPHA 19-Feb-2014:
					Initial build from W.A.L.T.E.R. 2.0 code
					-------------------------------------------------------------------------------
					v0.0.2 ALPHA 20-Feb-2014:
					Adding routines to scan the area and move the robot.

					Rewrote the ServoMotor struct to use naming like the Servo struct.
					-------------------------------------------------------------------------------
					v0.0.3 ALPHA 17-Apr-2014:
					Switched processor to the Arduino Mega 2560 R3 board, because there is not enough
						memory on a BotBoarduino (Arduino Duemilanove) for what I need to do.

					Making adjustments to take advantage of the extra hardware serial ports.
					-------------------------------------------------------------------------------
					v0.1.0 ALPHA 18-Apr-2014 (MAJOR RELEASE):
					More error checking in the setup() routine.

					Added some error checking to scanArea().

					Added the displayAreaScanReadings() routine to display all the readings from the
						areaScan array.

					Moved the code to read the color and heat sensors into separate routines.

					Added boolean switch settings HAVE_10DOF_IMU, HAVE_COLOR_SENSOR and HAVE_HEAT_SENSOR
						to the header file to allow turning this sensor code on and off easily.

					Added the displayIMUData() and readIMU() for the Adafruit 10DOF IMU

					Added conditional initialization for all optional sensors.

					Added the readGP2Y0A21YK0F() routine to read the Sharp GP2Y0A21YK0F IR sensor

					Changed the name of the readIR() routine to readGP2D12().

					Changed the name of the readPING() routine to readParallaxPING().
					-------------------------------------------------------------------------------
					v0.2.0 19-Apr-2014 (MAJOR RELEASE):
					Rearranged the sensor initialization code in initSensors() to allow setting whether
						these sensors are available or not on the robot.

					Added definintions in the header files to enable/disable the various sensors.

					Added the 'name' field to the Servo, ServoMotor, and Motor structs.

					Split the readIMU() routine into three additional routines - readLSM303DLHC(),
						readL3GD20Gyro(), and readBMP180() because each of these sensors can be purchased
						from Adafruit separately.

					Split the displayIMUData() into three separate display routines for the same reason.

					Added separate structs for each sensor: bmp180Data, l3gd20Data, and lsm303dlhcData

					Changed the readIMU() and displayIMUData() routines to call the respective sensor
						reading and display routines.

					Created the findDistanceObjects() routine and DistanceObject struct.

					Added display of distance objects in displayAreaScanReadings().
					-------------------------------------------------------------------------------
					v0.2.1 20-Apr-2014:
					Fixed a bug in the scanArea() routine. The index variable needed to be an int,
						because it can be negative. It was a uint16_6. The scanner works now!

					Added a 10 second delay before initialization to allow getting set to shoot
						videos or do other things before the robot moves.

					Starting to pull code I've written from other files, for stuff like displays and
						displaying data.
					-------------------------------------------------------------------------------
					v0.2.2 21-Apr-2014:
					Pulling in more initialization code from other files.

					Added display code for date and time.

					Added the calculatePitchRoll() routine, and modified the readIMU() routine to use it.

					Adde noise (sound) making routines playTone(), makeSound(), and soundAlarm()
					-------------------------------------------------------------------------------

	Dependencies:	Adafruit libraries:
						Adafruit_Sensor, Adafruit_L3GD20, Adafruit_TMP006, and Adafruit_TCS34725 libraries

					Hybotics libraries:
						Hybotics_10DOF_Unified (forked from the Adafruit_10DOF library)
						Hybotics_LSM303DLHC_Unified (forked from the Adafruit_LSM303 library)

					Other libraries:
						RTClib for the DS1307 (Adafruit's version)

	Comments:		Credit is given, where applicable, for code I did not originate.
*/
#include <Wire.h>

#include <BMSerial.h>
#include <RoboClaw.h>

#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_L3GD20.h>
#include <Hybotics_LSM303DLHC_Unified.h>
#include <Hybotics_BMP180_Unified.h>
#include <Hybotics_10DOF_Unified.h>
#include <RTClib.h>

/*
	Additional sensors
*/
#include <Adafruit_TMP006.h>
#include <Adafruit_TCS34725.h>

/*
  Local includes
*/
#include "Main.h"
#include "Pitches.h"

/********************************************************************/
/*	Bitmaps for the drawBitMap() routines 							*/
/********************************************************************/

static const uint8_t PROGMEM
	hpa_bmp[] = {
		B10001110,
		B10001001,
		B11101110,
		B10101000,
		B00000100,
		B00001010,
		B00011111,
		B00010001
	},

	c_bmp[] = {
		B01110000,
		B10001000,
		B10000000,
		B10001000,
		B01110000,
		B00000000,
		B00000000,
		B00000000
	},

	f_bmp[] = {
		B11111000,
		B10000000,
		B11100000,
		B10000000,
		B10000000,
		B00000000,
		B00000000,
		B00000000
	},

	m_bmp[] = {
		B00000000,
		B00000000,
		B00000000,
		B00000000,
		B11101110,
		B10111010,
		B10010010,
		B10000010
	},

	date_bmp[] = {
		B10110110,
		B01001001,
		B01001001,
		B00000100,
		B00000100,
		B01111100,
		B10000100,
		B01111100
	},

	year_bmp[] = {
		B00000000,
		B10001000,
		B10001000,
		B01110000,
		B00101011,
		B00101100,
		B00101000,
		B00000000
	},

	am_bmp[] = {
		B01110000,
		B10001010,
		B10001010,
		B01110100,
		B00110110,
		B01001001,
		B01001001,
		B01001001
	},

	pm_bmp[] = {
		B01111100,
		B10000010,
		B11111100,
		B10000000,
		B10110110,
		B01001001,
		B01001001,
		B01001001
	},

	allon_bmp[] = {
		B11111111,
		B11111111,
		B11111111,
		B11111111,
		B11111111,
		B11111111,
		B11111111,
		B11111111
	};

/************************************************************/
/*	Initialize global variables								*/
/************************************************************/

/*
	These variables control the display of various information
		on the seven segment and matrix displays.
*/

//	SSC-32 command variable
String ssc32Command = "";

//	Date display
boolean displayDate = true;
uint8_t dateMinuteCount = 0;

//	Time display
boolean displayTime = true;
uint8_t timeMinuteCount = 0;

//	Temperature display
boolean displayTemperature = true;
uint8_t temperatureMinuteCount = 0;

/*
	Time control variables
*/
uint8_t currentMinute = 0;
uint8_t lastMinute = -1;
long minuteCount = 0;						//	Count the time, in minutes, since we were last restarted

//	Enable run once only loop code to run
bool firstLoop = true;

//	True when the robot has not moved after an area scan
bool hasNotMoved = true;

//	This will always have the name of the last routine executed before an error
String lastRoutine;

/************************************************************/
/*	Initialize Objects										*/
/************************************************************/

Adafruit_L3GD20 gyroscope;
Hybotics_BMP180_Unified temperature = Hybotics_BMP180_Unified(10001);
Hybotics_LSM303DLHC_Accel_Unified accelerometer = Hybotics_LSM303DLHC_Accel_Unified(10002);
Hybotics_LSM303DLHC_Mag_Unified compass = Hybotics_LSM303DLHC_Mag_Unified(10003);
Hybotics_10DOF_Unified imu = Hybotics_10DOF_Unified();

Adafruit_TCS34725 rgb = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
Adafruit_TMP006 heat = Adafruit_TMP006();

RTC_DS1307 clock;

//	Support for multiple 7 segment displays
Adafruit_7segment sevenSeg[MAX_NUMBER_7SEG_DISPLAYS];

Adafruit_8x8matrix matrix8x8 = Adafruit_8x8matrix();

/*
	Setup all our serial devices
*/

//	Hardware Serial0: Console and debug (replaces Serial.* routines)
BMSerial console(SERIAL_CONSOLE_RX_PIN, SERIAL_CONSOLE_TX_PIN);

//	Hardware Serial1: Lynxmotion SSC-32 Servo Controller
BMSerial ssc32(SERIAL_SSC32_RX_PIN, SERIAL_SSC32_TX_PIN);

//	Hardware Serial2: XBee Mesh Wireless
BMSerial xbee(SERIAL_XBEE_RX_PIN, SERIAL_XBEE_TX_PIN);

//	Hardware Serial3: RESERVED
BMSerial reserved(SERIAL_RESERVED_RX_PIN, SERIAL_RESERVED_TX_PIN);

//	Software Serial: RoboClaw 2x5 Motor Controller
RoboClaw roboClaw(SERIAL_ROBOCLAW_RX_PIN, SERIAL_ROBOCLAW_TX_PIN, 10000, false);

//	We only have one RoboClaw 2x5 right now
uint8_t roboClawControllers = ROBOCLAW_CONTROLLERS - 1;
uint8_t	roboClawBaseAddress = ROBOCLAW_SERIAL_BASE_ADDR;
uint8_t roboClawAddress1 = ROBOCLAW_SERIAL_BASE_ADDR;
uint8_t roboClawAddress2 = ROBOCLAW_SERIAL_BASE_ADDR + 1;

/************************************************************/
/*	Initialize Motors, Servos, and Servo Motors						*/
/************************************************************/

//	These are real DC motors
Motor leftM1, rightM2;

//	Standard R/C servos
Servo gripLift, gripWrist, gripGrab, pan, tilt;

//	Continuous rotation servo motors
ServoMotor leftMotorM1 = {
	SERVO_MOTOR_LEFT_PIN,
	SERVO_MOTOR_LEFT_NAME,
	SERVO_MOTOR_LEFT_OFFSET,
	SERVO_MOTOR_LEFT_DIRECTION,
	SERVO_MOTOR_LEFT_NEUTRAL,
	SERVO_MOTOR_LEFT_MIN,
	SERVO_MOTOR_LEFT_MAX,
	0,
	0
};

ServoMotor rightMotorM2 = {
	SERVO_MOTOR_RIGHT_PIN,
	SERVO_MOTOR_RIGHT_NAME,
	SERVO_MOTOR_RIGHT_OFFSET,
	SERVO_MOTOR_RIGHT_DIRECTION,
	SERVO_MOTOR_RIGHT_NEUTRAL,
	SERVO_MOTOR_RIGHT_MIN,
	SERVO_MOTOR_RIGHT_MAX,
	0,
	0
};

//	Total number of area readings taken, or -1 if data is not valid
int nrAreaReadings;

//	PING Ultrasonic range sensor readings
int ping[MAX_NUMBER_PING];

//	Sharp GP2Y0A21YK0F IR range sensor readings
float ir[MAX_NUMBER_IR];

//	Area scan readings
AreaScanReading areaScan[MAX_NUMBER_AREA_READINGS];
bool areaScanValid = false;

/****************************************************************************/
/*	Seven Segment and 8x8 Matrix Display routines							*/
/****************************************************************************/
 
/*
    Write a floating point value to the 7-Segment display, such as the 0.56"
      4 digit displays with I2C backpacks, sold by Adafruit.

    Multiple 7 segment displays are supported automatically. You just have to
      set the number of displays in the IMU_Multi_Display.h and set the proper
      I2C addresses for the displays. The base address is 0x70, as shipped by
      Adafruit. Up to 8 of these displays are supported, with the 
      being
      highest addressed display farthest to the left, and decreasing addresses
      moving to the right. The lowest addressed (0x70) display has to be at the
      far right for this to work.
*/

/*
	Write a number (integer or floating point) to a 7-Segment display
*/
void writeNumber (uint8_t displayNr, uint16_t value, uint8_t decimal = 2, bool noblank = false) {
	uint8_t digitCount = 1, temp = 0;
	bool decimalPoint = false;

	temp = value / 100;
/*  
	console.print(F*"(writeNumber) value = "));
	console.print(value);
	console.print(F(", temp = "));
	console.println(temp);
*/

  //	Set first digit of the integer portion
	if ((noblank) or (temp > 9)) {
/*    
    console.print(F("(writeNumber) digit = "));
    console.println(digit);
*/

		decimalPoint = ((digitCount) == decimal);
		sevenSeg[displayNr].writeDigitNum(0, int(temp / 10), decimalPoint);  //  Tens
  } else {
	    sevenSeg[displayNr].clear();
  }

	//	Set the second digit of the integer portion
	digitCount += 1;
	decimalPoint = ((digitCount) == decimal);
	sevenSeg[displayNr].writeDigitNum(1, temp % 10, decimalPoint);         //  Ones

	//	Set the first digit of the decimal portion
	temp = value % 100;
	digitCount += 1;
	decimalPoint = ((digitCount) == decimal);
	sevenSeg[displayNr].writeDigitNum(3, int(temp / 10), decimalPoint);    //  Tens

	//	Set the second digit of the decimal portion
	digitCount += 1;
	decimalPoint = ((digitCount) == decimal);
	sevenSeg[displayNr].writeDigitNum(4, temp % 10, decimalPoint);         //  Ones
}

/*
	Clear all the seven segment and matrix displays
*/
void clearDisplays (void) {
	uint8_t nrDisp = 0;

	while (nrDisp < MAX_NUMBER_7SEG_DISPLAYS) {
		sevenSeg[nrDisp].clear();
		sevenSeg[nrDisp].drawColon(false);
		sevenSeg[nrDisp].writeDisplay();

		nrDisp += 1;
	}

	matrix8x8.clear();
	matrix8x8.writeDisplay();
}

/************************************************************/
/*	Utility routines										*/
/************************************************************/

/*
    Left zero pad a numeric string
*/
String leftZeroPadString (String st, uint8_t nrPlaces) {
  uint8_t i, len;
  String newStr = st;

  lastRoutine = String(F("leftZeroPadString"));
  
  if (newStr.length() < nrPlaces) {
    len = st.length();
  
    for (i = len; i < nrPlaces; i++) {
      newStr = String("0" + newStr);
    }
  }

  return newStr;
}

/*
    Convert a pulse width in ms to inches
*/
long microsecondsToInches (long microseconds) {
	/*
		According to Parallax's datasheet for the PING))), there are
			73.746 microseconds per inch (i.e. sound travels at 1130 feet per
			second).  This gives the distance travelled by the ping, outbound
			and return, so we divide by 2 to get the distance of the obstacle.
		See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
	*/

	lastRoutine = String(F("microsecondsToInches"));
	
	return microseconds / 74 / 2;
}

/*
    Convert a pulse width in ms to a distance in cm
*/
long microsecondsToCentimeters (long microseconds) {
	/*
		The speed of sound is 340 m/s or 29 microseconds per centimeter.

		The ping travels out and back, so to find the distance of the
			object we take half of the distance travelled.
	*/

	lastRoutine = String(F("microsecondsToCentimeters"));

	return microseconds / 29 / 2;
}

/*
    Pulses a digital pin for a duration in ms
*/
void pulseDigital(int pin, int duration) {
	lastRoutine = String(F("pulseDigital"));

	digitalWrite(pin, HIGH);			// Turn the ON by making the voltage HIGH (5V)
	delay(duration);					// Wait for duration ms
	digitalWrite(pin, LOW);				// Turn the pin OFF by making the voltage LOW (0V)
	delay(duration);					// Wait for duration ms
}

/*
	Convert a temperature in Celsius to Fahrenheit
*/
float toFahrenheit (float celsius) {
	lastRoutine = String(F("toFahrenheit"));

	return (celsius * 1.8) + 32;
}

/*
    Trim trailing zeros from a numeric string
*/
String trimTrailingZeros (String st) {
  uint8_t newStrLen = 0;
  String newStr = st;

  lastRoutine = String(F("trimTrailingZeros"));

  newStrLen = newStr.length();

  while (newStr.substring(newStrLen - 1) == "0") {
    newStrLen -= 1;
    newStr = newStr.substring(0, newStrLen);
  }

  return newStr;
}

/*
    Process error conditions
*/
void processError (byte errCode, String errMsg) {
	console.print(F("Error in routine '"));
	console.print(lastRoutine);
	console.print(F("', Code: "));
	console.print(errCode);
	console.print(F(", Message: "));
	console.print(errMsg);
	console.println(F("!"));
}

/*
	Wait for a bit to allow time to read the Console Serial Monitor log
*/
void wait (uint8_t nrSeconds, String text = "") {
	uint8_t count;

	lastRoutine = String(F("wait"));

	console.print(F("Waiting"));

	if (text != "") {
		console.print(F(" for "));
		console.print(text);
	}

	for (count = 0; count < nrSeconds; count++) {
		console.print(F("."));
		delay(1000);
	}

	console.println();
}

/*
	Test all the displays
*/
void testDisplays (uint8_t totalDisplays) {
	uint8_t nrDisp = 0;

	console.println(F("Testing All Displays"));

	while (nrDisp < totalDisplays) {
		sevenSeg[nrDisp].print(8888);
		sevenSeg[nrDisp].drawColon(true);
		sevenSeg[nrDisp].writeDisplay();

		nrDisp += 1;
	}

	matrix8x8.drawBitmap(0, 0, allon_bmp, 8, 8, LED_ON);
	matrix8x8.writeDisplay();

	delay(2000);

	clearDisplays();
}

/************************************************************/
/*	Miscellaneous Robot Routines							*/
/************************************************************/

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

/************************************************************/
/*	Display routines										*/
/************************************************************/

void displayAreaScanReadings (DistanceObject *distObj) {
	uint8_t index;
	AreaScanReading currentReading;

	lastRoutine = String(F("displayAreaScanReadings"));

	console.println(F("AreaScan Readings:"));
	console.println();

	for (index = 0; index < MAX_NUMBER_AREA_READINGS; index++) {
		currentReading = areaScan[index];

		console.print(F("Index: "));
		console.print(index);
		console.print(F(", Position: "));
		console.print(currentReading.positionDeg);
		console.print(F(" degrees, PING: "));
		console.print(currentReading.ping);
		console.print(F(" cm, IR: "));
		console.print(currentReading.ir);
		console.println(F(" cm."));
		console.println();

		console.print(F("Closest object:"));
		console.print(F("     PING: "));
		console.print(distObj->closestPING);
		console.print(F(", at position "));
		console.print(distObj->closestPosPING);
		console.println(F(" degrees."));

		console.print(F("     IR: "));
		console.print(distObj->closestIR);
		console.print(F(", at position "));
		console.print(distObj->closestPosIR);
		console.println(F(" degrees."));

		console.print(F("Farthest object:"));
		console.print(F("     PING: "));
		console.print(distObj->farthestPING);
		console.print(F(", at position "));
		console.print(distObj->farthestPosPING);
		console.println(F(" degrees."));

		console.print(F("     IR: "));
		console.print(distObj->farthestIR);
		console.print(F(", at position "));
		console.print(distObj->farthestPosIR);
		console.println(F(" degrees."));

		if (HAVE_COLOR_SENSOR) {
			console.println();
			displayColorSensorData(&currentReading.color);
		}

		if (HAVE_HEAT_SENSOR) {
			console.println();
			displayHeatSensorData(&currentReading.heat);
		}

		console.println();
	}
}

/*
	Display the TCS34725 RGB color sensor readings
*/
void displayColorSensorData (ColorSensor *colorData) {
	lastRoutine = String(F("displayColorSensorData"));

	console.println(F("Color Sensor Data:"));
	console.println();

	console.print(F("Color Temperature: "));
	console.print(colorData->colorTempC, DEC);
	console.print(F(" K - Lux: "));
	console.print(colorData->lux, DEC);
	console.print(F(" - Red: "));
	console.print(colorData->red, DEC);
	console.print(F(" Green: "));
	console.print(colorData->green, DEC);
	console.print(F(" Blue: "));
	console.print(colorData->blue, DEC);
	console.print(F(" C: "));
	console.println(colorData->c, DEC);
	console.println(F("."));
}

/*
	Display the TMP006 heat sensor readings
*/
void displayHeatSensorData (HeatSensor *heatData) {
	lastRoutine = String(F("displayHeatSensorData"));

	float objCelsius = heatData->objectTempC;
	float objFahrenheit = toFahrenheit(objCelsius);
	float dieCelsius = heatData->dieTempC;
	float dieFahrenheit = toFahrenheit(dieCelsius);

	console.println(F("Heat Sensor Data:"));
	console.println();

	console.print(F("Object Temperature: "));
	console.print(objFahrenheit);
	console.print(F(" F, "));
	console.print(objCelsius);
	console.println(F(" C."));
	console.print(F("Die Temperature: "));
	console.print(dieFahrenheit);
	console.print(F(" F, "));
	console.print(dieCelsius);
	console.println(F(" C."));
}

/*
	Display the LSM303DLHC Accelerometer/Magnetometer (Compass) data
*/
void displayAccelCompassData (lsm303dlhcData *acData) {

	//	LMS303DLHC Accelerometer readings
	console.println(F("Accelerometer Readings: X = "));
	console.print(acData->accelX);
	console.print(F(", Y = "));
	console.print(acData->accelY);
	console.print(F(", Z = "));
	console.println(acData->accelZ);
	console.println();

	//	LMS303DLHC Magnetometer (Compass) readings
	console.println(F("Magnetometer (Compass) Readings: X = "));
	console.print(acData->compassX);
	console.print(F(", Y = "));
	console.print(acData->compassY);
	console.print(F(", Z = "));
	console.println(acData->compassZ);
	console.println();
}

/*
	Display the L3GD20 Gyroscope readings
*/
void displayGyroData (l3gd20Data *gyroData) {
	console.println(F("Gyroscope Readings: X = "));
	console.print(gyroData->X);
	console.print(F(", Y = "));
	console.print(gyroData->Y);
	console.print(F(", Z = "));
	console.println(gyroData->Z);
	console.println();
}

/*
	Display the BMP180 Temperature / Pressure Data
*/
void displayTemperatureData (bmp180Data *tempData) {
	if (tempData->temperatureValid) {
		console.print(F("Room Temperature = "));
		console.print(tempData->fahrenheit);
		console.print(F(" F, "));
		console.print(tempData->celsius);
		console.println(F(" C."));
		console.println();
	} else {
		console.println(F("Temperature data is not valid."));
	}
}

/*
	Display the readings from the IMU (Accelerometer, Magnetometer [Compass], Gyroscope,
		and Orientation (if valid)
*/
void displayIMUData (InertialMeasurementUnit *imuData) {
	sensors_vec_t orientation = imuData->orientation;
	lsm303dlhcData accelCompassData = imuData->accelCompassData;
	bmp180Data tempData = imuData->tempData;
	l3gd20Data gyroData = imuData->gyroData;

	//	LMS303DLHC Accelerometer readings
	displayAccelCompassData(&accelCompassData);

	//	L3GD20 Gyroscope readings
	displayGyroData(&gyroData);

	//	BMP180 Temperater and Pressure readings
	displayTemperatureData(&tempData);

	//	Orientation (Pitch and Roll) data
	if (imuData->pitchRollValid || imuData->headingValid) {
		console.println(F("Orientation Readings:"));
	}
	
	//	Orientation readings - Pitch, Roll, and Heading
	if (imuData->pitchRollValid) {
		console.print(F("Roll: "));
		console.print(orientation.roll);
		console.print(F("; "));
		console.print(F("Pitch: "));
		console.print(orientation.pitch);
	}

	if (imuData->headingValid) {
		if (imuData->pitchRollValid) {
			console.print(F("; "));
		}

		console.print(F("Heading: "));
		console.print(orientation.heading);
		console.println(F(" degrees."));
	}

	console.println();
}

/*
    Display the GP2Y0A21YK0F IR sensor readings (cm)
*/
void displayIR (void) {
	uint8_t sensorNr = 0;

	lastRoutine = String("displayIR");
  
	console.println(F("IR Sensor readings:"));

	while (sensorNr < MAX_NUMBER_IR) {
		console.print(F("IR #"));
		console.print(sensorNr + 1);
		console.print(F(" range = "));
		console.print(ir[sensorNr]);
		console.println(F(" cm"));

		sensorNr += 1;
	}

	console.println();
}

/*
	Display the readings from the PING Ultrasonic sensors
*/
void displayPING (void) {
	uint8_t sensorNr = 0;

	lastRoutine = String(F("displayPING"));
  
	console.println("PING Ultrasonic Sensor readings:");
  
	//	Display PING sensor readings (cm)
	while (sensorNr < MAX_NUMBER_PING) {
		console.print("Ping #");
		console.print(sensorNr + 1);
		console.print(" range = ");
		console.print(ping[sensorNr]);
		console.println(" cm");

		sensorNr += 1;
	}
 
	console.println();
}

/*
	Display the data for a given motor
*/
void displayMotor (Motor *motor) {
	console.print(motor->descr);
	console.println(F(" Motor:"));

	//	Using Packet Serial
	console.print(F("Encoder is valid: "));

	if (motor->encoderValid) {
		console.print(F("Yes, Status: "));
		console.print(motor->encoderStatus);
		console.print(F(", Value: "));
		console.println(motor->encoder);
	} else {
		console.println(F("No"));
	}

	console.print(F("Speed is valid: "));

	if (motor->speedValid) {
		console.print(F("Yes, Status: "));
		console.print(motor->speedStatus);
		console.print(F(", Speed: "));
		console.println(motor->mspeed);
	} else {
		console.println(F("No"));
	}

	console.print(F("Moving "));

	if (motor->forward) {
		console.print(F("Forward"));
	} else {
		console.print(F("Reverse"));
	}

	console.print(F("Distance is valid: "));

	if (motor->distanceValid) {
		console.print(F("Yes, Distance: "));
		console.println(motor->distance);
	} else {
		console.println(F("No"));
	}
}

/*
	Display data from a RoboClaw 2x5 motor controller
*/
void displayRoboClawData (uint8_t address, Motor *leftMotorM1, Motor *rightMotorM2) {
	char version[32];

	roboClaw.ReadVersion(address, version);

	console.print(F("RoboClaw 2x5 status (version "));
	console.print(version);
	console.print(F("): "));
	console.println();

    if (leftMotorM1->encoderValid) {
		console.print(F("Left Motor Encoder = "));
		console.print(leftMotorM1->encoder, DEC);
		console.print(F(", Status =  "));
		console.print(leftMotorM1->encoderStatus, HEX);
		console.println();
	}

	if (leftMotorM1->speedValid) {
		console.print(F("Left Motor Speed = "));
		console.print(leftMotorM1->mspeed, DEC);
		console.println();
	}

	if (rightMotorM2->encoderValid) {
		console.print(F("Right Motor Encoder = "));
		console.print(rightMotorM2->encoder, DEC);
		console.print(F(", Status = "));
		console.print(rightMotorM2->encoderStatus, HEX);
		console.println();
	}

	if (rightMotorM2->speedValid) {
		console.print(F("Right Motor Speed = "));
		console.print(rightMotorM2->mspeed, DEC);
		console.println();
	}
	
	console.println();
}

/*
	Display servo data
*/
void displayServo (Servo *servo) {
	console.println("Servo '" + servo->descr + "':");

	console.print(F("Pin #"));
	console.print(servo->pin);
	console.print(F(", Offset: "));
	console.print(servo->offset);
	console.print(F(", Home: "));
	console.print(servo->homePos);
	console.print(F(" uS, Pulse: "));
	console.print(servo->msPulse);
	console.println(F(" uS."));

	console.print(F("Angle: "));
	console.print(servo->angle);
	console.print(F(" deg, Min Pulse: "));
	console.print(servo->minPulse);
	console.print(F(" uS, Max Pulse: "));
	console.print(servo->maxPulse);
	console.print(F(" uS, Max Degrees: "));
	console.print(servo->maxDegrees);
	console.print(F(" deg, Error: "));
	console.print(servo->error);
	console.println(F("."));

	console.println();
}

/************************************************************/
/*	Sensor reading routines									*/
/************************************************************/

void calculatePitchRoll (InertialMeasurementUnit *imuData, sensors_event_t *accelEvent, sensors_event_t *compassEvent, sensors_vec_t *orientation) {
	//	Calculate pitch and roll from the raw accelerometer data
	imuData->pitchRollValid = imu.accelGetOrientation(accelEvent, orientation);

	//	Calculate the heading using the raw magnetometer (compass)
	imuData->headingValid = imu.magGetOrientation(SENSOR_AXIS_Z, compassEvent, orientation);
}

ColorSensor readColorSensor (void) {
	ColorSensor colorData;

	rgb.getRawData(&colorData.red, &colorData.green, &colorData.blue, &colorData.c);
	colorData.colorTempC = rgb.calculateColorTemperature(colorData.red, colorData.green, colorData.blue);
	colorData.lux = rgb.calculateLux(colorData.red, colorData.green, colorData.blue);

	return colorData;
}

HeatSensor readHeatSensor (void) {
	HeatSensor heatData;

	heatData.dieTempC = heat.readDieTempC();
	heatData.objectTempC = heat.readObjTempC();

	return heatData;
}

/*
	Read the BMP180 Temperature / Pressure sensor

	Calculating altitude with reasonable accuracy requires pressure
		sea level pressure for your position at the moment the data is
		converted, as well as the ambient temperature in degress
		celcius.  If you don't have these values, a 'generic' value of
		1013.25 hPa can be used (defined as SENSORS_PRESSURE_SEALEVELHPA
		in sensors.h), but this isn't ideal and will give variable
		results from one day to the next.

	You can usually find the current SLP value by looking at weather
		websites or from environmental information centers near any major
		airport.

	For example, for Paris, France you can check the current mean
		pressure and sea level at: http://bit.ly/16Au8ol
*/
bmp180Data readBMP180Temp (void) {
	bmp180Data tempData;

	//	Get temperature and pressure data
	temperature.getEvent(&tempData.tempEvent);
	tempData.seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
	tempData.temperatureValid = tempData.tempEvent.pressure;

	if (tempData.temperatureValid) {
		temperature.getTemperature(&tempData.celsius);
		tempData.fahrenheit = toFahrenheit(tempData.celsius);
	}

	tempData.altitude = temperature.pressureToAltitude(tempData.seaLevelPressure, tempData.tempEvent.pressure, tempData.celsius); 

	return tempData;
}

/*
	Read the L3GD20 Gyroscope (also part of the 10DOF IMU)
*/
l3gd20Data readL3GD20Gyro (void) {
	l3gd20Data gyroData;

	gyroscope.read();

	gyroData.X = (int)gyroscope.data.x;
	gyroData.Y = (int)gyroscope.data.y;
	gyroData.Z = (int)gyroscope.data.z;

	return gyroData;
}

/*
	Read the LSM303DLHC Accelerometer/Magnetometer (also part of the 10DOF IMU)
*/
lsm303dlhcData readLSM303DLHCAccelMag (void) {
	lsm303dlhcData acData;

	//	Get accelerometer readings
//	console.println(F("Getting Accelerometer Readings.."));
	accelerometer.getEvent(&acData.accelEvent);
 
	acData.accelX = acData.accelEvent.acceleration.x;
	acData.accelY = acData.accelEvent.acceleration.y;
	acData.accelZ = acData.accelEvent.acceleration.z;

	//	Get compass readings
//	console.println(F("Getting Magnetometer (Compass) Readings.."));
	compass.getEvent(&acData.compassEvent);

	acData.compassX = acData.compassEvent.magnetic.x;
	acData.compassY = acData.compassEvent.magnetic.y;
	acData.compassZ = acData.compassEvent.magnetic.z;

	return acData;
}

/*
	Read the 10DOF Inertial Measurement Unit (IMU)
*/
InertialMeasurementUnit readIMU (void) {
	InertialMeasurementUnit imuData;

	imuData.accelCompassData = readLSM303DLHCAccelMag();
	imuData.gyroData = readL3GD20Gyro();
	imuData.tempData = readBMP180Temp();

//	console.println(F("Getting Orientation readings.."));

	/*
		Get pitch, roll, and heading information
	*/

	//	Calculate pitch and roll from the raw accelerometer data
	imuData.pitchRollValid = imu.accelGetOrientation(&imuData.accelCompassData.accelEvent, &imuData.orientation);

	//	Calculate the heading using the raw magnetometer (compass)
	imuData.headingValid = imu.magGetOrientation(SENSOR_AXIS_Z, &imuData.accelCompassData.compassEvent, &imuData.orientation);

	return imuData;
}

/*
	Read distance in cm from a Sharp GP2Y0A21YK0F IR sensor
*/
float readSharpGP2Y0A21YK0F (byte sensorNr) {
	byte pin = sensorNr + IR_PIN_BASE;

	float volts = analogRead(pin) * 0.0048828125;
	float distance = 65 * pow(volts, -1.10);

	lastRoutine = String(F("readSharpGP2Y0A21YK0F"));

	return distance;
}

/* 
	Function to read a value from a GP2D12 infrared distance sensor and return a
		distance value in centimeters.

	This sensor should be used with a refresh rate of 36ms or greater.

	Javier Valencia 2008

	float readGP2D12(byte pin)

	It can return -1 if something has gone wrong.

	TODO: Make several readings over a time period, and average them
		for the final reading.
*/
float readSharpGP2D12 (byte sensorNr) {
	byte pin = sensorNr + IR_PIN_BASE;
	int tmp;

	lastRoutine = String(F("readSharpGP2D12"));

	tmp = analogRead(pin);

	if (tmp < 3) {
		return -1.0;								// Invalid value
	} else {
		return (6787.0 /((float)tmp - 3.0)) - 4.0;	// Distance in cm
	}
}

/*
	Ping))) Sensor 

	This routine reads a PING))) ultrasonic rangefinder and returns the
		distance to the closest object in range. To do this, it sends a pulse
		to the sensor to initiate a reading, then listens for a pulse
		to return.  The length of the returning pulse is proportional to
		the distance of the object from the sensor.

	The circuit:
		* +V connection of the PING))) attached to +5V
		* GND connection of the PING))) attached to ground
		* SIG connection of the PING))) attached to digital pin 7

	http://www.arduino.cc/en/Tutorial/Ping

	Created 3 Nov 2008
		by David A. Mellis

	Modified 30-Aug-2011
		by Tom Igoe

	Modified 09-Aug-2013
		by Dale Weber

		Set units = true for cm, and false for inches
*/
int readParallaxPING (byte sensorNr, boolean units=true) {
	byte pin = sensorNr + PING_PIN_BASE;
	long duration;
	int result;

	lastRoutine = String(F("readParallaxPING"));

	/*
		The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
		Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
	*/
	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);
	delayMicroseconds(2);
	digitalWrite(pin, HIGH);
	delayMicroseconds(5);
	digitalWrite(pin, LOW);

	/*
		The same pin is used to read the signal from the PING))): a HIGH
		pulse whose duration is the time (in microseconds) from the sending
		of the ping to the reception of its echo off of an object.
	*/
	pinMode(pin, INPUT);
	duration = pulseIn(pin, HIGH);

	//  Convert the duration into a distance
	if (units) {
		//	Return result in cm
		result = microsecondsToCentimeters(duration);
	} else {
		//  Return result in inches.
		result = microsecondsToInches(duration);
	}
 
	delay(100);
  
	return result;
}

/*
	Read current data from the RoboClaw 2x5 Motor Controller
*/
uint16_t readRoboClawData (uint8_t address, Motor *leftM1, Motor *rightM2) {
	bool valid;
	uint8_t status;

	//	Error control
	uint16_t errorStatus;
	String errorMsg;

	errorStatus = 0;

	console.println(F("Reading Left Motor Encoder.."));

	leftM1->encoder = roboClaw.ReadEncM1(address, &status, &valid);
	leftM1->encoderStatus = status;
	leftM1->encoderValid = valid;

	console.println(F("Reading Left Motor Speed.."));

	leftM1->mspeed = roboClaw.ReadSpeedM1(address, &status, &valid);
	leftM1->speedStatus = status;
	leftM1->speedValid = valid;

	console.println(F("Reading Right Motor Encoder.."));

	rightM2->encoder = roboClaw.ReadEncM2(address, &status, &valid);
	rightM2->encoderStatus = status;
	rightM2->encoderValid = valid;

	console.println(F("Reading Right Motor Speed.."));

	rightM2->mspeed = roboClaw.ReadSpeedM2(address, &status, &valid);
	rightM2->speedStatus = status;
	rightM2->speedValid = valid;

	return errorStatus;
}

/********************************************************************/
/*	Lynxmotion SSC-32 Servo Controller routines						*/
/********************************************************************/

DistanceObject findDistanceObjects () {
	uint8_t readingNr;

	DistanceObject distObj = {0, 0, 0, 0, 0, 0, 0, 0};

	console.println(F("Finding the closest and farthest objects.."));

	//	Find the closest and farthest objects
	for (readingNr = 0; readingNr <= nrAreaReadings; readingNr++) {
		//	Check for the closest object
		if (areaScan[readingNr].ping < areaScan[distObj.closestPING].ping) {
			distObj.closestPING = readingNr;
			distObj.closestPosPING = areaScan[readingNr].positionDeg;
		}

		if (areaScan[readingNr].ir <=  areaScan[distObj.closestIR].ir) {
			distObj.closestIR = readingNr;
			distObj.closestPosIR = areaScan[readingNr].positionDeg;
		}

		//	Check for the farthest object
		if (areaScan[readingNr].ping > areaScan[distObj.farthestPING].ping) {
			distObj.farthestPING = readingNr;
			distObj.farthestPosPING = areaScan[readingNr].positionDeg;
		}

		if (areaScan[readingNr].ir > areaScan[distObj.farthestIR].ir) {
			distObj.farthestIR = readingNr;
			distObj.farthestPosIR = areaScan[readingNr].positionDeg;
		}
	}

	return distObj;
}

/*
    Move a servo by pulse width in ms (500ms - 2500ms) - Modified to use HardwareSerial2()
*/
uint16_t moveServoPw (Servo *servo, int servoPosition, boolean term, int moveSpeed = 0, int moveTime = 0) {
	uint16_t errorStatus = 0;
	char asciiCR = 13;

	lastRoutine = String(F("moveServoPw"));

	servo->error = 0;
  
	if ((servoPosition >= servo->minPulse) && (servoPosition <= servo->maxPulse)) {
		ssc32Command = ssc32Command + "#" + String(servo->pin) + "P" + String(servoPosition + servo->offset) + " ";

		servo->msPulse = servoPosition;
		servo->angle = ((servoPosition - SERVO_CENTER_MS) / 10);
    
		if (servo->maxDegrees == 180) {
			servo->angle += 90;
		}
	}

	if ((servoPosition < servo->minPulse) || (servoPosition > servo->maxPulse)) {
		errorStatus = 201;
		processError(errorStatus, F("Servo pulse is out of range"));
	} else {
		//  Add servo move speed
		if (moveSpeed != 0) {
			ssc32Command = ssc32Command + " S" + String(moveSpeed) + " ";
		}
    
		//  Terminate the command
		if (term == true) {
			if (moveTime != 0) {
				ssc32Command = ssc32Command + " T" + String(moveTime) + String(" ");
			}

			ssc32Command = ssc32Command + asciiCR;

			ssc32.print(ssc32Command);
			ssc32.println();

			ssc32Command = "";
		}
  	}

	if (errorStatus != 0) {
		servo->error = errorStatus;
	}

  	return errorStatus;
}

/*
    Move a servo by degrees (-90 to 90) or (0 - 180) - Modified to use BMSerial
*/
uint16_t moveServoDegrees (Servo *servo, int servoDegrees, boolean term, int moveSpeed = 0, int moveTime = 0) {
	uint16_t servoPulse;

	uint16_t errorStatus = 0;
	String errorMsg;

	lastRoutine = String(F("moveServoDegrees"));

	servo->error = 0;
  
	//  Convert degrees to ms for the servo move
	if (servo->maxDegrees == 90) {
		servoPulse = SERVO_CENTER_MS + (servoDegrees * 11);
	} else if (servo->maxDegrees == 180) {
		servoPulse = SERVO_CENTER_MS + ((servoDegrees -90) * 11);
	} else {
		errorStatus = 202;
	}

	if (errorStatus != 0) {
		processError(errorStatus, F("Servo position (degrees) is invalid"));
	} else {
		if ((servoPulse >= servo->minPulse) && (servoPulse <= servo->maxPulse)) {
			errorStatus = moveServoPw(servo, servoPulse, true);

			if (errorStatus != 0) {
				processError(errorStatus, "Could not move the " + servo->descr + " servo");
			}
		} else {
			errorStatus = 201;
			processError(errorStatus, F("Servo pulse is out of range"));
		}
	}

	return errorStatus;
}

/*
	Scan an arc of up to 180 degrees, and take sensor readings at each angle increment
*/
uint16_t scanArea (Servo *pan, int startDeg, int stopDeg, int incrDeg) {
	uint16_t errorStatus = 0;
	uint16_t readingNr = 0, nrReadings = 0;
	int positionDeg = 0;
	int totalRangeDeg = 0;

	lastRoutine = String(F("scanArea"));

	//	Check the parameters
	if (startDeg > stopDeg) {
		//	Start can't be greater than stop
		errorStatus = 401;
	} else if (((pan->maxDegrees == 90) && ((startDeg < -90) || (stopDeg > 90))) || ((pan->maxDegrees == 180) && ((startDeg < 0) || (stopDeg > 180)))) {
		//	One or more parameters is outside of the valid range
		errorStatus = 402;
	} else if ((startDeg < pan->minPulse) || (stopDeg > pan->maxPulse)) {
		//	Out of range for the pan servo
		errorStatus = 403;
	} else {
		//	Calculate the total range, in degrees
		totalRangeDeg = abs(stopDeg - startDeg);

		//	Calculate the number of readings we need room for
		nrReadings = totalRangeDeg / incrDeg;

		//	More error checking
		if (totalRangeDeg > 180) {
			//	Servos can only move up to 180 degrees
			errorStatus = 404;
		} else if (nrReadings > MAX_NUMBER_AREA_READINGS) {
			//	We can't take this many readings
			errorStatus = 405;
		} else if (incrDeg > totalRangeDeg) {
			//	Increment is too large for our range
			errorStatus = 406;
		} else {
			/*
				Continue normal processing
			*/

			//	Stop, so we can do this scan
			errorStatus = stopMotors();

			if (errorStatus != 0) {
				runAwayRobot(errorStatus);
			} else {
				readingNr = 0;

				console.println(F("Scanning the area.."));

				for (positionDeg = startDeg; positionDeg < stopDeg; positionDeg += incrDeg) {
					errorStatus = moveServoDegrees(pan, positionDeg, true);

					if (errorStatus != 0) {
						processError(errorStatus, "Could not move the " + pan->descr + " servo");
						break;
					} else {
						//	Delay to let the pan/tilt stabilize after moving it
						delay(1500);

						//	Take a reading from each pan/tilt sensor in cm
						areaScan[readingNr].ping = readParallaxPING(PING_FRONT_CENTER, true);
						areaScan[readingNr].ir = readSharpGP2Y0A21YK0F(IR_FRONT_CENTER);
						areaScan[readingNr].positionDeg = positionDeg;

						if (HAVE_COLOR_SENSOR) {
							areaScan[readingNr].color = readColorSensor();
						}

						if (HAVE_HEAT_SENSOR) {
							areaScan[readingNr].heat = readHeatSensor();
						}

						readingNr += 1;
					}
				}

				//	Send the pan servo back to home position
				errorStatus = moveServoPw(pan, pan->homePos, true);

				if (errorStatus != 0) {
					processError(errorStatus, "Could not move the " + pan->descr + " servo");
				}
			}
		}
	}

	if (errorStatus != 0) {
		processError(errorStatus, F("Could not complete the area scan"));
		nrAreaReadings = -1;
		areaScanValid = false;
	} else {
		//	Robot has not moved
		hasNotMoved = true;

		//	Set the number of readings taken
		nrAreaReadings = readingNr;

		//	This area scan is valid
		areaScanValid = true;
	}

	return errorStatus;
}

/*
	Set the motor speed
*/
uint16_t setMotorSpeed (ServoMotor *servoMotor, int spd, bool term) {
	uint16_t errorStatus = 0;
	uint16_t pulse = SERVO_CENTER_MS;
	int motorSpeed = spd;

	lastRoutine = String(F("setMotorSpeed"));

	if ((spd < SERVO_MOTOR_MIN_SPEED) || (spd > SERVO_MOTOR_MAX_SPEED)) {
		errorStatus = 501;
		processError(errorStatus, F("Speed is out of range"));
	} else {
		Servo servo;

		//	Setup for the call to moveServoPw()
		servo.pin = servoMotor->pin;
		servo.descr = String(servoMotor->descr);
		servo.offset = servoMotor->offset;
		servo.homePos = servoMotor->neutral;
		servo.minPulse = servoMotor->minPulse;
		servo.maxPulse = servoMotor->maxPulse;
		servo.error = 0;

		if (servoMotor->forward == false) {
			motorSpeed *= -1;
		}

		pulse += motorSpeed;
		servo.msPulse = pulse;

		//	Set the motor's speed
		errorStatus = moveServoPw(&servo, pulse, term);

		if (errorStatus != 0) {
			servoMotor->error = servo.error;
			processError(errorStatus, "Could not set the " + servoMotor->descr + " motor speed");
		} else {
			hasNotMoved = false;
		}
	}

	return errorStatus;
}

/*
	Stop both motors NOW
*/
uint16_t stopMotors (void) {
	uint16_t errorStatus = 0;

	lastRoutine = String(F("stopMotors"));
	console.println(F("Stopping the motors.."));

	errorStatus = setMotorSpeed(&leftMotorM1, 0, false);

	if (errorStatus != 0) {
		processError(errorStatus, "Could not set the speed for the " + leftMotorM1.descr + " motor");
	} else {
		errorStatus = setMotorSpeed(&rightMotorM2, 0, true);

		if (errorStatus != 0) {
			processError(errorStatus, "Could not set the speed for the " + rightMotorM2.descr + " motor");
		} else {
			hasNotMoved = true;
		}
	}

	return errorStatus;
}

/*
	Turn towards the farthest detected object
*/
uint16_t turnToFarthestObject (DistanceObject *distObj) {
	uint16_t errorStatus = 0;

	if (distObj->farthestPosPING < 0) {
		//	Turn to the right
		errorStatus = setMotorSpeed(&leftMotorM1, 100, false);

		if (errorStatus == 0) {
			errorStatus = setMotorSpeed(&rightMotorM2, -100, true);
			delay(1000);
		}

		if (errorStatus == 0) {
			//	Start moving forward again
			errorStatus = setMotorSpeed(&rightMotorM2, 100, true);
		}

		if (errorStatus != 0) {
			processError(errorStatus, F("There was a problem turning RIGHT"));
		}
	} else if (distObj->farthestPosPING > 0) {
		//	Turn to the left
		errorStatus = setMotorSpeed(&leftMotorM1, -100, false);

		if (errorStatus == 0) {
			errorStatus = setMotorSpeed(&rightMotorM2, 100, true);
			delay(1000);
		}

		if (errorStatus == 0) {
			//	Start moving forward again
			errorStatus = setMotorSpeed(&leftMotorM1, 100, true);
		}

		if (errorStatus != 0) {
			processError(errorStatus, F("There was a problem turning LEFT"));
		}
	} else {
		//	Backup and scan again
		errorStatus = setMotorSpeed(&leftMotorM1, -100, false);

		if (errorStatus == 0) {
			errorStatus = setMotorSpeed(&rightMotorM2, -100, true);
			delay(1000);
		} else {
			processError(errorStatus, F("There was a problem backing up"));
		}

		if (errorStatus == 0) {
			errorStatus = stopMotors();

			if (errorStatus != 0) {
				runAwayRobot(errorStatus);
			} else {
				errorStatus = scanArea(&pan, -90, 90, 10);

				if (errorStatus != 0) {
					processError(errorStatus, F("There was a problem with the area scan"));
				}
			}
		}
	}

	return errorStatus;
}

/*
	We can't stop the motors!
*/
void runAwayRobot (uint16_t errorStatus) {
	processError(errorStatus, F("Runaway robot"));
	xbee.println(F("Runaway robot!"));

	callForHelp();
}

/********************************************************/
/*	Initialization routines								*/
/********************************************************/

uint16_t initGripper (Servo *lift, Servo *wrist, Servo *grab) {
	uint16_t errorStatus = 0;

	lastRoutine = String(F("initGripper"));

	console.println("Initializing Gripper Position..");
  
	//  Put the 3DOF gripper at home position
	errorStatus = moveServoPw(lift, lift->homePos, false);

	if (errorStatus != 0) {
		processError(errorStatus, "Could not initialize the " + grab->descr + " servo");
	} else {
		errorStatus = moveServoPw(wrist, wrist->homePos, false);

		if (errorStatus != 0) {
			processError(errorStatus, "Could not initialize the " + wrist->descr + " servo");
		} else {
			errorStatus = moveServoPw(grab, grab->homePos, true);

			if (errorStatus != 0) {
				processError(errorStatus, "Could not initialize the " + grab->descr + " servo");
			}
		}
	}

	return errorStatus;
}

/*
	Initialize displays

	Multiple 7 segment displays will be supported. The displays
		should be on the breadboard, starting at the right with
		the lowest addressed display and going to the left.

*/
void initDisplays (uint8_t totalDisplays) {
	uint8_t nrDisp = 0;
	uint8_t address;

	console.println(F("Initializing Displays.."));

	while (nrDisp < totalDisplays) {
		sevenSeg[nrDisp] = Adafruit_7segment();
		address = SEVEN_SEG_BASE_ADDR + nrDisp;
		sevenSeg[nrDisp].begin(address);
		sevenSeg[nrDisp].setBrightness(5);
		sevenSeg[nrDisp].drawColon(false);

		nrDisp += 1;
	}

	/*
		The matrix display address is one higher than the last
			seven segment display, based on the number of seven
			seven segment displays that are configured.
	*/
	matrix8x8.begin(MATRIX_DISPLAY_ADDR);
	matrix8x8.setBrightness(5);
	matrix8x8.setRotation(3);
}

/*
	Set the Pan/Tilt to Home Position
*/
uint16_t initPanTilt (Servo *pan, Servo *tilt) {
	uint16_t errorStatus = 0;

	lastRoutine = String(F("initPanTilt"));

	console.println(F("Initializing Pan/Tilt Position.."));

	//  Put the front pan/tilt at home position
	errorStatus = moveServoPw(pan, pan->homePos, false);

	if (errorStatus != 0) {
		processError(errorStatus, "Could not initialize the " + pan->descr + " servo");
	} else {
		errorStatus = moveServoPw(tilt, tilt->homePos, true);

		if (errorStatus != 0) {
			processError(errorStatus, "Could not initialize the " + tilt->descr + " servo");
		}
	}

	return errorStatus;
}

/*
	Initialize the motors.
*/
uint16_t initMotors (ServoMotor *leftM1, ServoMotor *rightM2) {
	uint16_t errorStatus = 0;

	lastRoutine = String(F("initMotors"));

	console.println(F("Initializing Motors.."));

	errorStatus = setMotorSpeed(leftM1, SERVO_MOTOR_LEFT_NEUTRAL, false);

	if (errorStatus != 0) {
		processError(errorStatus, "Could not set the speed for the " + leftM1->descr + " motor");
	} else {
		leftM1->forward = SERVO_MOTOR_LEFT_DIRECTION;

		errorStatus = setMotorSpeed(rightM2, SERVO_MOTOR_RIGHT_NEUTRAL, true);

		if (errorStatus != 0) {
			processError(errorStatus, "Could not set the speed for the " + rightM2->descr + " motor");
		} else {
			rightM2->forward = SERVO_MOTOR_RIGHT_DIRECTION;
		}
	}

	return errorStatus;
}

/*
	Initialize the RoboClaw 2x5 motor controller
*/
void initRoboClaw (uint8_t address, uint16_t bps, Motor *leftM1, Motor *rightM2) {
	console.print(F("Initializing the RoboClaw 2x5 Motor Controller at address "));
	console.print(address, HEX);
	console.print(F(", for "));
	console.print(bps);
	console.println(F(" Bps communication."));

	roboClaw.begin(bps);

	//	Set the RoboClaw motor constants
	roboClaw.SetM1VelocityPID(address, ROBOCLAW_KD, ROBOCLAW_KP, ROBOCLAW_KI, ROBOCLAW_QPPS);
	roboClaw.SetM2VelocityPID(address, ROBOCLAW_KD, ROBOCLAW_KP, ROBOCLAW_KI, ROBOCLAW_QPPS);

	//	For Packet Serial modes
	leftM1->descr = ROBOCLAW_MOTOR_LEFT_NAME;
	leftM1->encoder = 0;
	leftM1->encoderStatus = 0;
	leftM1->encoderValid = false;
	leftM1->mspeed = 0;
	leftM1->speedStatus = 0;
	leftM1->speedValid = false;
	leftM1->forward = true;
	leftM1->distance = 0;
	leftM1->distanceValid = false;		    

	//	For Packet Serial modes
	rightM2->descr = ROBOCLAW_MOTOR_RIGHT_NAME;
	rightM2->encoder = 0;
	rightM2->encoderStatus = 0;
	rightM2->encoderValid = false;
	rightM2->mspeed = 0;
	rightM2->speedStatus = 0;
	rightM2->speedValid = false;
	rightM2->forward = true;
	rightM2->distance = 0;
	rightM2->distanceValid = false;		    
}

/*
	Initialize sensors
*/
uint16_t initSensors (void) {
	uint16_t errorStatus = 0;

	lastRoutine = String(F("initSensors"));

	console.println(F("Initializing Sensors.."));

	//	Initialize the 10DOF Inertial Measurement Unit (Adafruit)
	if (HAVE_10DOF_IMU || HAVE_LSM303DLHC_ACCEL) {
		//	Initialize the accelerometer
		console.println(F("     LSM303DLHC Accelerometer.."));

		if (! accelerometer.begin()) {
			/* There was a problem detecting the LSM303DLHC ... check your connections */
			errorStatus = 601;
			processError(errorStatus, F("Ooops, no LSM303DLHC detected ... Check your wiring!"));
		} else {
			console.println(F("     LSM303DLHC Magnetometer (Compass).."));

			//	Initialize the magnetometer (compass) sensor
			if (! compass.begin()) {
				/*	There was a problem detecting the LSM303DLHC ... check your connections */
				errorStatus = 602;
				processError(errorStatus, F("Ooops, no LSM303DLHC detected ... Check your wiring!"));
			}
		}
	}

	if ((errorStatus == 0) && (HAVE_10DOF_IMU || HAVE_L3GD20_GYRO)) {
		console.println(F("     L3GD20 Gyroscope.."));

		//	Initialize and warn if we couldn't detect the gyroscope chip
		if (! gyroscope.begin(gyroscope.L3DS20_RANGE_250DPS)) {
			errorStatus = 603;
			processError(errorStatus, F("Oops ... unable to initialize the L3GD20. Check your wiring!"));
		}
	}

	if ((errorStatus == 0) && (HAVE_10DOF_IMU || HAVE_BMP180_TEMP)) {

	}

	if ((errorStatus == 0) && HAVE_10DOF_IMU) {
		console.println(F("     10 DOF Inertial Measurement Unit.."));

		imu.begin();
	}

	if ((errorStatus == 0) && (HAVE_COLOR_SENSOR)) {
		//	Initialize the TCS3725 RGB Color sensor (Adafruit)
		if (! rgb.begin()) {
			errorStatus = 604;
			processError(errorStatus, F("There was a problem initializing the TCS34725 RGB color sensor .. check your wiring or I2C Address!"));
		}
	}

	if ((errorStatus == 0) && (HAVE_HEAT_SENSOR)) {
		//	Initialize the TMP006 heat sensor
		if (! heat.begin()) {
			errorStatus = 605;
			processError(errorStatus, F("There was a problem initializing the TMP006 heat sensor .. check your wiring or I2C Address!"));
		}
	}

	if ((errorStatus == 0) && (HAVE_DS1307_RTC)) {
		console.println(F("     DS1307 Real Time Clock.."));

		//	Check to be sure the RTC is running
		if (! clock.isrunning()) {
			errorStatus = 606;
			processError(errorStatus, F("The Real Time Clock is NOT running!"));
		}
	}

	return errorStatus;
}

/*
	Initialize servos to defaults
*/
void initServos (void) {
	lastRoutine = String(F("initServos"));

	gripLift.pin = SERVO_GRIP_LIFT_PIN;
	gripLift.descr = String(SERVO_GRIP_LIFT_NAME);
	gripLift.offset = SERVO_GRIP_LIFT_OFFSET;
	gripLift.homePos = SERVO_GRIP_LIFT_HOME;
	gripLift.msPulse = 0;
	gripLift.angle = 0;
	gripLift.minPulse = SERVO_GRIP_LIFT_MIN;
	gripLift.maxPulse = SERVO_GRIP_LIFT_MAX;
	gripLift.maxDegrees = SERVO_MAX_DEGREES;
	gripLift.error = 0;

	gripWrist.pin = SERVO_GRIP_WRIST_PIN;
	gripWrist.descr = String(SERVO_GRIP_WRIST_NAME);
	gripWrist.offset = SERVO_GRIP_WRIST_OFFSET;
	gripWrist.homePos = SERVO_GRIP_WRIST_HOME;
	gripWrist.msPulse = 0;
	gripWrist.angle = 0;
	gripWrist.minPulse = SERVO_GRIP_WRIST_MIN;
	gripWrist.maxPulse = SERVO_GRIP_WRIST_MAX;
	gripWrist.maxDegrees = SERVO_MAX_DEGREES;
	gripWrist.error = 0;

	gripGrab.pin = SERVO_GRIP_GRAB_PIN;
	gripGrab.descr = String(SERVO_GRIP_GRAB_NAME);
	gripGrab.offset = SERVO_GRIP_GRAB_OFFSET;
	gripGrab.homePos = SERVO_GRIP_GRAB_HOME;
	gripGrab.msPulse = 0;
	gripGrab.angle = 0;
	gripGrab.minPulse = SERVO_GRIP_GRAB_MIN;
	gripGrab.maxPulse = SERVO_GRIP_GRAB_MAX;
	gripGrab.maxDegrees = SERVO_MAX_DEGREES;
	gripGrab.error = 0;

	pan.pin = SERVO_PAN_PIN;
	pan.descr = String(SERVO_PAN_NAME);
	pan.offset = SERVO_PAN_OFFSET;
	pan.homePos = SERVO_PAN_HOME;
	pan.msPulse = 0;
	pan.angle = 0;
	pan.minPulse = SERVO_PAN_LEFT_MIN;
	pan.maxPulse = SERVO_PAN_RIGHT_MAX;
	pan.maxDegrees = SERVO_MAX_DEGREES;
	pan.error = 0;

	tilt.pin = SERVO_TILT_PIN;
	tilt.descr = String(SERVO_TILT_NAME);
	tilt.offset = SERVO_TILT_OFFSET;
	tilt.homePos = SERVO_TILT_HOME;
	tilt.msPulse = 0;
	tilt.angle = 0;
	tilt.minPulse = SERVO_TILT_DOWN_MIN;
	tilt.maxPulse = SERVO_TILT_UP_MAX;
	tilt.maxDegrees = SERVO_MAX_DEGREES;
	tilt.error = 0;
}

/************************************************************/
/*	Runs once to initialize everything						*/
/************************************************************/
void setup (void) {
	uint16_t errorStatus = 0;

	lastRoutine = String(F("SETUP"));

	//  Initialize the console port
	console.begin(115200);

	console.println();
	console.print(F("SES Rover Motor Servo Test, version "));
	console.print(BUILD_VERSION);
	console.print(F(" on "));
	console.println(BUILD_DATE);
	console.print(F("     for the "));
	console.print(BUILD_BOARD);
	console.println(F("."));

	//	Delay for 10 seconds, before starting initialization
	console.println();
	wait(7, "initialization");

	console.println();
	console.println(F("Initializing Serial Ports.."));

	//	Initialize the SSC-32 servo controller port
	ssc32.begin(115200);

	//	Initialize the XBee (ZigBee) Mesh Wireless port
	xbee.begin(115200);

	console.println(F("Initializing Digital Pins.."));

	//  Initialize the LED pin as an output.
	pinMode(HEARTBEAT_LED, OUTPUT);
	digitalWrite(HEARTBEAT_LED, LOW);

	if (HAVE_COLOR_SENSOR) {
		//	Initialize and turn off the TCS34725 RGB Color sensor's LED
		pinMode(COLOR_SENSOR_LED, OUTPUT);
		digitalWrite(COLOR_SENSOR_LED, LOW);
		delay(250);
		digitalWrite(COLOR_SENSOR_LED, HIGH);
		delay(250);
		digitalWrite(COLOR_SENSOR_LED, LOW);
	}

	if (HAVE_7SEG_DISPLAYS) {
		//	Initialize the displays
		initDisplays(MAX_NUMBER_7SEG_DISPLAYS);

		//	Test the displays
		testDisplays(MAX_NUMBER_7SEG_DISPLAYS);
	}

 	//  Initialize all servos
 	initServos();

	//	Initialize all sensors
	errorStatus = initSensors();

	if (errorStatus != 0) {
		processError(errorStatus, F("Could not initialize the sensors"));
	} else {
		//	Set the Pan/Tilt to home position
		errorStatus = initPanTilt(&pan, &tilt);

		if (errorStatus != 0) {
			processError(errorStatus, F("Could not initialize the PAN/TILT"));
		} else {
			//	Set the Gripper to home position
			errorStatus = initGripper(&gripLift, &gripWrist, &gripGrab);
		}

		if (errorStatus != 0) {
			processError(errorStatus, F("Could not initialize the GRIPPER"));
		} else {
			//	Initialize the motors
			errorStatus = initMotors(&leftMotorM1, &rightMotorM2);
		}

		if (errorStatus != 0) {
			processError(errorStatus, F("Could not initialize the motors"));
		} else {
			console.println();

			//	Start the motors, forward
			console.println(F("Starting the motors, forward"));
			errorStatus = setMotorSpeed(&leftMotorM1, 500, false);

			if (errorStatus != 0) {
				processError(errorStatus, F("Could not set speed for the LEFT motor"));
			} else {
				errorStatus = setMotorSpeed(&rightMotorM2, 500, true);

				if (errorStatus != 0) {
					processError(errorStatus, F("Could not set speed for the RIGHT motor"));
				} else {
					delay(3000);

					//	Stop the motors
					errorStatus = stopMotors();

					if (errorStatus != 0) {
						runAwayRobot(errorStatus);
					}					
				}
			}

			if (errorStatus != 0) {
				processError(errorStatus, F("Runaway robot"));
			} else {
				//	Start the motors, reverse
				console.println(F("Starting the motors, reverse"));
				errorStatus = setMotorSpeed(&leftMotorM1, -500, false);

				if (errorStatus != 0) {
					processError(errorStatus, F("Could not set speed for the LEFT motor"));
				} else {
					errorStatus = setMotorSpeed(&rightMotorM2, -500, true);

					if (errorStatus != 0) {
						processError(errorStatus, F("Could not set speed for the RIGHT motor"));
					} else {
						delay(3000);

						//	Stop the motors
						errorStatus = stopMotors();

						if (errorStatus != 0) {
							runAwayRobot(errorStatus);
						}
					}
				}
			}
		}

		if (errorStatus == 0) {
			//	Scan the entire 180 degree range and take readings
			console.println(F("Doing initial area scan.."));
			errorStatus = scanArea(&pan, -90, 90, 10);

			if (errorStatus != 0) {
				processError(errorStatus, F("Could not complete the initial area scan"));
			} else {
				areaScanValid = true;
			}
		}
	}
}

/************************************************************/
/*	Runs forever											*/
/************************************************************/
void loop (void) {
	uint16_t errorStatus = 0;

	//	The current date and time from the DS1307 real time clock
	DateTime now = clock.now();

	uint16_t displayInt;

	//	Display related variables
	boolean amTime;
	uint8_t displayNr = 0, count = 0, readingNr = 0;
	uint8_t currentHour = now.hour(), nrDisplays = 0;

	uint8_t analogPin = 0;
	uint8_t digitalPin = 0;

	DistanceObject distObject;
	ColorSensor colorData;
	HeatSensor heatData;
	InertialMeasurementUnit imuData;

	/*
		Code starts here
	*/

	lastRoutine = String(F("LOOP"));

	// Pulse the heartbeat LED
	pulseDigital(HEARTBEAT_LED, 500);

	currentMinute = now.minute();

	//	Send something wirelessly
	xbee.println(F("Hello World, I'm wireless!"));

	/*
		This is code that only runs ONE time, to initialize
			special cases.
	*/
	if (firstLoop) {
		console.println(F("Entering the main loop.."));

		lastMinute = currentMinute;

		firstLoop = false;
	}

	//	Start our motors at a reasonable speed.
	setMotorSpeed(&leftMotorM1, 100, false);
	setMotorSpeed(&rightMotorM2, 100, true);

	//  Display the date.
	if (displayDate && DISPLAY_INFORMATION && HAVE_7SEG_DISPLAYS) {
		displayInt = (now.month() * 100) + now.day();  

		//  Month and day
		writeNumber(0, displayInt, 0, true);
		matrix8x8.drawBitmap(0, 0, date_bmp, 8, 8, LED_ON);

		sevenSeg[0].writeDisplay();
		matrix8x8.writeDisplay();

		delay(5000);

		sevenSeg[0].clear();
		matrix8x8.clear();  

		//  Year
		writeNumber(0, now.year(), 0, false);
		matrix8x8.drawBitmap(0, 0, year_bmp, 8, 8, LED_ON);

		sevenSeg[0].writeDisplay();
		matrix8x8.writeDisplay();

		delay(5000);
    
		dateMinuteCount = 0;
		clearDisplays();
	}
  
	//	Display the time
	if (displayTime && DISPLAY_INFORMATION && HAVE_7SEG_DISPLAYS) {
		if (currentHour > 12) {
			amTime = false;
			currentHour = currentHour - 12;
		} else {
			amTime = true;
		}
	  
		displayInt = (currentHour * 100) + now.minute();  

		//  Display the current time on the 7 segment display
		writeNumber(0, displayInt, 0, false);
		sevenSeg[0].drawColon(true);
	  
		matrix8x8.clear();
	  
		if (amTime) {
			matrix8x8.drawBitmap(0, 0, am_bmp, 8, 8, LED_ON);
		} else {
			matrix8x8.drawBitmap(0, 0, pm_bmp, 8, 8, LED_ON);
		}
	  
		sevenSeg[0].writeDisplay();
		matrix8x8.writeDisplay();
	  
		delay(5000);

		timeMinuteCount = 0;
		clearDisplays();
	}

	if (HAVE_LSM303DLHC_ACCEL || HAVE_L3GD20_GYRO || HAVE_BMP180_TEMP || HAVE_10DOF_IMU) {
		imuData = readIMU();

		//	Accelerometer and Magnetometer (Compass)
		if (HAVE_LSM303DLHC_ACCEL || HAVE_10DOF_IMU) {
			console.println(F("Processing the accelerometer and magnetometer readings"));
			imuData.accelCompassData = readLSM303DLHCAccelMag();
		}

		//	Gyroscope
		if (HAVE_L3GD20_GYRO || HAVE_10DOF_IMU) {
			console.println(F("Processing the gyroscope readings"));
			imuData.gyroData = readL3GD20Gyro();
		}

		//	Temperature, Pressure, and Altitude
		if (HAVE_BMP180_TEMP || HAVE_10DOF_IMU) {
			console.println(F("Processing the temperature readings"));
			imuData.tempData = readBMP180Temp();
		}

		//	Orientation
		if (HAVE_10DOF_IMU) {
			calculatePitchRoll(&imuData, &imuData.accelCompassData.accelEvent, &imuData.accelCompassData.compassEvent, &imuData.orientation);
		}

		displayIMUData(&imuData);

		/*
			Put Accelerometer, Compass, Gyro, and Orientation reactive behaviors HERE
		*/
	}

	console.println(F("Getting Distance Sensor readings.."));

	//	Get readings from all the GP2Y0A21YK0F Analog IR range sensors, if any, and store them
	if (MAX_NUMBER_IR > 0) {
		for (analogPin = 0; analogPin < MAX_NUMBER_IR; analogPin++) { 
			ir[analogPin] = readSharpGP2Y0A21YK0F(analogPin);
		}

		displayIR();
	}

	//	Get readings from all the Parallax PING Ultrasonic range sensors, if any, and store them
	if (MAX_NUMBER_PING > 0) {
		for (digitalPin = 0; digitalPin < MAX_NUMBER_PING; digitalPin++) {
			ping[digitalPin] = readParallaxPING(digitalPin);
		}

		displayPING();
	}

	/*
		Put distance related reactive behaviors HERE
	*/

	//	Let's see if we're too close to an object.. If so, stop and scan the area
	if ((ping[PING_FRONT_CENTER] < PING_MIN_DISTANCE_CM) || (ir[IR_FRONT_CENTER] < IR_MIN_DISTANCE_CM)) {
		errorStatus = stopMotors();

		if (errorStatus != 0) {
			runAwayRobot(errorStatus);
		}

		//	Scan the area for a clear path
		errorStatus = scanArea(&pan, -90, 90, 10);

		//	Find the closest and farthest objects
		distObject = findDistanceObjects();

		errorStatus = turnToFarthestObject(&distObject);

		if (errorStatus != 0) {
			processError(errorStatus, F("Could not complete a turn to the farthest object"));

			errorStatus = stopMotors();

			if (errorStatus != 0) {
				runAwayRobot(errorStatus);
			}
		}
	}

	//	Read the TCS34725 RGB color sensor, if we have it
	if (HAVE_COLOR_SENSOR) {
		colorData = readColorSensor();

		displayColorSensorData(&colorData);
	}

	//	Read the TMP006 heat sensor, if we have it
	if (HAVE_HEAT_SENSOR) {
		heatData = readHeatSensor();
	
		displayHeatSensorData(&heatData);
	}

	//	Find the closest and farthest objects
	distObject = findDistanceObjects();

	/*
		Put object tracking behaviors HERE
	*/

	console.println();

	if (imuData.tempData.temperatureValid && displayTemperature && DISPLAY_INFORMATION && HAVE_7SEG_DISPLAYS) {
		//  Display the temperature in Fahrenheit
		writeNumber(0, int(imuData.tempData.fahrenheit * 100), 2, false);
		sevenSeg[0].writeDisplay();

		matrix8x8.clear();
		matrix8x8.drawBitmap(0, 0, f_bmp, 8, 8, LED_ON);
		matrix8x8.writeDisplay();

		delay(5000);

		//  Display the temperature in Celsius
		writeNumber(0, int(imuData.tempData.celsius * 100), 2, false);
		sevenSeg[0].writeDisplay();

		matrix8x8.clear();
		matrix8x8.drawBitmap(0, 0, c_bmp, 8, 8, LED_ON);
		matrix8x8.writeDisplay();

		delay(5000);
		temperatureMinuteCount = 0;
		clearDisplays();
	}

	//	Count the minutes
	if (currentMinute != lastMinute) {
		if (DISPLAY_INFORMATION && HAVE_7SEG_DISPLAYS) {
			dateMinuteCount += 1;
			temperatureMinuteCount += 1;
			timeMinuteCount += 1;
		}

		minuteCount += 1;
		lastMinute = currentMinute;
	}

	/*
		Update the information display control variables
	*/
	if (DISPLAY_INFORMATION && HAVE_7SEG_DISPLAYS) {
		displayDate = (dateMinuteCount == DISPLAY_DATE_FREQ_MIN);
		displayTemperature = (temperatureMinuteCount == DISPLAY_TEMPERATURE_FREQ_MIN);
		displayTime = (timeMinuteCount == DISPLAY_TIME_FREQ_MIN);
	}

	/*
		Delay a bit, to allow time to read the Serial Monitor information log
	*/
	wait(LOOP_DELAY_SECONDS);

	console.println();
}
