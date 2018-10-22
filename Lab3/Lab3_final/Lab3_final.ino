#define FFT_N 256 // set to 256 point fft
#define LOG_OUT 1 // use the log output function

#include <Servo.h> // include servo library
#include <FFT.h> // include the library
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
// #include "printf.h"

/*Set up nRF24L01 radio on SPI bus plus pins 9 & 10*/
RF24 radio(9, 10);

/* Radio pipe addresses for the 2 nodes to communicate.*/
const uint64_t pipes[2] = { 0x000000002ALL, 0x000000002BLL };

Servo servo_left; // pin 9
Servo servo_right; // pin 10

/*boolean which is true if a robot has been detected, false otherwise*/
bool sees_robot = false;

/*boolean which is true if 660Hz has been detected, false otherwise*/
bool detects_audio = false;

/*boolean which is true if the override button has been pressed, false otherwise*/
bool button_pressed = false;

/* Parameters hooked up to MUX which outputs to Analog pin A0
   Audio, IR connected to MUX
   Left, Middle, Right wall sensors are all connected to MUX
*/

 unsigned int data; // rf
 int x = 0; // current coordinates
 int y = 0;
 int heading; // direction


/*Line sensors*/
int sensor_left = A3;
int sensor_middle = A4;
int sensor_right = A5;

/*Initialize threshold values*/
int line_thresh = 400; //if below this we detect a white line
int wall_thresh = 150; //if above this we detect a wall
int IR_threshold = 160; //if above this we detect IR hat


/*Initializes the servo*/
void servo_setup() {
  servo_right.attach(10);
  servo_left.attach(9);
  servo_left.write(90);
  servo_right.write(90);
}

/*Go Straight*/
void go() {
  servo_left.write(100);
  servo_right.write(80);
}

/*Stop*/
void halt() {
  servo_left.write(90);
  servo_right.write(90);
}

/*Simple left turn*/
void turn_left() {
  servo_left.write(93);
  servo_right.write(70);
}

/*Simple right turn*/
void turn_right() {
  servo_left.write(110);
  servo_right.write(87);
}

/*Turns to the left in place*/
void turn_place_left() {
  servo_left.write(80);
  servo_right.write(80);
}


/*Turns to the right in place*/
void turn_place_right() {
  servo_left.write(100);
  servo_right.write(100);
}

/*Time it takes for wheels to reach intersection from the time the sensors detect the intersection*/
void adjust() {
  go();
  delay(700); //delay value to reach specification
  
}

void rf() {
  // format info into data
  
  data = data | y;
  data = data | x<<4;
  
  radio.stopListening();
  bool ok = radio.write( &data, sizeof(unsigned int) );

   if (ok)
      printf("ok...\n");
    else
      printf("failed.\n\r");

    // Now, continue listening
    radio.startListening();

    data = data & 0x00FF; // clears treasure/walls
}

void update_position() {
    switch (heading) {
        case 0: 
            y++;
            break;
        case 1: 
            x++;
            break;
        case 2:
            y--;
            break;
        case 3:
            x--;
            break;
    }
}

// assuming starting at bottom right facing north



void scan() {
    switch (heading) {
        case 0: // north
            if (check_left) data = data | 0x0100; // west=true
            if (check_front) data = data | 0x0200; // north=true
            if (check_right) data = data | 0x0400; // east=true
            break;
        case 1: // west
            if (check_left) data = data | 0x0800; // south=true
            if (check_front) data = data | 0x0100; // west=true
            if (check_right) data = data | 0x0200;// north=true
            break;
        case 2: // south
            if (check_left) data = data | 0x0400;// east=true
            if (check_front) data = data | 0x0800;// south=true
            if (check_right) data = data | 0x0100;// west=true
            break;
        case 3: // east
            if (check_left) data = data | 0x0200;// north=true
            if (check_front) data = data | 0x0400;// east=true
            if (check_right) data = data | 0x0800;// south=true
            break;
    }
}


/*Turns 90 degrees to the right until the middle sensor finds a line*/
void turn_right_linetracker() {
  turn_place_right();
  delay(300); //delay to get off the line
  /*Following while loops keep the robot turning until we find the line to the right of us*/
  while (analogRead(sensor_middle) < line_thresh);
  while (analogRead(sensor_middle) > line_thresh);
  heading--;
  if (heading == -1) heading = 3;
}

