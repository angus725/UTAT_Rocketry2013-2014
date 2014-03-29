#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif
#include "Wire.h"


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

class OxActuation
{
	public:
		OxActuation_Polling();
		int checkState();
		void moveTo(int commandState);
		void control();
	
	private:
		int state; // current/target state
		int motorSpeed; // keeps track of current motor speed
		void initTimer1Count();
		int checkStateInternal();
		void motorSetSpeed(int pwmSpd);
}