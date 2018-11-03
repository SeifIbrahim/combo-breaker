#include <Servo.h>
#include <Encoder.h>

const bool CW = false; //configure to true or false
const bool CCW = !CW;
const int SERVO_DEFAULT_THETA = 45;
const int SERVO_MIN_THETA = 0;
const int SERVO_MAX_THETA = 200;
const int SERVO_PIN = 9;
const int FB_SERVO = A0;
const int FB_DELTA = 10;
const int FB_TOLERANCE = 3.5;
const int FB_DELAY = 500;
//encoder pins
const int ENCODER1 = 3;
const int ENCODER2 = 2;
const int ENCODER_STEPS = 1200;
//stepper pins
const int DIR_PIN = 5;
const int STEP_PIN = 6;
const int DIGITS = 40;
const int MICROSTEPS = 8;//fraction on step on driver
const int STEPS = 200;
const int STEP_DELAY = 1600/MICROSTEPS;
const int BAUDRATE = 9600;

long prevPos, encoderOffset;
int mini, maxi, minFB, maxFB;
int combos[3][40];
float lb, hb, cur_digit = 0;
Servo shackle;
Encoder enc(ENCODER1, ENCODER2);

void setup() {
  Serial.begin(BAUDRATE);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(FB_SERVO, INPUT);
  shackle.attach(SERVO_PIN);
  prevPos = enc.read();
  encoderOffset = prevPos;
  
  setupServo();
}

float readEncoderDigit(){
  //Serial.print("Encoder At: ");
  //Serial.println(pfmod((enc.read() - encoderOffset), ENCODER_STEPS) * DIGITS / ENCODER_STEPS);
  return (pfmod((enc.read() - encoderOffset), ENCODER_STEPS) * DIGITS / ENCODER_STEPS);
}
int pmod(int a, int b){
  return (a % b + b) % b;
}