/*Turns to the left until a middle sensor finds a line*/
void turn_left_linetracker() {
  turn_place_left();
  delay(300); //delay to get off the line
  /*Following while loops keep the robot turning until we find the line to the left of us*/
  while (analogRead(sensor_middle) < line_thresh);
  while (analogRead(sensor_middle) > line_thresh);
  heading++;
  if (heading == 4) heading = 0;
}


/*
  Select output of mux based on select bits.
  Order is s2 s1 s0 from 000 to 111
  000 is audio
  001 is IR
  010
  011 is right wall sensor
  100
  101 is left wall sensor
  110
  111 is middle wall sensor
*/
void mux_select(int s2, int s1, int s0) {
  digitalWrite(11, s2); //MSB
  digitalWrite(12, s1);
  digitalWrite(13, s0); //LSB
  delay(10);
}

/*Returns true and turns on LED if there is a wall in front. */
bool check_left() {
  mux_select(1, 0, 1);
  if (analogRead(A0) > wall_thresh) {
    return true;
  } else {
    return false;
  }
}

/*Returns true and turns on LED if there is a wall in front. */
bool check_front() {
  mux_select(1, 1, 1);
  if (analogRead(A0) > wall_thresh) {
    return true;
  } else {
    return false;
  }
}

/*Returns true and turns on LED if there is a wall to the right. */
bool check_right() {
  mux_select(0, 1, 1);
  if (analogRead(A0) > wall_thresh) {
    return true;
  } else {
    return false;
  }
}



//NEED TO IMPLEMENT THIS
/*Sets button_pressed to true if we press our override button*/
//void button_detection() {
//  if () {
//    button_pushed = true;
//  }
//}


/*Sets detects_audio to true if we detect a 660Hz signal. Else does nothing*/
void audio_detection() {
  mux_select(0, 0, 0); //select correct mux output

  /*Set temporary values for relevant registers*/
  int temp1 = TIMSK0;
  int temp2 = ADCSRA;
  int temp3 = ADMUX;
  int temp4 = DIDR0;


  /*Set register values to required values for IR detection*/
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0

  cli();  // UDRE interrupt slows this way down on arduino1.0
  for (int i = 0 ; i < 512 ; i += 2) { // save 256 samples
    while (!(ADCSRA & 0x10)); // wait for adc to be ready
    ADCSRA = 0xf5; // restart adc
    byte m = ADCL; // fetch adc data
    byte j = ADCH;
    int k = (j << 8) | m; // form into an int
    k -= 0x0200; // form into a signed int
    k <<= 6; // form into a 16b signed int
    fft_input[i] = k; // put real data into even bins
    fft_input[i + 1] = 0; // set odd bins to 0
  }
  fft_window(); // window the data for better frequency response
  fft_reorder(); // reorder the data before doing the fft
  fft_run(); // process the data in the fft
  fft_mag_log(); // take the output of the fft
  sei();

  /*When audio is detected, detects_audio is set to true. Once detected
    this value is never set back to false*/
  for (byte i = 0; i < FFT_N / 2; i++) {
    if (i == 5 && fft_log_out[i] > 135) {
      detects_audio = true;
      digitalWrite(2, HIGH);
    }
  }

  /*Restore the register values*/
  TIMSK0 = temp1;
  ADCSRA = temp2;
  ADMUX = temp3;
  DIDR0 =  temp4;

  servo_setup();
}

