#include <wiringPi.h>
#include <softPwm.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 0

#if DEBUG
#define PR(a) printf(a);printf("\n")
#define PRVAR(text,var1,var2) printf(text,var1,var2); \
							       printf("\n")
#else
#define PR(a) 
#define PRVAR(text,var1,var2) 
#endif

#define NORMAL 0
#define PIN_RANGE_LOW 0
#define PIN_RANGE_HIGH 50
#define DELAY_TIME 1500
//clockwise
#define M_C_PIN 2
//counterclockwise
#define M_CC_PIN 1
#define MOTOR_ENABLED_PIN 0
//button pin
#define B_PIN 3
#define B_SD_PIN 25

//will define the states of the pins
typedef struct pinStates
{
	short ccPin, cPin;
} pinStates;

void interruptMotor(void);
void interruptShutDown(void);

short setUp(pinStates *pState);
void changeState(pinStates *pState);

//volatile: compile does not try to optizimze this allows for 
//better for unexpected changes
volatile short motor_flag = LOW;
volatile short running_flag = HIGH;

int main(void)
{
	pinStates pState;
	short error = setUp(&pState);//initializations: 
								 //setup of pins and states
	PR("Entering Loop");
	if(!error)
		while(running_flag)//shutoff interrupt
		{
			if(motor_flag) //motor interrupt
			{
				PRVAR("Pin States: %d %d",pState.ccPin,pState.cPin);
				changeState(&pState);
				PRVAR("Pin States: %d %d",pState.ccPin,pState.cPin);
				motor_flag = LOW;
			}
		}
	//shutoff motor
	digitalWrite(MOTOR_ENABLED_PIN, LOW);
	return error;
}

/**
* changes motor flag to HIGH
* will change direction
**/
void interruptMotor(void)
{
	PR("button press");
	delay(20);
	motor_flag = HIGH;
}

/**
* changes flag to shut down
**/
void interruptShutDown(void)
{
	PR("exitting...");
	running_flag = LOW;
}
/**
* changes the state of the pins
* changes the direction:
* sutdown motor, changes states, turns on motor.
**/
void changeState(pinStates *pStates)
{
	//turn motor off, set enabled low
	digitalWrite(MOTOR_ENABLED_PIN, LOW);
	delay(DELAY_TIME);
	//change direction
	pStates->cPin = !pStates->cPin;
	pStates->ccPin = !pStates->ccPin;
	
	digitalWrite(MOTOR_ENABLED_PIN, HIGH);
	//mult pin state by speed value if state is 0 pin is off
	softPwmWrite(M_CC_PIN,pStates->ccPin*PIN_RANGE_HIGH);
	softPwmWrite(M_C_PIN,pStates->cPin*PIN_RANGE_HIGH);
	delay(DELAY_TIME);
}

/**
* sets up the pin states wiring pi and other initialzations
* returns zero (NORMAL) if no error occurs and 1 for error.
**/
short setUp(pinStates *pState)
{
	short errorStatus = NORMAL;
	//setting initial state
	pState->ccPin = HIGH;
	pState->cPin = LOW;

 	//initialize wiringPi if failed not NORMAL (0) print error
 	// and set exitStatus to Abnormal termination '1'
	if(wiringPiSetup() == !NORMAL)
	{
		PR("wiring pi set up error");
		errorStatus = !NORMAL;
	}else
	{
		//setup pins
		pinMode(M_C_PIN, OUTPUT);
		pinMode(M_CC_PIN, OUTPUT);
		pinMode(MOTOR_ENABLED_PIN, OUTPUT);
		pinMode(B_PIN, INPUT);
		pinMode(B_SD_PIN, INPUT);
		
		//setting waiting for high signal to input pin
		pullUpDnControl(B_PIN, PUD_DOWN);
		pullUpDnControl(B_SD_PIN, PUD_DOWN);
		//software-driven PWM handler capable of outputting a PWM signal
		//this allows for the changing of motor speed
		if(softPwmCreate(M_C_PIN,PIN_RANGE_LOW, PIN_RANGE_HIGH) == !NORMAL)
		{
			PR("softPwmCreate error for mcpin");
			errorStatus = !NORMAL;
		}else if(softPwmCreate(M_CC_PIN,PIN_RANGE_LOW, PIN_RANGE_HIGH) == !NORMAL)
		{
			PR("softpwmCreate error for mccpin");
			errorStatus = !NORMAL;
		}else if(wiringPiISR (B_PIN, INT_EDGE_FALLING, &interruptMotor) == !NORMAL)
		{
			PR("wiringPiISR had an error setting up Motor Button");
			errorStatus = !NORMAL;
		}else if(wiringPiISR (B_SD_PIN, INT_EDGE_FALLING, &interruptShutDown) == !NORMAL)
		{
			PR("wringPiISR had an error setting up shutdown button");
			errorStatus = !NORMAL;
		}
	}
	return errorStatus;
}