float pfmod(float a, float b){
  return fmod((fmod(a, b) + b), b);
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

  Serial.println(fb);
  for(i -= FB_DELTA; i > SERVO_MIN_THETA; i -= FB_DELTA){
    Serial.print("i=");
    Serial.print(i);
    shackle.write(i);
    mini = i;
    delay(FB_DELAY);
    minFB = analogRead(FB_SERVO);
    Serial.print(" fb=");
    Serial.println(minFB);
    if(fb - minFB < FB_DELTA){
      mini = i + (FB_DELTA*FB_TOLERANCE);
      Serial.println("Going up!");
      break;
    }
    fb = minFB;
  }
  for(i += 2*FB_DELTA; i <= SERVO_MAX_THETA; i += FB_DELTA){
    Serial.print("i=");
    Serial.print(i);
    shackle.write(i);
    maxi = i;
    delay(FB_DELAY);
    maxFB = analogRead(FB_SERVO);
    Serial.print(" fb=");
    Serial.println(maxFB);
    
    if(maxFB - fb < FB_DELTA){
      maxi = i - (FB_DELTA*FB_TOLERANCE);
      Serial.print("crap!! changing to ");
      Serial.println(maxi);
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
    delayMicroseconds(STEP_DELAY*10);
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

  cur_digit = readEncoderDigit();
  
  Serial.print("going to ");
  Serial.print(digit);
  Serial.print(" cur=");
  Serial.print(cur_digit);

  float diff;
  if(cw == CW){
    diff = DIGITS - digit + cur_digit;
  }else{
    diff = digit - cur_digit;
  }
  diff = pfmod(diff, DIGITS);

  Serial.print(" moving ");
  Serial.println(diff);
  step((int)(diff * (STEPS / DIGITS)));
}

bool enterPin(int p1, int p2, int p3){
  spinTo(p1, 3, CW);
  spinTo(p2, 1, CCW);
  spinTo(p3, 0, CW);
  return openLock();
}

bool openLock(){
  int fb;
  bool success = false;
  shackle.write(maxi + (FB_DELTA*FB_TOLERANCE*2.5));
  delay(300);
  fb = analogRead(FB_SERVO);
  if(fb - maxFB > 1.5*FB_DELTA*FB_TOLERANCE){
    Serial.println("SUCESS!");
    success = true;
  }else{
    success = false;
    Serial.println("Faliure.");
  }
  shackleMid();
  delay(1000);
  return success;
}

void getStickBoundries(int start){
  spinTo(start, 0, CCW);
  shackleUp();
  delay(1000);
  spinTo(start + 3, 0, CCW);
  delay(500);
  hb = readEncoderDigit();
  Serial.print("hb = ");
  Serial.println(hb);
  spinTo(start - 3, 0, CW);
  delay(500);
  lb = readEncoderDigit();
  Serial.print("lb = ");
  Serial.println(lb);
  shackleMid();
  delay(1000);
}

float getStickPoint(int start){
  getStickBoundries(start);
  if(lb > hb) return round(hb - (DIGITS - lb))/2;
  return round(lb + hb) / 2;
}

float getStickRange(int start){
  getStickBoundries(start);
  return hb - lb;
}

void dialCombos(){
  for(int i = 0; i < DIGITS; i++){
    for(int j = 0; j < DIGITS; j++){
      for(int k = 0; k < DIGITS; k++){
        if(!(combos[0][i]==-1 || combos[1][j]==-1 || combos[2][k]==-1)){
          if(enterPin(combos[0][i], combos[1][j], combos[2][k])){
            Serial.print("The combo is: ");
            Serial.print(combos[0][i]); Serial.print(" ");
            Serial.print(combos[1][j]); Serial.print(" ");
            Serial.print(combos[2][k]);
          }
        }
      }
    }
  }
}

int findResistance(){
  spinTo(0, 3, CW);
  delay(1000);
  Serial.println("DONE 3");
  shackle.write(maxi + (0.35*FB_DELTA*(FB_TOLERANCE-1)));
  spinTo(0, 1, CW);
  int ret = (int)(round(readEncoderDigit()));
  delay(1000);
  shackleMid();
  return ret;
}

/*void findCombos(){
  memset(combos, -1, sizeof(combos));
  bool found = false;
  
  //find 3rd digit
  for(int i = 0; i < DIGITS; i+=3){
    float f = getStickPoint(i);
    if(f > 16) f = 0;
    for(int j = f; j < DIGITS; j+=10){
      float s = getStickPoint(j);
      if(f != fmod(s, 10)){
        combos[2][0] = ((int)s == s) ? s : f;
        found = true;
        break;  
       }
    }
    if(found)break;
  }
  
  for(int i = (combos[2][0] + 2) % 4; i < DIGITS; i+=4){
    combos[1][i] = i;
  }
  for(int i = combos[2][0] % 4; i < DIGITS; i+=4){
    combos[0][i] = i;
  }
}

void findCombos2(){
  memset(combos, -1, sizeof(combos));
  int l1, l2, t1, t2;
  bool found = false;

  //find 1st digit
  combos[0][0] = findResistance() + 5;
  int mod = combos[0][0] % 4;
  
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
}*/

void findCombos3(){
  bool found = false;
  memset(combos, -1, sizeof(combos));

  Serial.println("Finding first number!");
  combos[0][0] = pmod(findResistance() + 5, DIGITS);
  Serial.print("First Number: ");
  Serial.println(combos[0][0]);

  Serial.println("Finding third number!");
  //find 3rd digit
  for(int i = 0; i < 13; i+=3){
    float f = getStickPoint(i);
    //if(f > 16) f = 0;
    Serial.print("First Stick point: ");
    Serial.println(f);
    for(int j = f + 10; j < DIGITS; j+=10){
      float s = getStickPoint(j);
      Serial.print(s);
      Serial.print(" ");
      if(f != fmod(s, 10)){
        combos[2][0] = ((int)s == s) ? s : f;
        found = true;
        break;  
       }
    }
    Serial.println();
    if(found)break;
  }
  if(!found) Serial.print("ERROR: Couldn't find 3rd digit");
  
  Serial.print("Third Number: ");
  Serial.println(combos[2][0]);

  Serial.print("Possible 2nd digits: ");  
  for(int i = (combos[2][0] % 4 == 2)? 0 : combos[2][0] % 4 + 2; i < DIGITS; i+=4){
    if(!((combos[2][0] + 2) % 40 == i || (combos[2][0] - 2) % 40 == i)){
      combos[1][i] = i;
      Serial.print(i);
      Serial.print(" ");
    }
  }
  Serial.println();
}

void loop(){
  // put your main code here, to run repeatedly:
  findCombos3();
  dialCombos();
  //Serial.println(readEncoderDigit());
 /* findCombos();
  dialCombos();
  findCombos2();
  dialCombos();*/
}

