// SPI interface and selectors
#ifdef SCK
#define SCK_CUSTOM SCK
#else
#define SCK_CUSTOM 52
#endif

#ifdef MOSI
#define MOSI_CUSTOM MOSI
#else
#define MOSI_CUSTOM 50
#endif

#define THERMO_1_SELECTOR 47
#define THERMO_2_SELECTOR 48
#define THERMO_3_SELECTOR 49

//being lazy...
#define SERIALPRINT(value) Serial.print(value)
#define PRINTINTEGER(value) printInteger(value)
#define PRINTDOUBLE(value, precision) printDouble(value, precision)




/***************************************************
This is a library for the Adafruit Thermocouple Sensor w/MAX31855K

Designed specifically to work with the Adafruit Thermocouple Sensor
----> https://www.adafruit.com/products/269

These displays use SPI to communicate, 3 pins are required to
interface
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, all text above must be included in any redistribution
****************************************************/
#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>


//Written by Sanjeev Narayanaswamy and Hayden Lau
//Feb 21, 2014
#define ARM_PIN 12
#define IGN_PIN 13
#define IGN_CONT_PIN A0

//Initialize Variables
char inputChar;
int continuityRaw;
boolean isArmed;
boolean continuity;
int contLow = 400; //Continuity Threshold Lower Value
int contHigh = 1023; //Continuity Threshold Upper Value


void(*resetFunc) (void) = 0; // pointer to beginning of program storage? calling this resets the sketch completely



namespace ignition
{

	void continuityCheck()
	{
		Serial.println("Performing Continuity Test...");
		continuityRaw = analogRead(IGN_CONT_PIN);
		//Serial.println(continuityRaw);

		if (continuityRaw < contLow || continuityRaw > contHigh)
		{
			Serial.println("ERROR: CONTINUITY PROBLEM");
			continuity = false;
		}
		else
		{
			Serial.println("Continuity Acceptable");
			continuity = true;
		}

	}

	void arm()
	{ // arms the circuit
		Serial.println("Arming...");
		digitalWrite(ARM_PIN, HIGH);
		isArmed = true;
	}

	void disarm()
	{ // disarms the circuit
		Serial.println("Disarming...");
		digitalWrite(IGN_PIN, LOW);
		digitalWrite(ARM_PIN, LOW);
		isArmed = false;
	}

	void fire()
	{//fires the circuit if armed and continuity is satisfied
		if ((continuity == true) && (isArmed == true))
		{
			Serial.println("Firing...");
			digitalWrite(IGN_PIN, HIGH);
		}
	}

}


class Adafruit_MAX31855 {
public:
	Adafruit_MAX31855(int8_t SCLK, int8_t CS, int8_t MISO);

	double readInternal(void);
	double readCelsius(void);
	double readFarenheit(void);
	uint8_t readError();
	void readAll(float &, double &, uint8_t &);

private:
	int8_t sclk, miso, cs;
	uint32_t spiread32(void);
};

//(int8_t SCLK, int8_t CS, int8_t MISO) 
Adafruit_MAX31855::Adafruit_MAX31855(int8_t SCLK, int8_t CS, int8_t MISO) {
	sclk = SCLK;
	cs = CS;
	miso = MISO;

	//define pin modes
	pinMode(cs, OUTPUT);
	pinMode(sclk, OUTPUT);
	pinMode(miso, INPUT);

	digitalWrite(cs, HIGH);
}


double Adafruit_MAX31855::readInternal(void) {
	uint32_t v;

	v = spiread32();

	// ignore bottom 4 bits - they're just thermocouple data
	v >>= 4;

	// pull the bottom 11 bits off
	float internal = v & 0x7FF;
	internal *= 0.0625; // LSB = 0.0625 degrees
	// check sign bit!
	if (v & 0x800)
		internal *= -1;
	//Serial.print("\tInternal Temp: "); Serial.println(internal);
	return internal;
}

double Adafruit_MAX31855::readCelsius(void) {

	int32_t v;

	v = spiread32();

	//Serial.print("0x"); Serial.println(v, HEX);

	/*
	float internal = (v >> 4) & 0x7FF;
	internal *= 0.0625;
	if ((v >> 4) & 0x800)
	internal *= -1;
	Serial.print("\tInternal Temp: "); Serial.println(internal);
	*/

	if (v & 0x7) {
		// uh oh, a serious problem!
		return NULL;
	}

	// get rid of internal temp data, and any fault bits
	v >>= 18;
	//Serial.println(v, HEX);

	// pull the bottom 13 bits off
	int16_t temp = v & 0x3FFF;

	// check sign bit
	if (v & 0x2000)
		temp |= 0xC000;
	//Serial.println(temp);

	double centigrade = v;

	// LSB = 0.25 degrees C
	centigrade *= 0.25;
	return centigrade;
}

