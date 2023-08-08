/*
Fruit dehydratation system simple version - fds_simple

Grigorije Vesic EE10/2015
Software development started 6.8.2023.

*/

/*2x16 LCD display*/
#include <LiquidCrystal.h>

/*I2C library*/
#include <Wire.h>

#define DS3231_I2C_ADDRESS 0x68
#define changeHexToInt(hex) ((((hex)>>4)*10)+((hex)%16))
#define SEK 0x00
#define MIN 0x01
#define SAT 0x02
#define DAY 0x03
#define DATE  0x04
#define MONTH 0x05
#define YEAR  0x06

byte ds3231_Store[7];
byte init3231_Store[7]={0x01,0x01,0x00,0x01,0x01,0x01,0x01};



#define TEMP_DEFAULT 55
#define HOURS_DEFAULT 24

#define DHTPIN 2

byte target_temp = TEMP_DEFAULT;
byte target_hours = HOURS_DEFAULT;
byte target_feature = 1;
bool start_sig = 0;

LiquidCrystal lcd(8,9,4,5,6,7);

byte button_read(void);

void set_temp(byte *);
void set_hours(byte *);
void set_feature(byte *);

int dht_read(void);

void DS3231_settime(void);
void DS3231_init(void);
void DS3231_Readtime(void);


void setup() {
  
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("System starting..");
  delay(1000);
  lcd.clear();
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

  byte button = 0;
  button = button_read(); //reading buttons
  
  
  lcd.setCursor(0,0);
  //set_feature(&button);


  if(button != 1){
    //start button - SELECT not pressed
    if(start_sig==0){
      lcd.print("Set parameters:");
      set_feature(&button);
      switch(target_feature)
      {
        case 1:{
         set_temp(&button);
         lcd.setCursor(0,1);
         lcd.print(">");
         break;
        }
        case 2:{
         set_hours(&button);
         lcd.setCursor(8,1);
         lcd.print(">");
         break;
        }
        default: break; 
      }
    }
  }
  else
  {
    //start button - SELECT pressed
    start_sig = 1;
    lcd.clear();
  }
  
  //SYSTEM RUNNING IMPLEMENTATION
  if(start_sig == 1)
  {
    lcd.print("System working..");
    int temperature;
    temperature = dht_read();
    lcd.setCursor(0,1);
    lcd.print("Measured T=");
    lcd.setCursor(11,1);
    lcd.print(temperature);
  }
  else
  {
    //SYSTEM IDLING
    lcd.setCursor(1,1);
    lcd.print("T[C]=");
    lcd.setCursor(6,1);
    lcd.print(target_temp);

    lcd.setCursor(9,1);
    lcd.print("H[h]=");
    lcd.setCursor(14,1);
    lcd.print(target_hours);
  
    if(button!=0) lcd.clear();
  }

}

byte button_read()
{
  int tmp = analogRead(0);  
  if(tmp<730) delay(120); //button debouncing works for <1023 as well
  
  if(tmp > 710 && tmp<730) //SELECT
    return 1;
  if(tmp > 470 && tmp<490) //LEFT
    return 2;
  if(tmp > 120 && tmp<140) //UP
    return 3;
  if(tmp > 300 && tmp<320) //DOWN
    return 4;
  if(tmp <5) //RIGHT
    return 5;

  return 0; //none button pressed
}

void set_temp(byte *btn)
{
  byte tmp_btn = *btn;
  switch(tmp_btn)
  {
    case 3: {
      lcd.print("Pressed: UP");
      if(target_temp == 70) break; //max temp is 70
      target_temp = target_temp + 1;
      break;
      }
    case 4: {
      lcd.print("Pressed: DOWN");
      if(target_temp == 20) break; //min temp is 20
      target_temp = target_temp - 1;
      break;
      }
    default: {break;}
  }
  
}

