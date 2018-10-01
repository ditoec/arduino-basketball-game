//import necessary library
#include <TimerOne.h>

//Digit to 7 Seg conversion
#define D0 ~(0b11111010)
#define D1 ~(0b01100000)
#define D2 ~(0b10111100)
#define D3 ~(0b11110100)
#define D4 ~(0b01100110)
#define D5 ~(0b11010110)
#define D6 ~(0b11011110)
#define D7 ~(0b11100000)
#define D8 ~(0b11111110)
#define D9 ~(0b11100110)

//possible state of the program
#define STOP 0
#define RUNNING 1
#define FINISH 2

//define playing time in seconds
#define PLAYTIME 40

//define IO pins alias
#define START_BTN 2
#define SWITCH1   3
#define SWITCH2   4
#define SPEAKER   5

//This is how the 7 Segments are defined, each 7Seg has 1 shift register

// | 7Seg 1 | 7Seg 2 | 7Seg 3 | 7Seg 4 | 7Seg 5 | 7Seg 6 |
// |      Time       |     Player 1    |    Player 2     |

// LatchPin and ClockPin are connected in parallel to all shift register
// DataPin is connected to DS pin of 7Seg 1, then Q7* of 7Seg 1 is connected to DS pin of 7Seg 2, and so on
#define LATCHPIN  6 //Pin connected to ST_CP of 74HC595
#define CLOCKPIN  7 //Pin connected to SH_CP of 74HC595
#define DATAPIN   8 //Pin connected to DS of 74HC595 of 7Seg 1

//tone frequency definition, please change accordingly
#define START_TONE 300
#define PLAYER1_TONE 600
#define PLAYER2_TONE 800
#define END_TONE 1100

int game_state=STOP; //game state, initially STOP

volatile int refresh = 0; //flag for refresh display
volatile int secondCounter=0; //placeholder for 1/4 second counter

volatile int time=0; //placeholder for time

volatile int delaySwitch1=0; //placeholder for delaySwitch1 flag
volatile int delaySwitch2=0; //placeholder for delaySwitch2 flag

int player1=0; //placeholder for player 1 score
int player2=0; //placeholder for player 2 score

void setup() {
  //set the PIN functions
  pinMode(START_BTN,INPUT_PULLUP);
  pinMode(SWITCH1,INPUT_PULLUP);
  pinMode(SWITCH2,INPUT_PULLUP);
  pinMode(SPEAKER,OUTPUT);
  pinMode(LATCHPIN,OUTPUT);
  pinMode(CLOCKPIN,OUTPUT);
  pinMode(DATAPIN,OUTPUT);
  //initiate timer to tick every 250ms
  Timer1.initialize(250000);
  Timer1.attachInterrupt(timercall);
  Timer1.stop();
  refreshDisplay();
}

//check input with debouncing, I assume all the switch is active low (DRY CONTACT)
bool digitalReadBounce(int pin) {
  bool result = LOW;
  if(digitalRead(pin)==LOW){
    delay(50);
    if(digitalRead(pin)==LOW)
      result = HIGH;
  }
  return result;  
}

void loop() 
{
  //check current game state
  //if game is STOP/INIT, listen to START_BTN only to play the game and start timer
  if(game_state == STOP){
    if(digitalReadBounce(START_BTN)){
      game_state = RUNNING;
      tone(SPEAKER,START_TONE,1000);
      secondCounter=0;
      time=PLAYTIME;
      refreshDisplay();
      Timer1.start();
    }
  }
  //if the game is running, watch for SW1 and SW2 to count for score
  //watch for the game time, if game time reach a threshold, finish the game
  else if(game_state == RUNNING){
    int _time =0;
    int _delaySwitch1 =0;
    int _delaySwitch2 =0;
    int _refresh=0;
    noInterrupts();
    _delaySwitch1 = delaySwitch1;
    _delaySwitch2 = delaySwitch2;
    _time = time;
    _refresh = refresh;
    interrupts();
    if(digitalReadBounce(SWITCH1) && !_delaySwitch1){
      player1++;
      noInterrupts();
      delaySwitch1=1;
      interrupts();
      tone(SPEAKER,PLAYER1_TONE,500);
    }
    if(digitalReadBounce(SWITCH2) && !_delaySwitch2){
      player2++;
      noInterrupts();
      delaySwitch2=1;
      interrupts();
      tone(SPEAKER,PLAYER2_TONE,500);
    }
    if(_time==0){
      Timer1.stop();
      game_state = FINISH;
      tone(SPEAKER,END_TONE,2000);
    }
    if(_refresh){
      noInterrupts();
      refresh=0;
      interrupts();
      refreshDisplay();
    }   
  }
  //if the state of the game is FINISH, then pushing the START_BTN will reset the time and score to 0
  else if(game_state == FINISH){
    if(digitalReadBounce(START_BTN)){
      game_state = STOP;
      time = 0;
      player1 = 0;
      player2 = 0;
      refreshDisplay();
    }
  }
}