uint8_t Adafruit_MAX31855::readError() {
	return spiread32() & 0x7;
}

double Adafruit_MAX31855::readFarenheit(void) {
	float f = readCelsius();
	f *= 9.0;
	f /= 5.0;
	f += 32;
	return f;
}

uint32_t Adafruit_MAX31855::spiread32(void) {
	int i;
	uint32_t d = 0;

	digitalWrite(sclk, LOW);
	_delay_ms(1);
	digitalWrite(cs, LOW);
	_delay_ms(1);

	for (i = 31; i >= 0; i--)
	{
		digitalWrite(sclk, LOW);
		_delay_ms(1);
		d <<= 1;
		if (digitalRead(miso)) {
			d |= 1;
		}

		digitalWrite(sclk, HIGH);
		_delay_ms(1);
	}

	digitalWrite(cs, HIGH);
	//Serial.println(d, HEX);
	return d;
}

//give me values to change!
void Adafruit_MAX31855::readAll(float &internalTemp, double &celsius, uint8_t &errno)
{
	uint32_t v;
	v = spiread32();

	if (v & 0x7) {
		// uh oh, a serious problem!
		errno = spiread32() & 0x7;
		return;
	}

	// ignore bottom 4 bits - they're just thermocouple data
	v >>= 4;

	// pull the bottom 11 bits off
	internalTemp = v & 0x7FF;
	internalTemp *= 0.0625; // LSB = 0.0625 degrees
	// check sign bit!
	if (v & 0x800)
		internalTemp *= -1;
	//Serial.print("\tInternal Temp: "); Serial.println(internal);




	// get rid of internal temp data, and any fault bits
	v >>= 14;
	//Serial.println(v, HEX);

	// pull the bottom 13 bits off
	int16_t temp = v & 0x3FFF;

	// check sign bit
	if (v & 0x2000)
		temp |= 0xC000;
	//Serial.println(temp);

	celsius = v;

	// LSB = 0.25 degrees C
	celsius *= 0.25;
	return;

}







namespace thermo
{

	bool thermoFail = false;
	bool	thermoEnable1 = false,
			thermoEnable2 = false,
			thermoEnable3 = false;
	bool readThermo;

	bool checkError(Adafruit_MAX31855 checkThis, int reference, bool silent);
		//prototype

	Adafruit_MAX31855
		thermo1(SCK_CUSTOM, THERMO_1_SELECTOR, MOSI_CUSTOM),
		thermo2(SCK_CUSTOM, THERMO_2_SELECTOR, MOSI_CUSTOM),
		thermo3(SCK_CUSTOM, THERMO_3_SELECTOR, MOSI_CUSTOM);

	// 0 for pass, 1 for fail
	bool checkAll(){
		bool doesItError = false;
		thermoEnable1 = checkError(thermo1, 1, false);
		thermoEnable2 = checkError(thermo2, 2, false);
		thermoEnable3 = checkError(thermo3, 3, false);
		doesItError = thermoEnable1 && thermoEnable2 && thermoEnable3;
		if (doesItError)
		{
			SERIALPRINT("Thermocouple checking FAILED\n");
			thermoFail = true;
			return true;
		}
		else
			SERIALPRINT("Thermocouple checking PASSED\n");
		thermoFail = false;
		return false;
	}

	bool checkAll(bool silent){
		bool doesItError = false;
		thermoEnable1 = checkError(thermo1, 1, false);
		thermoEnable2 = checkError(thermo2, 2, false);
		thermoEnable3 = checkError(thermo3, 3, false);
		doesItError = thermoEnable1 && thermoEnable2 && thermoEnable3;
		if (doesItError)
		{
			SERIALPRINT("Thermocouple checking FAILED\n");
			thermoFail = true;
			return true;
		}
		thermoFail = false;
		return false;
	}

	// refrain from using outside of checkAll if possible
	bool checkError(Adafruit_MAX31855 checkThis, int reference, bool silent){

		uint8_t errorCode = checkThis.readError();
		if (errorCode == 0 && !silent)
		{
			SERIALPRINT("Connected to Thermo couple ");
			PRINTINTEGER(reference);
			SERIALPRINT("\n");
			return false; //no errors
		}
		else if (errorCode & B1 && !silent) //binary
		{
			SERIALPRINT("OPEN circuit error on thermo couple ");
			PRINTINTEGER(reference);

		}
		else if (errorCode & B10 && !silent) //binary
		{
			SERIALPRINT("SHORT TO GROUND circuit error on thermo couple ");
			PRINTINTEGER(reference);

		}
		else if (errorCode & B100 && !silent) //binary
		{
			SERIALPRINT("SHORT TO VCC circuit error on thermo couple ");
			PRINTINTEGER(reference);
		}
		return true; // error
	}

