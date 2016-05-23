#include <Servo.h>
#include <Encoder.h>

const bool CW = true; //configure to true or false
const bool CCW = !CW;
const int SERVO_DEFAULT_THETA = 30;
const int SERVO_MIN_THETA = 0;
const int SERVO_MAX_THETA = 200;
const int SERVO_PIN = 9;
const int FB_SERVO = A0;
const int FB_DELTA = 10;
const int FB_TOLERANCE = 3.5;
const int FB_DELAY = 500;
//encoder pins
const int ENCODER1 = 2;
const int ENCODER2 = 3;
const int ENCODER_STEPS = 1200;
//stepper pins
const int DIR_PIN = 5;
const int STEP_PIN = 6;
const int DIGITS = 40;
const int MICROSTEPS = 8;//fraction on step on driver
const int STEPS = 200;
const int STEP_DELAY = 1600/MICROSTEPS;
const int BAUDRATE = 9600;

int cur_digit = 0;
long prevPos;
int mini, maxi, minFB, maxFB;
int combos[3][40];
float lb, hb;
Servo shackle;
Encoder enc(ENCODER1, ENCODER2);

void setup() {
  Serial.begin(BAUDRATE);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(FB_SERVO, INPUT);
  shackle.attach(SERVO_PIN);
  prevPos = enc.read();
  
  setupServo();
}

void shackleUp(){
  shackle.write(maxi + (FB_DELTA*(FB_TOLERANCE-1)));
}

void shackleMid(){
  shackle.write(maxi);
}

//find servo positions
void setupServo(){
  int i = SERVO_DEFAULT_THETA;

  shackle.write(i);
  delay(500);
  int fb = analogRead(FB_SERVO);

  for(i -= FB_DELTA; i > SERVO_MIN_THETA; i -= FB_DELTA){
    shackle.write(i);
    mini = i;
    delay(FB_DELAY);
    minFB = analogRead(FB_SERVO);
    if(minFB - fb < FB_DELTA){
      mini = i + (FB_DELTA*FB_TOLERANCE);
      break;
    }
    fb = minFB;
  }
  for(i += 2*FB_DELTA; i <= SERVO_MAX_THETA; i += FB_DELTA){
    shackle.write(i);
    maxi = i;
    delay(FB_DELAY);
    maxFB = analogRead(FB_SERVO);
    if(maxFB - fb < FB_DELTA){
      maxi = i - (FB_DELTA*FB_TOLERANCE);
      shackleMid();
      break;
    }
    fb = maxFB;
  }
  
}

void step(int steps){
  for(int i = 0; i < steps*MICROSTEPS; i++){
    digitalWrite(STEP_PIN, LOW);
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_DELAY);
  }
  /*long curPos = enc.read();
  if(curPos != prevPos){
    prevPos = Math.abs(curPos%ENCODER_STEPS);
    //debug here
  }*/
}

void spinTo(int digit, int spins, bool cw){
  digitalWrite(DIR_PIN, cw? HIGH : LOW);
  for(int i = 0; i < spins; i++){
    step(STEPS);
  }
  int cur = digit;
  if(cw == CW){
    digit = DIGITS - digit + cur_digit;
  }else{
    digit = digit - cur_digit;
  }
  digit += DIGITS;
  digit %= DIGITS;
  cur_digit = cur;
  step(digit * (STEPS / DIGITS));
}

void enterPin(int p1, int p2, int p3){
  spinTo(p1, 3, CW);
  spinTo(p2, 1, CCW);
  spinTo(p3, 0, CW);
}

bool openLock(){
  int fb;
  bool success = false;
  shackle.write(maxi + (FB_DELTA*FB_TOLERANCE*1.5));
  delay(300);
  fb = analogRead(FB_SERVO);
  if(fb - maxFB > FB_DELTA*FB_TOLERANCE){
    Serial.println("SUCESS!");
    success = true;
  }else{
    success = false;
    Serial.println("Faliure.");
  }
  shackleMid();
  return success;
}



void getStickBoundries(int start){
  float low, high;
  spinTo(start, 0, CCW);
  shackleUp();
  spinTo(start - 3, 0, CW);
  lb = enc.read() / ENCODER_STEPS * DIGITS;
  spinTo(start - 3, 0, CW);
  hb = enc.read() / ENCODER_STEPS * DIGITS;
  shackleMid();
}

float getStickPoint(int start){
  getStickBoundries(start);
  return round(lb + hb) / 2;
}

float getStickRange(int start){
  getStickBoundries(start);
  return hb - lb;
}

void dialCombos(){
  for(int i = 0; i < sizeof(combos[0])/sizeof(int); i++){
    for(int j = 0; j < sizeof(combos[1])/sizeof(int); j++){
      for(int k = 0; k < sizeof(combos[2])/sizeof(int); k++){
        if(!(i==-1 || j==-1 || k==-1))enterPin(combos[0][i], combos[1][j], combos[2][k]);
      }
    }
  }
}

int findResistance(){
  spinTo(0, 3, CW);
  shackle.write(maxi + (FB_DELTA*(FB_TOLERANCE-2)));
  spinTo(0, 1, CW);
  return enc.read() / ENCODER_STEPS * DIGITS;
}

//returns 2d array
void findCombos(){
  memset(combos, -1, sizeof(combos));
  //find 3rd digit
  for(int i = 0; i < DIGITS; i+=3){
    float f = getStickPoint(i);
    for(int j = f; j < DIGITS; j+=10){
      float s = getStickPoint(j);
      if(f != fmod(s, 10)){
        combos[2][0] = s;
        break;
      }
    }
  }
  for(int i = combos[2][0] % 4 + 2; i < DIGITS; i+=4){
    combos[1][i] = i;
  }
  for(int i = combos[2][0] % 4; i < DIGITS; i+=4){
    combos[0][i] = i;
  }
}

void findCombos2(){
  memset(combos, -1, sizeof(combos));
  int l1, l2, t1, t2;
  int mod = combos[0][0] % 4;
  bool found = false;

  //find 1st digit
  combos[0][0] = findResistance() + 5;
    
  //find first 2 stick points
  for(int i = 0; i < 11; i+=3){
    float s = getStickPoint(i);
    if(s == (int)s){
      if(!l1){
        l1 = s;
      }else if(!l2){
        l2 = s;
        found = true;
      }
    }
    if(found) break;
  }
  if(!found) Serial.print("ERROR: Couldn't find stick points");

  //find 3rd digit
  for (int i = 0; i < 4; i++){
    
    if (((10 * i) + l1) % 4 == mod && !t1){
      t1 = ((10 * i) + l1) % 4;
    }else if(!t2){
      t2 = ((10 * i) + l1) % 4;
    }
    
    if (((10 * i) + l2) % 4 == mod  && !t1){
      t1 = ((10 * i) + l2) % 4;
    }else if(!t2){
      t2 = ((10 * i) + l2) % 4;
    }
    
  }
  if(getStickRange(t1) > getStickRange(t2)){
    combos[2][0] = t1;
  }else{
    combos[2][0] = t2;
  }
  
  //find 2nd digit
  for(int i = combos[2][0] % 4 + 2; i < DIGITS; i+=4){
    if(!((combos[2][0] + 2) % 40 == i || (combos[2][0] - 2) % 40 == i)){
      combos[1][i] = i;
    }
  }
}

void loop(){
  // put your main code here, to run repeatedly:
  findCombos();
  dialCombos();
  findCombos2();
  dialCombos();
}

