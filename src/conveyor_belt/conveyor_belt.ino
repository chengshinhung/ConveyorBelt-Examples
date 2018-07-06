#include <FlexiTimer2.h>
#include <Adafruit_TCS34725.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include "Ultrasonic.h"

#define DEVICE_NAME           "ConveyorBelt\n"
#define SOFTWARE_VERSION      "V1.1.0\n"

rgb_lcd lcd;
Ultrasonic ultrasonic(10);

enum pick_mode_e {
  RED_MODE = 0,
  GREEN_MODE,
  YELLOW_MODE,
};

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);   // <! debug uart
  Serial.print( DEVICE_NAME );
  Serial.print( SOFTWARE_VERSION );
  Serial1.begin(115200);
  Serial2.begin(115200); // <! uarm b uart

  pinMode(5,INPUT_PULLUP); // key
  pinMode(12,INPUT_PULLUP);

  //initiate stepper driver
  pinMode(A2,OUTPUT);
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(4,OUTPUT);
  pinMode(A6,OUTPUT); //STEP
  pinMode(A7,OUTPUT); //DIR
  
  digitalWrite(A2,HIGH);
  digitalWrite(2,HIGH);//MS3
  digitalWrite(3,LOW);//MS2
  digitalWrite(4,LOW);//MS1
  digitalWrite(A6,LOW);
  digitalWrite(A7,LOW);

  Serial1.write("M2400 S0\n");
  Serial1.write("G0 X180 Y0 Z160 F2000000\n");
  Serial1.write("M2122 V1\n");              //report when finish the movemnet
  Serial2.write("M2400 S0\n");
  Serial2.write("G0 X180 Y0 Z160 F2000000\n");
  Serial2.write("M2122 V1\n");              //report when finish the movemnet

  /*if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }*/
  tcs.begin();
  lcd.begin(16, 2);
  lcd.setRGB(0, 0, 255);
  lcd.print(" conveyor belt");
  lcd.setCursor(0, 1);
  lcd.print("  count:");
}

int inByte = 0;//serial buf
void wait_uarm_move_done(){
  inByte=0;//clear the buffer
  while(inByte!='@'){
     if (Serial2.available() > 0) {
        inByte = Serial2.read();
     }
  }
}

void uarm_a_pick_up_down(){
  Serial1.write("G0 X69 Y-217 Z100 F2000000\n"); delay(100);
  Serial1.write("G2202 N3 V108\n"); delay(100);
  Serial1.write("G0 X69 Y-217 Z30 F2000000\n"); delay(1000);
  Serial1.write("M2231 V1\n");delay(100);
  Serial1.write("G0 X69 Y-217 Z100 F2000000\n");delay(2000);
  Serial1.write("G2202 N3 V90\n");delay(100);
  Serial1.write("G0 X156 Y0 Z100 F2000000\n"); delay(100);
  Serial1.write("G0 X156 Y0 Z52 F2000000\n"); delay(2000);
  Serial1.write("M2231 V0\n");delay(100);
  Serial1.write("G0 X165 Y0 Z100 F2000000\n");delay(100);
  Serial1.write("G0 X180 Y0 Z160 F2000000\n");delay(100); 
}

void uarm_b_pick_up_down(enum pick_mode_e mode){
  Serial2.write("G0 X157 Y59 Z80 F2000000\n");delay(100);
  Serial2.write("G0 X157 Y59 Z52 F2000000\n");delay(100);  
  Serial2.write("M2231 V1\n");delay(100);   
  Serial2.write("G0 X157 Y59 Z100 F2000000\n");delay(100);
  Serial1.write("G2202 N3 V90\n");delay(100);
  switch(mode){
    case RED_MODE :
      Serial2.write("G0 X160 Y180 Z100 F2000000\n");delay(100);
      Serial2.write("G0 X160 Y180 Z10 F2000000\n");delay(4000);  
      Serial2.write("M2231 V0\n");
      Serial2.write("G0 X160 Y180 Z160 F2000000\n");delay(100);   
      break;
    case GREEN_MODE:
      Serial2.write("G0 X160 Y230 Z100 F2000000\n");delay(100);
      Serial2.write("G0 X160 Y230 Z10 F2000000\n");delay(3500);  
      Serial2.write("M2231 V0\n");
      Serial2.write("G0 X160 Y230 Z160 F2000000\n");delay(100);            
      break;
    case YELLOW_MODE:
      Serial2.write("G0 X160 Y280 Z100 F2000000\n");delay(100);
      Serial2.write("G0 X160 Y280 Z10 F2000000\n");delay(4000);  
      Serial2.write("M2231 V0\n");
      Serial2.write("G0 X160 Y280 Z30 F2000000\n");delay(100);         
      break;
  } 
  Serial2.write("G0 X180 Y0 Z160 F2000000\n");   
}