	//prints internal tempreture
	int printInternal(int reference)
	{
		double internalTemp;
		switch (reference)
		{
		case 1:
			internalTemp = thermo1.readInternal();
			break;
		case 2:
			internalTemp = thermo2.readInternal();
			break;
		case 3:
			internalTemp = thermo3.readInternal();
			break;
		default:
			internalTemp = 9000.1;
			break;
		}

		SERIALPRINT("Thermocouple_");
		SERIALPRINT(reference);
		SERIALPRINT(" internal tempreture:");
		PRINTDOUBLE(internalTemp, 2);
		SERIALPRINT("\n");
		return internalTemp;
	}

	//prints external tempreture
	int printC(int reference)
	{
		double externalTemp;
		switch (reference)
		{
		case 1:
			externalTemp = thermo1.readCelsius();
			break;
		case 2:
			externalTemp = thermo2.readCelsius();
			break;
		case 3:
			externalTemp = thermo3.readCelsius();
			break;
		default:
			externalTemp = 9000.1;
			break;
		}

		SERIALPRINT("!Thermocouple_");
		SERIALPRINT(reference);
		SERIALPRINT(" tempreture:");
		PRINTDOUBLE(externalTemp, 2);
		SERIALPRINT("\n");
		return externalTemp;
	}

	void printValidData(Adafruit_MAX31855 & thermocouple, int referenceNum)
	{
		// basic readout test, just print the current temp
		Serial.print("!");
		Serial.print(referenceNum);
		Serial.print(":\nInternal Temp = ");
		Serial.println(thermocouple.readInternal());

		double c = thermocouple.readCelsius();
		if (isnan(c)) {
			Serial.println("\nSomething wrong with thermocouple!");
		}
		else {
			Serial.print("\n!C = ");
			Serial.println(c);
		}
		//Serial.print("F = ");
		//Serial.println(thermocouple.readFarenheit());
	}
}



void setup()
{

	Serial.begin(9600);

	delay(500);
	thermo::checkAll();

	// initialize serial communications at 9600 bps:
	pinMode(ARM_PIN, OUTPUT);
	pinMode(IGN_PIN, OUTPUT);
	digitalWrite(ARM_PIN, 0);
	boolean isArmed = false;
	digitalWrite(IGN_PIN, 0);
	boolean continuity = false;

	//Print menu to Serial
	Serial.print("a = arm\nd = disarm\nf = fire\nc = continuity\nr = reset program");
	Serial.print("\n\n\n");
}

void loop()
{



	// continously get input
	while (Serial.available() > 0) Serial.read();
	// status message
	if (isArmed)
		Serial.print(" - ARMED");
	else
		Serial.print("Circuit is Disarmed");
	Serial.println(" - Waiting for command:");
	// wait for input
	while (!(Serial.available() > 0))
	{

		if (!thermo::thermoFail){
			if (!thermo::thermoEnable1)
				thermo::printValidData(thermo::thermo1, 1);
			if (!thermo::thermoEnable2)
				thermo::printValidData(thermo::thermo2, 2);
			if (!thermo::thermoEnable3)
				thermo::printValidData(thermo::thermo3, 3);
			thermo::checkAll(true); //silent check
		}
		else
			thermo::checkAll();
		delay(500);
	}
	;
	inputChar = Serial.read();
	Serial.println("acknowledged\n"); // acknowledge possible command
	switch (inputChar)
	{ // which command was it?
	case 'a':
	case 'A':
		ignition::arm();
		break;
	case 'd':
	case 'D':
		ignition::disarm();
		break;
	case 'f':
	case 'F':
		ignition::fire();
		break;
	case 'c':
	case 'C':
		ignition::continuityCheck();
		break;
	case 'r':
	case 'R':
		//Setting the output pins to low before resetting
		ignition::disarm();
		Serial.print("Resetting...");
		Serial.print("\n\n\n\n");
		Serial.flush(); // wait for serial to finish transmitting
		resetFunc(); // reset software completely
		break;

	default:
		Serial.println("Error - no matching case");
		break;
	}


}

void printInteger(int value)
{
	Serial.println(value);

	return;
}

void printDouble(double value, int precision)
{
	precision = pow(10, precision);
	Serial.print(int(value));  //prints the int part
	Serial.print("."); // print the decimal point
	int frac;
	if (value >= 0)
		frac = (value - int(value)) * precision;
	else
		frac = (int(value) - value) * precision;
	Serial.print(frac, DEC);
}
