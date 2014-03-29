/*
Ox actuation code interface
UTAT Rocketry 2013-2014
Hayden Lau, March 2014

The only public function is moveTo(). When moveTo is called with a position, it will set the motor moving towards the target position.
While the motor is moving, isMoving will be true. Do not directly change isMoving, or else the code will break. When the motor is finished
moving, isMoving will be set to false.

potential features in future:
-call function on finish moving motor

internal workings
This code uses interrupts heavily. When moveTo is called, the target position will be set along with some other "private" variables (see
'private' section). moveInterrupt() will then be called, which calculates how to move the motor a specified time to get to position. It
will then clear timer/counter1 and set it to count, as well as set it to interrupt (and call moveInterrupt()) at a specified number of counts
(i.e. when the motor has moved a displacement). Then, it will set the motor's velocity via PWM (analogWrite) and return. The next time
moveInterrupt() is called, it will re-calculate and move the motor more. Each time, moveInterrupt checks whether the motor is close enough
to the target position. If it is, this series of interrupts will stop and isMoving will be set to false.
*/

// position defines
#define OX_P_CLOSED1	0
#define OX_P_MAIN	525
#define OX_P_CLOSED2 1050
#define OX_P_VENT	1575

// displacement defines
#define OX_D_FULLTURN 2100
#define OX_D_HALFTURN 1050
#define OX_D_QUARTERTURN 525

// control defines
#define OX_C_ERRORMAX 50 // if error to target is less than this, motor done moving
// more to come, once the moving algorithm is set up

// public
uint16_t lastPosCount;
bool isMoving; // true if motor is still in the process of moving to target position
void moveTo(uint16_t target); // moves motor to position


// private

/* last known position of motor, in encoder counts
0 point to be calibrated to, closed position #1
*/
int velocity;
uint16_t targetPosCount;

// public
uint16_t wrapPos(uint16_t pos);
void moveInterrupt();
