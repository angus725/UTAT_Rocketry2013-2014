// pin definitions
#define MOTOR_PIN1 11
#define MOTOR_PIN2 10
#define ENCODER_A 5

// motor definitions
#define FULLTURN 8400
#define FULL 100 // 1 turn all the way around
#define HALF 50
#define QUARTER 25

#define FULL_DEG 360 // 1 turn all the way around
#define HALF_DEG 180
#define QUARTER_DEG 90


//valve definitions
#define FILL_POSITION	0
#define CLOSED1		90
#define DRAIN_POSITION	180
#define CLOSED2		270

// motor speeds
#define STOP_SPEED 0
#define STALL_SPEED 50
#define LOW_SPEED 100
#define MED_SPEED 150
#define HIGH_SPEED 200
#define MAX_SPEED 255



extern uint16_t valve_angle;
extern bool positive_CCW;

namespace valveDirection
{
	bool calibrated;
	void changeBy(int i)
	{
		valve_angle += i;
		while (valve_angle >= 360)
			valve_angle -= 360;
		while (valve_angle < 0)
			valve_angle += 360;
		return;
	}
	void setTo(int i)
	{
		valve_angle = 0;
		changeBy(i);
	}

}

namespace valveFunctions
{
	void shutValve()
	{
		if (valve_angle != CLOSED1 || valve_angle != CLOSED2)
		{
			turnByPercent(QUARTER);
			valveDirection::changeBy(QUARTER_DEG);
		}
	}

	void fill_position()
	{
		switch (valve_angle)
		{
		case FILL_POSITION: return;

		case CLOSED1:
			turnByPercent(-QUARTER);
			valveDirection::changeBy(-QUARTER_DEG);
			return;

		case DRAIN_POSITION:
			turnByPercent(HALF);
			valveDirection::changeBy(HALF_DEG);
			return;
		case CLOSED2:
			turnByPercent(QUARTER);
			valveDirection::changeBy(QUARTER_DEG);

		default:
			break;
		}

	}
	void drain_position()
	{
		switch (valve_angle)
		{
		case DRAIN_POSITION: return;

		case CLOSED1:
			turnByPercent(QUARTER);
			valveDirection::changeBy(QUARTER_DEG);
			return;

		case FILL_POSITION:
			turnByPercent(HALF);
			valveDirection::changeBy(HALF_DEG);
			return;
		case CLOSED2:
			turnByPercent(-QUARTER);
			valveDirection::changeBy(-QUARTER_DEG);

		default:
			break;
		}

	}

}





void turnByPercent(int16_t percentOfFullTurn)
{

	uint16_t count = 0;
	TCNT1 = 0;  // reset the hardware counter
	// start the counting
	bitSet(TCCR1B, CS12);  // Counter Clock source is external pin
	bitSet(TCCR1B, CS11);  // Clock on rising edge  

	motorSetSpeed(MAX_SPEED);
	while (count < FULLTURN * percentOfFullTurn / 100)
		count = TCNT1;
	motorSetSpeed(STOP_SPEED);
	TCCR1B = 0;
	// stop the counting 
}

//turns valve by + or - degrees at max speed
void turnValveByValue(int16_t degrees)
{
	int direction = 0;
	if (degrees > 0)
		direction = 1;
	else if (degrees < 0)
		direction = -1;
	else
		return;

	uint16_t count = 0;
	TCNT1 = 0;  // reset the hardware counter
	// start the counting
	bitSet(TCCR1B, CS12);  // Counter Clock source is external pin
	bitSet(TCCR1B, CS11);  // Clock on rising edge  

	motorSetSpeed(MAX_SPEED * direction);
	while (count < FULLTURN / FULL_DEG * degrees)
		count = TCNT1;
	motorSetSpeed(STOP_SPEED);
	TCCR1B = 0;
	// stop the counting 
}

//turns valve by + or - tenthDegrees
//Speed should be between 0 and 255
void precisionTurn(int16_t tenthDegrees, int16_t speed)
{
	int direction = 0;
	if (tenthDegrees > 0)
		direction = 1;
	else if (tenthDegrees < 0)
		direction = -1;
	else
		return;

	if (speed > MAX_SPEED)
		speed = MAX_SPEED;
	uint16_t count = 0;
	TCNT1 = 0;  // reset the hardware counter
	// start the counting
	bitSet(TCCR1B, CS12);  // Counter Clock source is external pin
	bitSet(TCCR1B, CS11);  // Clock on rising edge  

	motorSetSpeed(speed * direction);
	while (count < FULLTURN / 3600 * tenthDegrees)
		count = TCNT1;
	motorSetSpeed(STOP_SPEED);
	TCCR1B = 0;
	// stop the counting 
}

void motorSetSpeed(int pwmSpd) {

	if (positive_CCW == false)
		pwmSpd = -pwmSpd;

	if (pwmSpd >= 0) {
		analogWrite(MOTOR_PIN1, pwmSpd);
		analogWrite(MOTOR_PIN2, 0);
		digitalWrite(LED_PIN, HIGH);
	}
	else {
		analogWrite(MOTOR_PIN2, -pwmSpd);
		analogWrite(MOTOR_PIN1, 0);
		digitalWrite(LED_PIN, LOW);
	}

}