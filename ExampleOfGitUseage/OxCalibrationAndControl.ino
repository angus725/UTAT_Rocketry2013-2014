#include <math.h>
#include <new.h>
#include "Adafruit_MAX31855.ino"
#include "ValveFunctions.ino"

// HAYDEN do these
// they can be found at the bottom of the code
#define READINTEGER() readInteger()
#define READBOOL() readBool()
#define PRINTINTEGER(value) printInteger(value)
#define PRINTBOOL(value) printBool(value)
#define PRINTDOUBLE(value, precision) printDouble(value, precision)

//being lazy...
#define SERIALPRINT(value) Serial.print(value)

//pins
#define LED_PIN 13

#define ARM_PIN 12
#define IGNITION_PIN 13
#define IGNITION__COUNT_PIN A0 //initialization?

// SPI interface and selectors
#define SCK 20
#define MOSI 21
#define THERMO_1_SELECTOR 22
#define THERMO_2_SELECTOR 23
#define THERMO_3_SELECTOR 24



#define OXDIZER_IGNITION_DELAY 1500 //ms
#define DEFAULT_DELAY 5000 //ms

#define MAINLOOP_TIME 1000 //how often to repeat thermocouple temp



uint16_t valve_angle;
bool positive_CCW;

namespace ignition
{
	//Initialize Variables
	int continuityRaw;
	boolean isArmed;
	boolean continuity;
	const int contLow = 400; //Continuity Threshold Lower Value
	const int contHigh = 1023; //Continuity Threshold Upper Value

	void continuityCheck()
	{
		Serial.println("Performing Continuity Test...");
		continuityRaw = analogRead(IGNITION__COUNT_PIN);
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
		digitalWrite(IGNITION_PIN, LOW);
		digitalWrite(ARM_PIN, LOW);
		isArmed = false;
	}

	void fire()
	{//fires the circuit if armed and continuity is satisfied
		if ((continuity == true) && (isArmed == true))
		{
			Serial.println("Firing...");
			digitalWrite(IGNITION_PIN, HIGH);
			delay(1000);
			digitalWrite(IGNITION_PIN, LOW);
		}
	}

}


namespace thermo
{

	bool readThermo;

	Adafruit_MAX31855
		thermo1(SCK, THERMO_1_SELECTOR, MOSI),
		thermo2(SCK, THERMO_2_SELECTOR, MOSI),
		thermo3(SCK, THERMO_3_SELECTOR, MOSI);

