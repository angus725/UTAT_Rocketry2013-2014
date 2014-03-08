// SPI interface and selectors
#ifdef SCK
#define SCK_CUSTOM SCK
#else
#define SCK_CUSTOM 20
#endif

#ifdef MOSI
#define MOSI_CUSTOM MOSI
#else
#define MOSI_CUSTOM 21
#endif

#define THERMO_1_SELECTOR 22
#define THERMO_2_SELECTOR 23
#define THERMO_3_SELECTOR 24

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
	bool readThermo;

	bool checkError(Adafruit_MAX31855 checkThis, int reference); //prototype

	Adafruit_MAX31855
		thermo1(SCK_CUSTOM, THERMO_1_SELECTOR, MOSI_CUSTOM),
		thermo2(SCK_CUSTOM, THERMO_2_SELECTOR, MOSI_CUSTOM),
		thermo3(SCK_CUSTOM, THERMO_3_SELECTOR, MOSI_CUSTOM);

	// 0 for pass, 1 for fail
	bool checkAll(){
		if (
			checkError(thermo1, 1) ||
			checkError(thermo2, 2) ||
			checkError(thermo3, 3)
			)
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

	// refrain from using outside of checkAll if possible
	bool checkError(Adafruit_MAX31855 checkThis, int reference){

		uint8_t errorCode = checkThis.readError();
		if (errorCode == 0)
		{
			return false; //no errors
		}
		else if (errorCode & B1) //binary
		{
			SERIALPRINT("OPEN circuit error on thermo couple ");
			PRINTINTEGER(reference);
			SERIALPRINT("\n");

		}
		else if (errorCode & B10) //binary
		{
			SERIALPRINT("SHORT TO GROUND circuit error on thermo couple ");
			PRINTINTEGER(reference);
			SERIALPRINT("\n");

		}
		else if (errorCode & B100) //binary
		{
			SERIALPRINT("SHORT TO VCC circuit error on thermo couple ");
			PRINTINTEGER(reference);
			SERIALPRINT("\n");

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

		SERIALPRINT("Thermocouple_");
		SERIALPRINT(reference);
		SERIALPRINT(" tempreture:");
		PRINTDOUBLE(externalTemp, 2);
		SERIALPRINT("\n");
		return externalTemp;
	}
}





void setup()
{

	Serial.begin(9600);

	Serial.println("MAX31855 test");
	// wait for MAX chip to stabilize
	delay(500);
	thermo::checkAll();
}

void loop()
{
	if (!thermo::thermoFail){
		// basic readout test, just print the current temp
		Serial.print("Internal Temp = ");
		Serial.println(thermo::thermo1.readInternal());

		double c = thermo::thermo1.readCelsius();
		if (isnan(c)) {
			Serial.println("Something wrong with thermocouple!");
		}
		else {
			Serial.print("C = ");
			Serial.println(c);
		}
		//Serial.print("F = ");
		//Serial.println(thermocouple.readFarenheit());

		delay(1000);
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