//this function refresh 7 segment display
void refreshDisplay(){
  //ground LATCHPIN and hold low for as long as we are transmitting
  digitalWrite(LATCHPIN, 0);
  //start transmitting data of 7Seg 6
  shiftOut(DATAPIN,CLOCKPIN,intTo7Seg(player2%10)); 
  //start transmitting data of 7Seg 5
  shiftOut(DATAPIN,CLOCKPIN,intTo7Seg(player2/10));
    //start transmitting data of 7Seg 4
  shiftOut(DATAPIN,CLOCKPIN,intTo7Seg(player1%10)); 
  //start transmitting data of 7Seg 3
  shiftOut(DATAPIN,CLOCKPIN,intTo7Seg(player1/10));
    //start transmitting data of 7Seg 2
  shiftOut(DATAPIN,CLOCKPIN,intTo7Seg(time%10)); 
  //start transmitting data of 7Seg 1
  shiftOut(DATAPIN,CLOCKPIN,intTo7Seg(time/10));
  //return the latch pin high to signal the chip that it 
  //no longer needs to listen for information
  digitalWrite(LATCHPIN, 1);  
}

byte intTo7Seg(int number){
  byte encoded = 0b00000000;
  switch(number){
    case 0: encoded = D0; break;
    case 1: encoded = D1; break;
    case 2: encoded = D2; break;
    case 3: encoded = D3; break;
    case 4: encoded = D4; break;
    case 5: encoded = D5; break;
    case 6: encoded = D6; break;
    case 7: encoded = D7; break;
    case 8: encoded = D8; break;
    case 9: encoded = D9; break;
  }
  return encoded;
}

void timercall()
{
  refresh = 1;
  secondCounter++;
  if(secondCounter==4){
    secondCounter=0;
    time--;
  }
  if(delaySwitch1){
    delaySwitch1++;
    if(delaySwitch1>4)delaySwitch1=0;
  }
  if(delaySwitch2){
    delaySwitch2++;
    if(delaySwitch2>4)delaySwitch2=0;
  }
}

void shiftOut(int myDataPin, int myClockPin, byte myDataOut) {
  // This shifts 8 bits out MSB first, 
  //on the rising edge of the clock,
  //clock idles low

  //internal function setup
  int i=0;
  int pinState;

  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

  //for each bit in the byte myDataOut�
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights. 
  for (i=7; i>=0; i--)  {
    digitalWrite(myClockPin, 0);

    //if the value passed to myDataOut and a bitmask result 
    // true then... so if we are at i=6 and our value is
    // %11010100 it would the code compares it to %01000000 
    // and proceeds to set pinState to 1.
    if ( myDataOut & (1<<i) ) {
      pinState= 1;
    }
    else {	
      pinState= 0;
    }

    //Sets the pin to HIGH or LOW depending on pinState
    digitalWrite(myDataPin, pinState);
    //register shifts bits on upstroke of clock pin  
    digitalWrite(myClockPin, 1);
    //zero the data pin after shift to prevent bleed through
    digitalWrite(myDataPin, 0);
  }

  //stop shifting
  digitalWrite(myClockPin, 0);
}
