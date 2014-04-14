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
#define C_OX_SLOWVENT			'4'
#define C_OX_STEP_FORWARD		'5'
#define C_OX_STEP_REVERSE		'6'
#define C_OX_CHECKSTATE			'7'
#define C_OX_CHECKSPEED			'8'
//#define C_OX_CALIBRATE

// constants related to the control
#define OX_SPEED_TOACTUATE			255
#define OX_SPEED_TOVENT				-255
#define OX_COUNTS_VENTTOOFF			690
#define OX_COUNTS_ACTUATETOOFF		665
#define OX_COUNTS_VENTTOSLOWVENT	475
#define OX_COUNTS_ACTUATETOSLOWVENT	875
#define OX_COUNTS_OFFTOSLOWVENT		400
#define OX_COUNTS_SLOWVENTTOOFF		200
#define OX_COUNTS_DX				20	// minimum distance moved

namespace OxActuation
{
	enum oxidizerState
	{
		off = 0,
		vent = 1,
		actuate = 2,
		moving = 3, // not used in internal
		slowVent = 4,
		undefined = 5
	};

	void init(); // initialize the entire thing, place in setup()
	oxidizerState checkState(); // check the state of the oxidizer
	void moveTo(oxidizerState commandState); // main actuation function
	bool control();
	void moveStep(bool isForward);
	
	oxidizerState state; // current/target state
	oxidizerState prevState; // previous state
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
		case C_OX_SLOWVENT:
			Serial.println("Moving to SLOWVENT");
			OxActuation::moveTo(OxActuation::slowVent);
			break;
		case C_OX_STEP_FORWARD:
			Serial.println("Stepping toward ACTUATE");
			OxActuation::moveStep(true);
			break;
		case C_OX_STEP_REVERSE:
			Serial.println("Stepping toward VENT");
			OxActuation::moveStep(false);
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
	OxActuation::control();
	//Serial.print("Speed:");
	//Serial.print(OxActuation::motorSpeed);
	//Serial.print(" State:");
	//Serial.println(OxActuation::state);
	//Serial.print("prevstate: ");
	//Serial.println(OxActuation::prevState);
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
		prevState = state;
	}

	// the 'clean' version of checkState, returns moving if it's moving
	oxidizerState checkState()
	{
		if (motorSpeed != 0)
			return moving;
		else if (state == undefined)
			return undefined;
		return checkStateInternal();
	}

	// returns state of ox motor
	// does not take into the account the fact that motor is moving
	// blindly checks microswitches & assumes if neither are triggered
	// then motor is in OFF position
	// does not take into account slowVent/undefined state
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
		// if state is undefined, need to go to a defined position first
		if (state == undefined && commandState != vent && commandState != actuate)
		{
			Serial.println("WARNING: State Undefined, cannot moveTo commanded position");
			return;
		}
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
			if (state == vent || state == slowVent)
				motorSetSpeed(OX_SPEED_TOACTUATE); // away from vent
			if (state == actuate)
				motorSetSpeed(OX_SPEED_TOVENT); // away from actuate
			break;
		case slowVent:
			TCNT_ = 0;
			if (state == vent)
				motorSetSpeed(OX_SPEED_TOACTUATE); // away from vent
			if (state == actuate || state == off)
				motorSetSpeed(OX_SPEED_TOVENT); // away from actuate
			break;
		default:
			Serial.println("WARNING: Invalid argument in OxActuation::moveTo()");
			break;
		}
		prevState = state;
		state = commandState;
	}
	
	// manually move a step forward or backward
	// leaves the state in 'undefined'
	// to define the state again move to actuate/vent
	// forward is towards actuate
	void moveStep(bool isForward)
	{
		TCNT_ = 0;
		if (isForward)
			motorSetSpeed(OX_SPEED_TOACTUATE);
		else
			motorSetSpeed(OX_SPEED_TOVENT);
		state = undefined;
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
				Serial.println("Motor at VENT");
				return true;
			}
			break;
		case actuate:
			if (digitalRead(PIN_OX_SWITCHACTUATE) == LOW)
			{
				motorSetSpeed(0);
				Serial.println("Motor at ACTUATE");
				return true;
			}
			break;
		case off:
			if ((prevState == actuate && TCNT_ >= OX_COUNTS_ACTUATETOOFF) ||
				(prevState == vent && TCNT_ >= OX_COUNTS_VENTTOOFF) ||
				(prevState == slowVent && TCNT_ >= OX_COUNTS_SLOWVENTTOOFF))
			{
				motorSetSpeed(0);
				Serial.println("Motor at OFF");
				return true;
			}
			break;
		case slowVent:
			if ((prevState == actuate && TCNT_ >= OX_COUNTS_ACTUATETOSLOWVENT) ||
				(prevState == vent && TCNT_ >= OX_COUNTS_VENTTOSLOWVENT) ||
				(prevState == off && TCNT_ >= OX_COUNTS_OFFTOSLOWVENT))
			{
				motorSetSpeed(0);
				Serial.println("Motor at SLOWVENT");
				return true;
			}
			break;
		case undefined:
			if (TCNT_ >= OX_COUNTS_DX)
			{
				motorSetSpeed(0);
				Serial.println("Motor stepped");
				return true;
			}
			break;
		default:
			Serial.println("ERROR: Invalid target state in OxActuation::control()");
			break;
		}
		return false;
	}

	void initTimerCount()
	{
		TCCR5A = 0x00;        // using Timer 5 b/c T5 pin
		TCCR5B = 0x07;
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