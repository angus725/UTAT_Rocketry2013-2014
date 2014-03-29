// Oxidizer Actuation Code
// UTAT Rocketry 2013-2014
// works with Ox Actuation V3
// works on the Arduino Mega (Uno needs revision of pins and timer/counter code)
// Hayden Lau, March 2014

// modes for the two different arduinos
//#define UNO
#define MEGA

// pin defs
#ifdef MEGA
#define PIN_OX_SWITCHACTUATE	4
#define PIN_OX_SWITCHVENT		5
#define PIN_OX_A				2 // to actuate
#define PIN_OX_B				3 // to vent
#define PIN_OX_ENCODER			47
#define TCNT_					TCNT5
#endif
#ifdef UNO
#define PIN_OX_SWITCHACTUATE	4
#define PIN_OX_SWITCHVENT		2
#define PIN_OX_A				11
#define PIN_OX_B				3
#define PIN_OX_ENCODER			5
#define	TCNT_					TCNT1
#endif

// command defs for Serial interface
#define C_OX_VENT				'1'
#define C_OX_ACTUATE			'2'
#define C_OX_OFF				'3'
#define C_OX_CHECKSTATE			'4'
#define C_OX_CHECKSPEED			'5'
//#define C_OX_CALIBRATE

// constants related to the control
#define OX_SPEED_TOACTUATE		255
#define OX_SPEED_TOVENT			-255
//#define OX_SPEED_OFF			0
#define OX_COUNTSTOOFF_FROMVENT		725
#define OX_COUNTSTOOFF_FROMACTUATE	675

namespace OxActuation
{
	enum oxidizerState
	{
		off = 0,
		vent = 1,
		actuate = 2,
		moving = 3 // not used in internal
	};

	void init(); // initialize the entire thing, place in setup()
	oxidizerState checkState(); // check the state of the oxidizer
	void moveTo(oxidizerState commandState); // main actuation function
	bool control();

	oxidizerState state; // current/target state
	int motorSpeed; // keeps track of current motor speed
	void initTimerCount();
	oxidizerState checkStateInternal();
	void motorSetSpeed(int pwmSpd);
}

void setup()
{
	Serial.begin(9600);

	OxActuation::init();

	// print menu & state
}

void loop()
{
	if (Serial.available())
	{
		int serialByte = Serial.read();
		switch (serialByte)
		{
		case C_OX_VENT:
			// check if state is same as commanded
			Serial.println("Moving to VENT");
			OxActuation::moveTo(OxActuation::vent);
			break;
		case C_OX_ACTUATE:
			Serial.println("Moving to ACTUATE");
			OxActuation::moveTo(OxActuation::actuate);
			break;
		case C_OX_OFF:
			Serial.println("Moving to OFF");
			OxActuation::moveTo(OxActuation::off);
			break;
		case C_OX_CHECKSTATE:
			switch (OxActuation::checkState())
			{
			case OxActuation::vent:
				Serial.println("STATE: Vent");
				break;
			case OxActuation::actuate:
				Serial.println("STATE: Actuate");
				break;
			case OxActuation::off:
				Serial.println("STATE: Off");
				break;
			case OxActuation::moving:
				Serial.println("STATE: Moving");
				break;
			default:
				Serial.println("WARNING: Invalid state");
				break;
			}
			break;
		case C_OX_CHECKSPEED:
			Serial.println(OxActuation::motorSpeed);
			break;
		default:
			Serial.println("WARNING: Invalid command");
			break;
		}
		// if want to print status, print here
	}
	//Serial.print("Speed:");
	//Serial.print(OxActuation::motorSpeed);
	//Serial.print(" State:");
	//Serial.println(OxActuation::checkStateInternal());
	OxActuation::control();
	//Serial.println(TCNT_);
	//delay(100);
}

namespace OxActuation
{
	// inits everything
	void init()
	{
		// init pins
		pinMode(PIN_OX_SWITCHACTUATE, INPUT_PULLUP);
		pinMode(PIN_OX_SWITCHVENT, INPUT_PULLUP);
		pinMode(PIN_OX_A, OUTPUT);
		pinMode(PIN_OX_B, OUTPUT);
		pinMode(PIN_OX_ENCODER, INPUT);

		initTimerCount();
		motorSpeed = 0;
		state = checkStateInternal();
	}

	// the 'clean' version of checkState, returns moving if it's moving
	oxidizerState checkState()
	{
		if (motorSpeed != 0)
			return moving;
		return checkStateInternal();
	}

	// returns state of ox motor
	// does not take into the account the fact that motor is moving
	// blindly checks microswitches & assumes if neither are triggered
	// then motor is in OFF position
	oxidizerState checkStateInternal()
	{
		if (digitalRead(PIN_OX_SWITCHACTUATE) == LOW)
			return actuate;
		if (digitalRead(PIN_OX_SWITCHVENT) == LOW)
			return vent;
		return off;
	}

	// this sets the target state & starts the motor moving
	// clear counter if needed
	void moveTo(oxidizerState commandState)
	{
		// check if current state == command state
		if (commandState == state)
			return;
		switch (commandState)
		{
		case vent:
			motorSetSpeed(OX_SPEED_TOVENT); // towards vent
			break;
		case actuate:
			motorSetSpeed(OX_SPEED_TOACTUATE); // towards actuate
			break;
		case off:
			TCNT_ = 0;
			if (state == vent)
				motorSetSpeed(OX_SPEED_TOACTUATE); // away from vent
			if (state == actuate)
				motorSetSpeed(OX_SPEED_TOVENT); // away from actuate
			break;
		}
		state = commandState;
	}

	// polls to see if motor needs to be stopped
	// take into account motorSpeed - if motorspeed is 0 and target is off then assume off state
	// Off - check if count > limit, stop motor
	// Actuate or vent - check if microswitch triggered, stop motor
	bool control()
	{
		// check if moving
		if (motorSpeed == 0)
			return false;
		// check state
		switch (state)
		{
		case vent:
			if (digitalRead(PIN_OX_SWITCHVENT) == LOW)
			{
				motorSetSpeed(0);
				return true;
			}
			break;
		case actuate:
			if (digitalRead(PIN_OX_SWITCHACTUATE) == LOW)
			{
				motorSetSpeed(0);
				return true;
			}
			break;
		case off:
			if (motorSpeed > 0 && TCNT_ >= OX_COUNTSTOOFF_FROMVENT || motorSpeed < 0 && TCNT_ >= OX_COUNTSTOOFF_FROMACTUATE)
			{
				motorSetSpeed(0);
				return true;
			}
			break;
		}
		return false;
	}

	void initTimerCount()
	{
#ifdef MEGA
		TCCR5A = 0x00;        // using Timer 5 b/c T5 pin
		TCCR5B = 0x07;
#endif
#ifdef UNO
		TCCR1A = 0x00;
		bitSet(TCCR1B, CS12);  // Counter Clock source is external pin
		bitSet(TCCR1B, CS11);  // Clock on rising edge  
#endif
	}

	// positive is CCW
	// use with OX_SPEED_XX
	void motorSetSpeed(int pwmSpd)
	{
		// if speed is zero, this brakes
		motorSpeed = pwmSpd;
		if (pwmSpd >= 0)
		{
			analogWrite(PIN_OX_B, 0);
			analogWrite(PIN_OX_A, pwmSpd);
			Serial.print("PIN A:");
			Serial.println(pwmSpd);
		}
		else
		{
			analogWrite(PIN_OX_A, 0);
			analogWrite(PIN_OX_B, -pwmSpd);
			Serial.print("PIN B:");
			Serial.println(pwmSpd);
		}
	}
}