void set_hours(byte *btn)
{
  byte tmp_btn = *btn;
  switch(tmp_btn)
  {
    case 3: {
      lcd.print("Pressed: UP");
      if(target_hours == 48) break; //max hours to work is 48
      target_hours = target_hours + 1;
      break;
      }
    case 4: {
      lcd.print("Pressed: DOWN");
      if(target_hours == 6) break; //min hours to work is 6
      target_hours = target_hours - 1;
      break;
      }
    default: {break;}  
  }
}

void set_feature(byte *btn)
{
  byte tmp_btn = *btn;
  switch(tmp_btn)
  {
    case 2: {
      target_feature = 1;
      lcd.print("Pressed: LEFT");
      break;
      }
    case 5: {
      target_feature = 2;
      lcd.print("Pressed: RIGHT"); 
      break;
      }
    default: {break;}  
  }  
}


int dht_read(void)
{
    
    int data[100];
    int counter = 0;
    int laststate = HIGH;
    int j=0;

    pinMode(DHTPIN, OUTPUT);

    //waking up sensor
    digitalWrite(DHTPIN, HIGH);
    delay(50);
    digitalWrite(DHTPIN, LOW);//host sending start signal
    delay(10);//1-10ms

    pinMode(DHTPIN, INPUT);

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    //wait for pin to drop
    while(digitalRead(DHTPIN) == 1){
      delayMicroseconds(1); //waiting for around 20-40uS
      //usleep(1);
    }

    //read data 
    //80us LOW as response, and 80us HIGH for preparation to send data
    //first pulse is 50uS LOW, after 26-28us HIGH for 0, and 70us HIGH for 1
    //laststate is initially set on HIGH
    
    for (int i=0; i<100; i++){
      counter = 0;
        while(digitalRead(DHTPIN) == laststate){
          counter++;
          delayMicroseconds(1);
          if(counter == 100) break;
        }
      
      laststate = digitalRead(DHTPIN);
      if(counter == 100) break;

      if((i>3) && (i%2 == 0)){
        data[j/8] <<= 1; //shifting left by one place and inserting 0
          if(counter > 10) //if reading all zeros, reduce number to compare
          data[j/8] |= 1; //if counter is >10 -> pulse>28us and read value is 1
        j++;
      }
    }

    
    int h = 1;
    int t = 1;
    if ((j >= 39) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) {
                    //checkig cheksum last 8 bits
      h = data[0] * 256 + data[1];
      t = (data[2] & 0x7F)* 256 + data[3]; //first bit of 8 temp bits is temperature sign +/-
      
      if (data[2] & 0x80)  t *= -1; //first bit value 1 means negative temperature
  }

  return t;

}

void DS3231_settime(void){

    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0); // set next input to start at the SECONDS register (register 0)
    for(int i=0; i<=6; i++){
        Wire.write(ds3231_Store[i]);
    }
    Wire.endTransmission();
}

void DS3231_init(void){

    for(int i=0; i<=6; i++)
        ds3231_Store[i]=init3231_Store[i];

    DS3231_settime();
}

void DS3231_Readtime(void){

    //unsigned char time_data[7];
    //int fd =  wiringPiI2CSetup(0x68);
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0);
    Wire.endTransmission();
    Wire.requestFrom(DS3231_I2C_ADDRESS, 7);

    ds3231_Store[0] = Wire.read() & 0x7f; //sec
    ds3231_Store[1] = Wire.read() & 0x7f; //min
    ds3231_Store[2] = Wire.read() & 0x3f; //hour

    ds3231_Store[3] = Wire.read() & 0x07; //day
    ds3231_Store[4] = Wire.read() & 0x3f; //date
    ds3231_Store[5] = Wire.read() & 0x1f; //month
    ds3231_Store[6] = Wire.read() & 0xff; //year

    for(int i=0; i<=6; i++){
        ds3231_Store[i] = changeHexToInt(ds3231_Store[i]);
    }
}