	// 0 for pass, 1 for fail
	bool checkAll(){
		if (
			checkError(thermo1, 1) ||
			checkError(thermo2, 2) ||
			checkError(thermo3, 3)
			)
		{
			SERIALPRINT("Thermocouple checking FAILED\n");
			return true;
		}
		else
			SERIALPRINT("Thermocouple checking PASSED\n");
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
	pinMode(LED_PIN, OUTPUT);
	pinMode(MOTOR_PIN1, OUTPUT);
	pinMode(MOTOR_PIN2, OUTPUT);
	pinMode(ENCODER_A, INPUT);

	pinMode(ARM_PIN, OUTPUT);
	pinMode(IGNITION_PIN, OUTPUT);

	digitalWrite(ARM_PIN, 0);
	ignition::isArmed = false;
	digitalWrite(IGNITION_PIN, 0);
	ignition::continuity = false;

	digitalWrite(LED_PIN, HIGH);
	TCCR1A = 0;

	thermo::readThermo = false;
	valveDirection::calibrated = false;

	thermo::checkAll();

}



void loop()
{


	char userInput = NULL;
	Serial.print("Main menu\n");
	Serial.print("\tM to calibrate the valve\n");
	Serial.print("\tN to calibrate computer with previously recorded output values\n");
	Serial.print("\tR to initiate Main firing sequence\n");
	Serial.print("\tI to print out calibration values\n");
	Serial.print("\tT to toggle main menu thermalcouple readout\n");
	Serial.print("\tC to check all thermo couples\n");

	while (!(Serial.available() > 0))
	{
		if (thermo::readThermo)
		{

			delay(MAINLOOP_TIME);
			//print thermo info
			thermo::printC(1);
			thermo::printC(2);
			thermo::printC(3);
		}
	}
	userInput = Serial.read();

	switch (userInput)
	{
	case 'm':
	case 'M':
		manualCalibration();
		valveDirection::calibrated = true;
		break;
	case 'n':
	case 'N':
		numericalCalibration();
		valveDirection::calibrated = true;
		break;

	case 'r':
	case 'R':
		if (valveDirection::calibrated == false)
			Serial.print("Please calibrate before use\n");
		else
			actuationSequence();
		break;

	case 'i':
	case 'I':
		PRINTBOOL(positive_CCW);
		PRINTINTEGER(valve_angle);
		break;

	case 'c':
	case 'C':
		thermo::printInternal(1);
		thermo::printInternal(2);
		thermo::printInternal(3);
		thermo::checkAll();
		break;

	case 't':
	case 'T':
		if (thermo::readThermo)
			thermo::readThermo = false;
		else
		{

			thermo::readThermo = true;
		}

		break;
	default:
		break;
	}



}


void actuationSequence()
{
	char userInput = NULL;
	int timeDelay = DEFAULT_DELAY; // default 5 sec delay
	bool armed = false;
	bool thermoOut = false;
	while (1) // '=' to return
	{
		if (!armed) //making sure ignition and software ARM are identical
			ignition::disarm();

		Serial.print("Firing sequnce started.\n");

		//firing delay
		Serial.print("\tD to set delay to firing in miliseconds\n");
		Serial.print("\tI to display timer value\n");

		//safety
		Serial.print("\n\t! to to release safety switch\n");
		Serial.print("\tX to to lock safety switch\n");

		Serial.print("\n\t1 to set valve to Fill\n");
		Serial.print("\t2 fire engine!\n");
		Serial.print("\tAny other character to shut valve, locks safety\n");

		Serial.print("\n\tC to manual ignition continuity check\n");

		Serial.print("\n\tT to single poll/stop tempreture readings\n");

		Serial.print("\n\t= to return to main menu\n");

		while (!(Serial.available() > 0))
		{
			if (thermoOut)
			{
				thermo::printC(1);
				thermo::printC(2);
				thermo::printC(3);
			}
		}
		userInput = Serial.read();

		switch (userInput)
		{
		case '1':
			if (armed)
			{
				Serial.print("Fill position\n");

				valveFunctions::fill_position();
				timeDelay = 0;
				armed = false;
			}
			else
				Serial.print("Safety lock is locked...\n");
			break;

		case '2':
			if (armed)
			{
				Serial.print("Firing!\n");
				while (timeDelay > 0)
				{
					delay(1000);
					PRINTINTEGER(timeDelay / 1000);
					SERIALPRINT("\n");
					timeDelay -= 1000;
				}

				// OX VALVE OPEN
				valveFunctions::drain_position();
				// IGNITION DELAY
				delay(OXDIZER_IGNITION_DELAY);
				// IGNITION
				ignition::fire();

				timeDelay = 0;
				armed = false;
				thermoOut = true;
			}
			else
				Serial.print("Safety lock is locked...\n");
			break;

		case 'd':
		case 'D':
			timeDelay = READINTEGER();
			armed = false;
			break;

		case 'c':
		case 'C':
			ignition::continuityCheck();
			break;

		case 't':
		case 'T':
			if (thermoOut)
				thermoOut = false;
			else
			{
				thermo::printC(1);
				thermo::printC(2);
				thermo::printC(3);
			}
			break;


		case '!':
			armed = true;
			ignition::arm();
			break;

		case '=':
			return;
			break;


		case 'i':
		case 'I':
			Serial.print("\t\tTime delay:\n\t\t\t");
			PRINTINTEGER(timeDelay);
			if (armed || ignition::isArmed)
				Serial.print("Safety lock is ARMED\n");
			else
				Serial.print("Safety lock is LOCKED\n");
			break;

		default:
			Serial.print("Shutting valve\n");
			valveFunctions::shutValve();
			armed = false;
			break;
		}
	}



}


void numericalCalibration()
{
	Serial.print("\tType in clockwise boolean value\n");
	positive_CCW = READBOOL();
	Serial.print("\tType in valve value\n");
	valveDirection::setTo(READINTEGER());
	return;

}

void manualCalibration()
{
	// manual calibration
	char userInput = NULL;
	valve_angle = 0;

	calibrateCCW();

	while (1) // return statement inside case statement
	{
		Serial.print("Manual Calibration menu\n");
		Serial.print("\tT to reset motor turning direction\n");
		Serial.print("\tD if valve is set to position 1\n");
		Serial.print("\tL if the motor needs to be turned COUNTER ClockWise\n");
		Serial.print("\tR if the motor needs to be turned ClockWise\n");

		while (!(Serial.available() > 0));
		userInput = Serial.read();

		switch (userInput)
		{
		case 'l':
		case 'L':
			calibrateTuning('l');
			break;
		case 'r':
		case 'R':
			calibrateTuning('r');
			break;
		case 'd':
		case 'D':
			Serial.print("Computer is now set. Valve closing.\n");
			valveFunctions::shutValve();
			return;
			break;

		case 't':
		case 'T':
			calibrateCCW();
			break;
		default:
			break;
		}

	}

}

void calibrateTuning(char direction)
{
	int dir = 0; // direction
	if (direction = 'r')
		dir = 1;
	else
		dir = -1;
	char userInput = NULL;
	while (1) // return statement inside case statement
	{
		Serial.print("\t\t0 to turn 0.5 degrees\n");
		Serial.print("\t\t1 to turn 1 degrees\n");
		Serial.print("\t\t2 to turn 2 degrees\n");
		Serial.print("\t\t3 to turn 5 degrees\n");
		Serial.print("\t\t4 to turn 15 degrees\n");
		Serial.print("\t\t5 to turn 30 degrees\n");
		Serial.print("\t\t6 to turn 60 degrees\n");
		Serial.print("\t\t7 to turn 90 degrees\n");
		Serial.print("\t\tH to turn 180 degrees\n");

		while (!(Serial.available() > 0));
		userInput = Serial.read();

		switch (userInput)
		{
		case '0':
			precisionTurn(5 * dir, STALL_SPEED);
			return;
		case '1':
			precisionTurn(10 * dir, STALL_SPEED);
			return;
		case '2':
			precisionTurn(20 * dir, STALL_SPEED);
			return;
		case '3':
			precisionTurn(50 * dir, LOW_SPEED);
			return;
		case '4':
			precisionTurn(150 * dir, MED_SPEED);
			return;
		case '5':
			precisionTurn(300 * dir, MED_SPEED);
			return;
		case '6':
			precisionTurn(600 * dir, HIGH_SPEED);
			return;
		case '7':
			turnValveByValue(QUARTER_DEG * dir);
			return;
		case 'h':
		case 'H':
			turnByPercent(FULL * dir);
			return;
		default:
			break;
		}

	}


}

// makes CounterClockWise direction +
void calibrateCCW()
{
	char userInput = NULL;

	while (1) // return statement inside case statement
	{
		turnByPercent(FULL);
		Serial.print("Is the motor turning COUNTER CLOCKWISE? y/n (or anything else to re-try)\n");

		while (!(Serial.available() > 0));
		userInput = Serial.read();

		switch (userInput)
		{
		case 'y':
		case 'Y':
			positive_CCW = true;
			Serial.print("Motor is set to positive mode\n");
			return;
			break;
		case 'n':
		case 'N':
			positive_CCW = false;
			Serial.print("Motor is set to negative mode\n");
			return;
			break;

		default:
			break;
		}

	}
}



bool readBool()
{
	Serial.println("(T/F)");
	while (!(Serial.available() > 0));
	char inputChar = Serial.read();
	while (Serial.available() > 0) Serial.read();
	switch (inputChar)
	{
	case 't':
	case 'T':
		return true;
	default:
		return false;
	}
}

int readInteger()
{
	Serial.println("(type an integer)");
	while (!(Serial.available() > 0));
	int inputInt = Serial.parseInt();
	while (Serial.available() > 0) Serial.read();
	return inputInt;
}

void printInteger(int value)
{
	Serial.println(value);

	return;
}

void printBool(int value)
{
	if (value)
		Serial.println("True");
	else
		Serial.println("False");

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