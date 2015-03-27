/*
 * automatic_popup.ino
 *
 * Created: 03-Dec-14 08:38:53 PM
 * Author: Mahmudur
 */ 
const uint8_t debug = 3;

// Led Pins
const int LED_EXT		= 24;
const int LED_12V		= 25;

// Relay Pins
const int relay_d0		= 7;
const int relay_d1		= 6;
const int relay_d2		= 5;
const int relay_d3		= 4;

// Current Measure Pin
const int CURRENT_MEASURE_PIN = A0;

int count = 0;
boolean led_state = false;

// serial variables
#define STRING_LENG		64
char inputString[STRING_LENG];
boolean rcvStateFlag = false;
byte	rcvCharCount = 0;

int count_upDown = 0;
boolean updown_test_flag = false;

// Motor related variables
enum MotorState
{
	MNONE = 0,
	FORWARD = 1,
	REVERSE = 2,
	STOP = 3,
};

enum MotorState motorState = MNONE;
boolean motor_moving_forward = false;
boolean motor_moving_reverse = false;
uint16_t motor_move_count = 0;

const uint16_t motorForwarMoveTimeLimit = 24;
const uint16_t motorReverseMoveTimeLimit = 5;
const uint16_t motorManualMoveTimeLimit = 50;
const uint16_t boardLowStateTimeLimit = 10;

// Board related variables
enum BoardPosition
{
	BNONE,
	BHIGH,
	BLOW,
};
enum BoardPosition boardPosition = BNONE;
uint16_t board_low_position_count = 0;
uint16_t board_low_position_sec_count = 0;

// current measure value
uint16_t adcValue = 0;
uint16_t sensorValue = 0;
long current = 0;

void sendDone(void){
	Serial.println("done");
}

void sendAck(void){
	Serial.println("ack");
}

void sendDeviceReady(void){
	Serial.println("Device Ready...");
}

void setSignalLED(boolean set)
{
	if(set)
	{
		digitalWrite(LED_BUILTIN,HIGH);
		digitalWrite(LED_EXT,HIGH);
	}
	else
	{
		digitalWrite(LED_BUILTIN,LOW);
		digitalWrite(LED_EXT,LOW);
	}
}

void setup(void)
{	
	Serial.begin(9600);
	// LED pins init
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(LED_EXT, OUTPUT);
	pinMode(LED_12V, OUTPUT);
	// Relay pins init
	pinMode(relay_d0, OUTPUT);
	pinMode(relay_d1, OUTPUT);
	pinMode(relay_d2, OUTPUT);
	pinMode(relay_d3, OUTPUT);
  
	motorState = MNONE;
	motor_moving_forward = false;
	motor_moving_reverse = false;
	motor_move_count = 0;
	boardPosition = BNONE;
	rcvStateFlag = false;
	rcvCharCount = 0;
	board_low_position_count = 0;
	board_low_position_sec_count = 0;
	adcValue = 0;
	sensorValue = 0;
	current = 0;
	
	setSignalLED(0);
	setMotorStop(1);
	
	sendDeviceReady();
}

void motorSoftStop(void)
{
	digitalWrite(relay_d0,LOW);
	digitalWrite(relay_d1,LOW);
	digitalWrite(relay_d2,LOW);
	digitalWrite(relay_d3,LOW);
}

void motorHardStop(void)
{
	digitalWrite(relay_d0,HIGH);
	digitalWrite(relay_d1,HIGH);
	digitalWrite(relay_d2,LOW);
	digitalWrite(relay_d3,LOW);
}

void setMotorStop(boolean set)
{
	motor_moving_forward = false;
	motor_moving_reverse = false;
	motor_move_count = 0;
	if (set)// hard
	{
		motorHardStop();
	}
	else // soft
	{
		motorSoftStop();
	}
	if (debug > 0)
	{
		Serial.println("Stopped");
	}
}

void motorForward(void)
{
	digitalWrite(relay_d0,HIGH);
	digitalWrite(relay_d1,LOW);
	digitalWrite(relay_d2,LOW);
	digitalWrite(relay_d3,HIGH);
}

void setMotorForward(void)
{
	motor_move_count = 0;
	boardPosition = BNONE;
	motorForward();
	motor_moving_forward = true;
	motor_moving_reverse = false;
}

void motorReverse(void)
{
	digitalWrite(relay_d0,LOW);
	digitalWrite(relay_d1,HIGH);
	digitalWrite(relay_d2,HIGH);
	digitalWrite(relay_d3,LOW);
}

void setMotorReverse(void)
{
	motor_move_count = 0;
	boardPosition = BNONE;
	motorReverse();
	motor_moving_reverse = true;
	motor_moving_forward = false;
}

