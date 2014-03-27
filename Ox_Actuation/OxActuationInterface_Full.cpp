// [INTEGRATION] - instructions on how to integrate parts

// Defines
// [INTEGRATION] - concantenate these defines
// pin defines - check of these are not using tc1 for PWM
#define OX_P_MOTOR1			11
#define OX_P_MOTOR2			10

// position defines
#define OX_X_CLOSED1		0
#define OX_X_MAIN			525
#define OX_X_CLOSED2		1050
#define OX_X_VENT			1575
#define OX_X_MIN			0 // minimum count to be valid position
#define OX_X_MAX			2100 // max count to be valid

// displacement defines - are these needed?
#define OX_D_FULLTURN		2100
#define OX_D_HALFTURN		1050
#define OX_D_QUARTERTURN	525

// control defines
#define OX_C_MAXERROR		50 // if error to target is less than this, motor done moving
#define OX_C_MAXSPEED		255 // max speed of motor
#define OX_C_MINSPEED		0 // min speed of motor
// more to come, once the moving algorithm is set up

// Prototypes
// [INTEGRATION] - put prototypes above whereever the functions may be called
namespace OxActuation
{
	// public variables and functions
	uint16_t lastPosCount; // last known position of motor - to calibrate, change this (DO NOT change while motor is moving)
	bool isMoving; // true if motor is still in the process of moving to target position

	void moveTo(uint16_t target); // moves motor to position
	// use like: moveTo(OX_X_MAIN);
	
	// private variables and functions
	int velocity; // velocity in PWM counts, + or -
	uint16_t targetPosCount; // target position currently (or previously) moving towards
	
	uint16_t wrapPos(uint16_t pos); // wraparound to normalize position
	void moveInterrupt(); // this is the main control function, attached to interrupt when motor is moving but this isn't executing
	void motorSetSpeed(int pwmSpd); // actual function that sets the motor speed
}

void setup()
{
	// initialize vars
	OxActuation::isMoving = false;
	OxActuation::velocity = 0;
	
	// pseudo-calibration
	OxActuation::lastPosCount = 0;
}

void loop()
{
	// sample code
	OxActuation::moveTo(OX_X_CLOSED1);
	delay(1000);
	while(OxActuation::isMoving); // make sure it's not moving before trying to move
	
	OxActuation::moveTo(OX_X_MAIN);
	delay(1000);
	while(OxActuation::isMoving);
	
	OxActuation::moveTo(OX_X_CLOSED2);
	delay(1000);
	while(OxActuation::isMoving);
	
	OxActuation::moveTo(OX_X_VENT);
	delay(1000);
	while(OxActuation::isMoving);
}

// function definitions
// [INTEGRATION] - put with the rest of the function definitions
namespace OxActuation
{
	void moveTo(uint16_t target)
	{
		// check that target is within min and max of valid position count
		// set internal variables
		targetPosCount = target;
		// call moveInterrupt()
		moveInterrupt();
	}

	uint16_t wrapPos(uint16_t pos)
	{
		if (pos > OX_X_MAX)
			return pos %= OX_X_MAX; // to optimize just minus OX_P_MAX
		if (pos < 0)
			return pos = OX_X_MAX - (pos % OX_X_MAX);
		return pos;
	}
	
	void moveInterrupt()
	{
	// read counter
	// maths!
	// done yet?
	// set counter (check if already over?)
	// set motor
	}
	
	void motorSetSpeed(int pwmSpd)
	{
		if (pwmSpd >= 0)
		{
			analogWrite(OX_P_MOTOR1, pwmSpd);
			analogWrite(OX_P_MOTOR2, 0);
		}
		else
		{
			analogWrite(OX_P_MOTOR2, -pwmSpd);
			analogWrite(OX_P_MOTOR1, 0);
		}
	}
}
