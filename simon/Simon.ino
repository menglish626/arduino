#include	"pitches.h"
#include	<avr/pgmspace.h>
// state vars
const char STOPPED = 0;
const char RESTART = 1;
const char PLAYING = 2;
const char PLAYBACK = 3;

// pins
const PROGMEM char RESET = 0;
const PROGMEM char BLUE_LED = 10;
const PROGMEM char RED_LED = 2;
const PROGMEM char YELLOW_LED = 3;
const PROGMEM char GREEN_LED = 4;
const PROGMEM char BLUE_BUTTON = 5;
const PROGMEM char RED_BUTTON = 6;
const PROGMEM char YELLOW_BUTTON = 7;
const PROGMEM char GREEN_BUTTON = 8;
const PROGMEM char SPEAKER = 9;
//other const PROGMEMant
const int MAX_ROUNDS = 10;
const int NUM = 4;
const PROGMEM char LEDS[NUM] = { BLUE_LED, RED_LED, YELLOW_LED, GREEN_LED };
const PROGMEM char BUTTONS[NUM] = { BLUE_BUTTON, RED_BUTTON, YELLOW_BUTTON, GREEN_BUTTON };
const float TONES[NUM] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
const PROGMEM int BUZZ_LENGTH = 500; //seconds
const int MAX_WAIT = 100;

//songs
const PROGMEM int WIN_SONG_L = 10;
const PROGMEM int LOSE_SONG_L = 10;
const PROGMEM int LOSE_SONG[LOSE_SONG_L] = {0,1,3,2,1,2,3,1,2,0};
const PROGMEM int WIN_SONG[WIN_SONG_L] = {0,3,1,1,2,1,3,2,0,0};

//global variables
char state;
char buttons;
int curr_round;
int wait;
int num_rounds;
char game[MAX_ROUNDS];

void get_buttons()
	{
	int i;
	buttons = (char) 0;
	for(i=0; i<NUM; i++)
		{
		if(digitalRead(pgm_read_byte_near(BUTTONS + i)) == LOW)
			{
			buttons = (buttons & ((0x1 << i) ^ ((char) 0xFF))) | (0x1 << i);
			}
		}	
	}

void flash_and_buzz(int i)
	{
	digitalWrite(pgm_read_byte_near(LEDS + i), HIGH);
	tone(pgm_read_byte_near(&SPEAKER), TONES[i], pgm_read_byte_near(&BUZZ_LENGTH));
	delay(pgm_read_byte_near(&BUZZ_LENGTH));
	digitalWrite(pgm_read_byte_near(LEDS+ i), LOW);
	} // end flash_and_buzz

void flash_leds()
	{
	int i;
	for(i=0; i<NUM; i++)
		{
		if(buttons & (0x1 << i))
			{
			flash_and_buzz(i);
			}
		}
	}

void game_over()
	{
	Serial.println(F("Game over!"));
	int i;
	for(i=0; i<pgm_read_byte_near(&LOSE_SONG_L); i++)
		{
		flash_and_buzz(pgm_read_byte_near(LOSE_SONG+ i));
		}
	}

void winner()
	{
	Serial.println(F("Winner!"));
	int i;
	for(i=0; i<pgm_read_byte_near(&WIN_SONG_L); i++)
		{
		flash_and_buzz(pgm_read_byte_near(WIN_SONG+ i));
		}
	}

void(* reset) (void) = 0;

// setup()
void setup()
	{
	delay(1000);
	attachInterrupt(digitalPinToInterrupt(RESET), reset, LOW);
	Serial.begin(9600);
	Serial.println(F("Setup"));
	state = STOPPED;

	pinMode(A0,INPUT);
	randomSeed(analogRead(A0));

	pinMode(pgm_read_byte_near(&SPEAKER), OUTPUT);
	pinMode(pgm_read_byte_near(RESET), INPUT);

	int i;
	for(i=0; i<NUM; i++)
		{
		pinMode(pgm_read_byte_near(BUTTONS + i), INPUT);
		pinMode(pgm_read_byte_near(LEDS + i), OUTPUT);
		digitalWrite(pgm_read_byte_near(LEDS + i),LOW);
		}
	} //end setup

// loop()
void loop()
	{
	// [RESTART]
	//reset the device
	if(state == RESTART)
		{
		delay(500);
		state = PLAYBACK;
		num_rounds = 0;
		buttons = 0;
		return;
		}

	// [STOPPED]
	//device is stopped, just check for reset
	if(state == STOPPED)
		{
		while(true)
			{
			if(digitalRead(RESET) == LOW)
				{
				delay(500);
				Serial.println(F("RESET"));
				state = RESTART;
				return;
				}
			}
		}

	int i;

	// [RESET?]
	//check if reset button had been pressed generally
	if((state != STOPPED) && (digitalRead(RESET) == LOW))
		{
		Serial.println(F("STOPPED"));
		state = STOPPED;
		return;
		}
	else if(digitalRead(RESET) == LOW)
		{
		Serial.println(F("RESET"));
		state = RESTART;
		return;
		}
	
	// [PLAYBACK]
	// the pattern is being played for the user
	if(state == PLAYBACK)
		{
		Serial.println(F("PLAYBACK"));
		//extend the pattern
    	if(num_rounds < MAX_ROUNDS)
    		{
    		game[num_rounds++] = (char) random(0,4);
    		}
    
    	for(i=0; i<num_rounds; i++)
    		{
    		flash_and_buzz(game[i]);
    		delay(300);
    		}
		delay(100);
		curr_round = 0;
		wait = 0;
		state = PLAYING;
		return;
		}

	// [PLAYING]	
	// user is trying to recall the pattern
	if(state == PLAYING)
		{
		Serial.println(F("PLAYING"));
		Serial.println(wait);
		Serial.println(MAX_WAIT);
        //timeout
        if(wait++ == MAX_WAIT)
            {
            Serial.println(F("Timeout"));
            Serial.println(F("STOPPED"));
            game_over();
            state = STOPPED;
            return;
            }

        delay(1);
        get_buttons();
        if(!buttons){return;}
        flash_leds();

        // bad sequence
        if(buttons != (0x1 << game[curr_round]))
            {
            Serial.println(F("Bad Answer"));
            Serial.println(F("STOPPED"));
            game_over();
            state = STOPPED;
            return;
            }
        if((++curr_round == num_rounds) && (num_rounds == MAX_ROUNDS))
            {
            Serial.println(F("STOPPED"));
            winner();
            state = STOPPED;
            return;
            }
        if(curr_round == num_rounds)
            {
            delay(500);
            state = PLAYBACK;
            return;
            }
		wait = 0;
        return;
        } //end if

	} //end loop