void time_callback(void){
  static bool level_state = false;
  if( !level_state ){
    PORTF |= (1<<6);
    level_state = true;
  }else{
    PORTF &= (~(1<<6));
    level_state = false;
  } 
}

void belt_move(){
  FlexiTimer2::set(8, 1.0/100000, time_callback);
  FlexiTimer2::start();
}


bool is_red(uint16_t r, uint16_t g, uint16_t b){
  if( (r<1800) || (r>3000) ){
    return false;
  }
  if( (g<500) || (g>1500) ){
    return false;
  }
  if( (b<500) || (b>1300) ){
    return false;
  }
  return true;  
}

bool is_green(uint16_t r, uint16_t g, uint16_t b){
  if( (r<1800) || (r>2800) ){
    return false;
  }
  if( (g<2800) || (g>4500) ){
    return false;
  }
  if( (b<1100) || (b>2500) ){
    return false;
  }
  return true;
}

bool is_yellow(uint16_t r, uint16_t g, uint16_t b){
  if( (r<4000) || (r>6000) ){
    return false;
  }
  if( (g<3800) || (g>6000) ){
    return false;
  }
  if( (b<1300) || (b>3000) ){
    return false;
  }
  return true;
}


void loop() {
  uint16_t r, g, b, c;
  static bool work_state = false;
  static int pick_cnt = 0;
  static bool detect_switch_flag = false;
  tcs.getRawData(&r, &g, &b, &c);
  if( (digitalRead(12)==LOW) && (!work_state) ){
    Serial.print( "start\r\n" );
    work_state = true;
    uarm_a_pick_up_down();
    belt_move();
    detect_switch_flag = true;
  }
  if( is_red(r, g, b) ){
    Serial.print( "red block\r\n" );
    FlexiTimer2::stop();
    uarm_b_pick_up_down(RED_MODE); 
    work_state = false;
  }else if( is_green(r, g, b) ){
    Serial.print( "green block\r\n" );
    FlexiTimer2::stop();
    uarm_b_pick_up_down(GREEN_MODE);
    work_state = false; 
  }else if( is_yellow(r, g, b) ){
    Serial.print( "yellow block\r\n" );
    FlexiTimer2::stop();
    uarm_b_pick_up_down(YELLOW_MODE);
    work_state = false;
  }
  if( detect_switch_flag ){
    cli();
    static bool detect_flag = false;
    if( ultrasonic.MeasureInCentimeters() < 10 ){
      if( !detect_flag ){ 
        pick_cnt++; 
        detect_switch_flag = false;
        FlexiTimer2::set(5, 1.0/100000, time_callback);
        FlexiTimer2::start();
       }
      detect_flag = true;
    }else{
      detect_flag = false;
    }
    sei();
  }




  if( digitalRead(5)==LOW ){
    //belt_move_step(2000000);
    pick_cnt = 0;
    lcd.setCursor(8, 1);
    lcd.print("        ");
  }
  lcd.setCursor(8, 1);
  lcd.print(pick_cnt);

/*
  Serial.print("R: "); Serial.print(r, DEC); Serial.print(" ");
  Serial.print("G: "); Serial.print(g, DEC); Serial.print(" ");
  Serial.print("B: "); Serial.print(b, DEC); Serial.print("\r\n");
*/
  
}
