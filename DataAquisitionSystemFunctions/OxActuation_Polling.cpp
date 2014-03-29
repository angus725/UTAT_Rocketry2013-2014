#define PIN_OX_SWITCHACTUATE	9
#define PIN_OX_SWITCHVENT		8
#define PIN_OX_CW				11
#define PIN_OX_CCW				3

#define C_OX_VENT				'1'
#define C_OX_ACTUATE			'2'
#define C_OX_OFF				'3'
#define C_OX_CHECK				'4'
//#define C_OX_CALIBRATE

#define OX_STATE_VENT			1
#define OX_STATE_ACTUATE		2
#define OX_STATE_OFF			0
#define OX_STATE_MOVING			3
//#define OX_STATE_CALIBRATE

#define OX_SPEED_TOACTUATE		255
#define OX_SPEED_TOVENT			-255
//#define OX_SPEED_OFF			0

#define OX_COUNTSTOCENTER		525

namespace OxActuation
{
	int state; // current/target state
	int motorSpeed; // keeps track of current motor speed
	void init();
	int checkState();
	void moveTo(int commandState);
	void control();
	
	void initTimer1Count();
	int checkStateInternal();
	void motorSetSpeed(int pwmSpd);
}

void setup()
{
	Serial.begin(9600);
	pinMode(PIN_OX_SWITCHACTUATE, INPUT_PULLUP);
	pinMode(PIN_OX_SWITCHVENT, INPUT_PULLUP);
	pinMode(PIN_OX_CW, OUTPUT);
	pinMode(PIN_OX_CCW, OUTPUT);
	
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
				OxActuation::moveTo(OX_STATE_VENT);
				break;
			case C_OX_ACTUATE:
				Serial.println("Moving to ACTUATE");
				OxActuation::moveTo(OX_STATE_ACTUATE);
				break;
			case C_OX_OFF:
				Serial.println("Moving to OFF");
				OxActuation::moveTo(OX_STATE_OFF);
				break;
			case C_OX_CHECK:
				switch(OxActuation::checkState())
				{
					case OX_STATE_VENT:
						Serial.println("STATE: Vent");
						break;
					case OX_STATE_ACTUATE:
						Serial.println("STATE: Actuate");
						break;
					case OX_STATE_OFF:
						Serial.println("STATE: Off");
						break;
					case OX_STATE_MOVING:
						Serial.println("STATE: Moving");
						break;
					default:
						Serial.println("WARNING: Invalid state");
						break;
				}
			default:
				Serial.println("WARNING: Invalid command");
			break;
		}
		// if want to print status, print here
	}
	OxActuation::control();
}

namespace OxActuation
{
	void init()
	{
		initTimer1Count();
		motorSpeed = 0;
		state = checkStateInternal();
	}

	int checkState()
	{
		if (motorSpeed == 0)
			return OX_STATE_MOVING;
		return checkStateInternal();
	}

	// returns state of ox motor
	// does not take into the account the fact that motor is moving
	// blindly checks microswitches & assumes if neither are triggered
	// then motor is in OFF position
	int checkStateInternal()
	{
		if (digitalRead(PIN_OX_SWITCHACTUATE) == LOW)
			return OX_STATE_ACTUATE;
		if (digitalRead(PIN_OX_SWITCHVENT) == LOW)
			return OX_STATE_VENT;
		return OX_STATE_OFF;
	}

	// this sets the target state & starts the motor moving
	// clear counter if needed
	void moveTo(int commandState)
	{
		// check if current state == command state
		if (commandState == state)
			return;
		switch (commandState)
		{
			case OX_STATE_VENT:
				motorSetSpeed(OX_SPEED_TOVENT); // towards vent
			break;
			case OX_STATE_ACTUATE:
				motorSetSpeed(OX_SPEED_TOACTUATE); // towards actuate
			break;
			case OX_STATE_OFF:
				TCNT1 = 0;
				if (state == OX_STATE_VENT)
					motorSetSpeed(OX_SPEED_TOACTUATE); // away from vent
				if (state == OX_STATE_ACTUATE)
					motorSetSpeed(OX_SPEED_TOVENT); // away from actuate
			break;
		}
		state = commandState;
	}

	// polls to see if motor needs to be stopped
	// take into account motorSpeed - if motorspeed is 0 and target is off then assume off state
	// Off - check if count > limit, stop motor
	// Actuate or vent - check if microswitch triggered, stop motor
	void control()
	{
		// check if moving
		if (motorSpeed == 0)
			return;
		// check state
		switch(state)
		{
			case OX_STATE_VENT:
				if (digitalRead(PIN_OX_SWITCHVENT) == LOW)
					motorSpeed = 0;
			break;
			case OX_STATE_ACTUATE:
				if (digitalRead(PIN_OX_SWITCHACTUATE) == LOW)
					motorSpeed = 0;
			break;
			case OX_STATE_OFF:
				if (TCNT1 >= OX_COUNTSTOCENTER)
					motorSpeed = 0;
			break;
		}
	}

	void initTimer1Count()
	{
		TCCR1A = 0;        // reset timer/counter control register A
		bitSet(TCCR1B, CS12);  // Counter Clock source is external pin
		bitSet(TCCR1B, CS11);  // Clock on rising edge
	}

	// positive is CCW
	// use with OX_SPEED_XX
	void motorSetSpeed(int pwmSpd)
	{
		// if speed is zero, this brakes
		motorSpeed = pwmSpd;
		if (pwmSpd >= 0)
		{
			analogWrite(PIN_OX_CW, 0);
			analogWrite(PIN_OX_CCW, pwmSpd);
		}
		else
		{
			analogWrite(PIN_OX_CCW, 0);
			analogWrite(PIN_OX_CW, -pwmSpd);
		}
	}

	/*
	void calibrate()
	{
		
	}
	*/
}