/*Sets sees_Robot to true if there is a robot, else sees_robot = false*/
void IR_detection() {
  mux_select(0, 0, 1); //select correct mux output

  /*Set temporary values for relevant registers*/
  int t1 = TIMSK0;
  int t2 = ADCSRA;
  int t3 = ADMUX;
  int t4 = DIDR0;

  /*Set register values to required values for IR detection*/
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0

  cli();  // UDRE interrupt slows this way down on arduino1.0
  for (int i = 0 ; i < 512 ; i += 2) { // save 256 samples
    while (!(ADCSRA & 0x10)); // wait for adc to be ready
    ADCSRA = 0xf5; // restart adc
    byte m = ADCL; // fetch adc data
    byte j = ADCH;
    int k = (j << 8) | m; // form into an int
    k -= 0x0200; // form into a signed int
    k <<= 6; // form into a 16b signed int
    fft_input[i] = k; // put real data into even bins
    fft_input[i + 1] = 0; // set odd bins to 0
  }
  fft_window(); // window the data for better frequency response
  fft_reorder(); // reorder the data before doing the fft
  fft_run(); // process the data in the fft
  fft_mag_log(); // take the output of the fft
  sei();

  for (byte i = 0 ; i < FFT_N / 2 ; i++) {
    /*If there is a robot*/
    if (i == 43 && fft_log_out[i] > IR_threshold) {
      digitalWrite(7, HIGH);
      sees_robot = true;
    }
    /*If there is no robot detected (care about not seeing IR case because in our implementation we need to exit our lock)*/
    if (i == 43 && fft_log_out[i] < IR_threshold) {
      digitalWrite(7, LOW);
      sees_robot = false;
    }
  }

  /*Restore the register values*/
  TIMSK0 = t1;
  ADCSRA = t2;
  ADMUX = t3;
  DIDR0 =  t4;
}
/*follows the line the robot is on, else halts if all three sensors are not on a line*/
void linefollow() {
  if (analogRead(sensor_middle) < line_thresh) {

    go();
  }
  if (analogRead(sensor_left) < line_thresh) {
    turn_left();
  }
  if (analogRead(sensor_right) < line_thresh) {
    turn_right();
  }
  if (analogRead(sensor_right) > line_thresh && analogRead(sensor_left) > line_thresh && analogRead(sensor_middle) > line_thresh) {
    halt();
  }
}
/*Returns true if the robot is at an intersection, else false*/
bool atIntersection() {
  if ((analogRead(sensor_right) < line_thresh) && (analogRead(sensor_left) < line_thresh) && (analogRead(sensor_middle) < line_thresh)) {
    return true;
  }
  else {
    return false;
  }
}

/*Traverses a maze via right hand wall following while line following*/
void maze_traversal() {

  /*If there is a robot avoid it!!*/
  if (sees_robot) {

    /*Turn right until we see a line*/
    turn_right_linetracker();
    /*Ensure that the line we pick up isn't gonna run us into a wall*/
    while (check_front()) {
      turn_right_linetracker();
    }
  }

  /*If there is NO ROBOT then traverse the maze via right hand wall following*/
  else {

    /*If we are at an intersection*/
    if (atIntersection()) {

      /*Check if there is a wall to the right of us*/
      if (!check_right()) {
        adjust();
        update_position();
        rf();
        turn_right_linetracker();
      }

      /*If there is no wall to the right of us and no wall in front of us */
      else if (!check_front()) {
        adjust(); //adjust here takes us off the intersection allowing us to linefollow
        update_position();
        rf();
      }

      /*There IS A WALL to the right of us AND in front of us*/
      else {
        adjust();
        update_position();
        rf();
        turn_left_linetracker();
        /*Following if statement allows for robot to turn around at dead end*/
        if (check_front()) {
          turn_left_linetracker();
        }
      }
    }
    /*If we are not at an intersection then line follow*/
    linefollow();
  }
}

/*Set up necessary stuff*/
void setup() {
  Serial.begin(115200); // use the serial port
  servo_setup();
  pinMode(2, OUTPUT); // LED to indicate whether a wall is to the front or not
  pinMode(3, OUTPUT); // LED to indicate whether a wall is to the right or not
  pinMode(7, OUTPUT); // LED to indicate whether there is a robot in front of us

  //MUX SELECT PINS
  pinMode(11, OUTPUT); //S2 - MSB
  pinMode(12, OUTPUT); //S1
  pinMode(13, OUTPUT); // S0 - LSB

  // Setup and configure rf radio
  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15, 15);
  radio.setAutoAck(true);
  // set the channel
  radio.setChannel(0x50);
  // set the power
  // RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm.
  radio.setPALevel(RF24_PA_MAX);
  //RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.setDataRate(RF24_2MBPS);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(8);

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
}

/*Main code to run*/
void loop() {
  /*Loop until we hear a 660Hz signal. Loop allows us to skip audio detection code on reiteration once the signal
    has been detected*/
  while (!detects_audio) { //UPDATE ONCE BUTTON OVERRIDE IS IN PLACE
    audio_detection();
  }
  IR_detection(); //update sees_robot
  maze_traversal(); //traverse the maze
}