void loop(void)
{
	//if(count_upDown < 10)
	//{
		//if(updown_test_flag == false)
		//{
			//updown_test_flag = true;
			//sendAck();
			//setMotorReverse();
			//motorState = REVERSE;
		//}
	//}
	delay(50);
	//count++;
	//if (count == 5)
	//{
		//count = 0;
		//if (led_state)
		//{
			//digitalWrite(LED_EXT,HIGH);
			//led_state = false;
		//}
		//else
		//{
			//digitalWrite(LED_EXT,LOW);
			//led_state = true;
		//}
	//}
	adcValue = analogRead(CURRENT_MEASURE_PIN);
	sensorValue = adcValue;
	if( sensorValue < 103 ) sensorValue = 102;
	else if( sensorValue > 921 ) sensorValue = 922;
	sensorValue -= 102;
	// calculate current using integer operation
	current = ( ( sensorValue * 49951 ) >> 10 ) - 20000;
	if((current > 1000 || current < -1000) && adcValue != 1023)
	{
		if (debug>2)
		{
			Serial.print("Current = ");
			Serial.print((int)current,DEC);  // prints current value
			Serial.print(" mA [");
			Serial.print(adcValue,DEC);  // prints the value read
			Serial.println("]");
		}
	}
	if (motor_moving_forward == true)
	{
		motor_move_count++;
		if(motor_move_count >= motorForwarMoveTimeLimit/2){
			if(current >= 4000){
				//motor_move_count = motorForwarMoveTimeLimit;				
				motorSoftStop();
				setSignalLED(1);
			}
		}
		if(motor_move_count == motorForwarMoveTimeLimit)
		{
			//updown_test_flag = false;
			motor_move_count = 0;
			boardPosition = BHIGH;
			motorState = STOP;
			setMotorStop(1);
			sendDone();
			if (debug>0)
			{
				Serial.println("MFMTLR");
			}
		}
	}
	if (motor_moving_reverse == true)
	{
		motor_move_count++;
		if (motor_move_count == motorReverseMoveTimeLimit)
		{
			motor_move_count = 0;
			boardPosition = BLOW;
			board_low_position_count = 0;
			motorState = STOP;
			setMotorStop(1);
			if (debug>0)
			{
				Serial.println("MRMTLR");
			}
			
		}
	}
	
	if (boardPosition == BLOW)
	{
		board_low_position_count++;
		if(board_low_position_count == boardLowStateTimeLimit){
			board_low_position_count = 0;
			motorState = FORWARD;
			setSignalLED(1);
			setMotorForward();
			if (debug>0)
			{
				Serial.println("BLPSTLR");
			}
		}
	}
}

void parseSerialCommands(const char * parseString, int length)
{
	if (debug>3)
	{
		Serial.println(parseString);
	}
	if(strncmp("down",parseString,4)== 0)
	{
		sendAck();
		setMotorReverse();
		motorState = REVERSE;
	}
	else if(strncmp("m:",parseString,2)== 0)
	{
		switch(parseString[2]){
			case 'F':
				motorForward();
				delay(motorManualMoveTimeLimit);
				motorSoftStop();
			break;
			
			case 'R':
				motorReverse();
				delay(motorManualMoveTimeLimit);
				motorSoftStop();
			break;
			
			case 'S':
				motorHardStop();
			break;
			
			case 's':
				motorSoftStop();
			break;
		}
		if (debug>0)
		{
			Serial.print("m:");
			Serial.println(parseString[2]);
		}
	}
	else if (strncmp("led:",parseString,4)== 0)
	{
		switch(parseString[4]){
			case '0':
				//12v led off
				digitalWrite(LED_12V,LOW);
			break;
			
			case '1':
				//12v led on
				digitalWrite(LED_12V,HIGH);
			break;
		}
		if (debug>0)
		{
			Serial.print("led:");
			Serial.println(parseString[2]);
		}
	}
}

void serialEvent()
{
	while (Serial.available())
	{
		// get the new byte:
		char inChar = (char)Serial.read();
		switch(rcvStateFlag)
		{
		
			case 0:
				if(inChar == '\n')
				{
					rcvStateFlag = true;
					rcvCharCount = 0;
				}
			break;
		
			case 1:
				if(inChar == '\r')
				{
					rcvStateFlag = false;
					inputString[rcvCharCount] = '\0';
					if(rcvCharCount>0){
						parseSerialCommands((const char *)inputString,rcvCharCount);
					}
					rcvCharCount = 0;
				}
				else
				{
					inputString[rcvCharCount++] = inChar;
					if(rcvCharCount > STRING_LENG)
					{
						rcvCharCount = 0;
						rcvStateFlag = false;
					}
				}
			break;
		}
	}
}
