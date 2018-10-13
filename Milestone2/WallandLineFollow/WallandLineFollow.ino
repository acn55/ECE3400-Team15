#include <Servo.h> // include servo library

// A0 is being used for the IR sensor

Servo servo_left; // pin 9
Servo servo_right; // pin 10

/*Initialize the parameters to corresponding pins*/
int wall_front = A1;
int wall_right = A2;


int sensor_left = A3;
int sensor_middle = A4;
int sensor_right = A5;

/*Initialize threshold values*/
int line_thresh = 400; //if below this we detect a white line
int wall_thresh = 150; //if above this we detect a wall


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

/*Wide left turn*/
void turn_left() {
  servo_left.write(93);
  servo_right.write(70);
}

/*Wide right turn*/
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
  // digitalWrite(7, HIGH);
  go();
  delay(700);
  // digitalWrite(7, LOW);
}

/*Turns 90 degrees to the right until the middle sensor finds a line*/
void turn_right_linetracker() {
  turn_place_right();
  delay(300);
  while (analogRead(sensor_middle) < line_thresh);
  while (analogRead(sensor_middle) > line_thresh);
}

/*Turns to the left until a middle sensor finds a line*/
void turn_left_linetracker() {
  turn_place_left();
  delay(300);
  while (analogRead(sensor_middle) < line_thresh);
  while (analogRead(sensor_middle) > line_thresh);
}

/*Returns true and turns on LED if there is a wall inf front. */
bool check_front() {
  if (analogRead(A1) > wall_thresh) {
    digitalWrite(2, HIGH);
    return true;
  } else {
    digitalWrite(2, LOW);
    return false;
  }
}

/*Returns true and turns on LED if there is a wall to the right. */
bool check_right() {
  if (analogRead(A2) > wall_thresh) {
    digitalWrite(3, HIGH);
    return true;
  } else {
    digitalWrite(3, LOW);
    return false;
  }
}

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

void the_follower() {
  check_front();
  check_right();
  if ((analogRead(sensor_right) < line_thresh) && (analogRead(sensor_left) < line_thresh) && (analogRead(sensor_middle) < line_thresh)) {
    if (!check_right()) {
      adjust();
      turn_right_linetracker();
    }
    else if (!check_front()) {
      adjust();

    }
    else {
      adjust();
      turn_left_linetracker();
      if(check_front()){
        turn_left_linetracker();
      }
    }
  }
  linefollow();
}
void setup() {
  Serial.begin(115200); // use the serial port
  servo_setup();
  pinMode(2, OUTPUT); // LED to indicate whether a wall is to the front or not
  pinMode(3, OUTPUT); // LED to indicate whether a wall is to the right or not
  pinMode(7, OUTPUT); // LED to indicate whether a wall is to the right or not
}

void loop() {
  the_follower();
}
