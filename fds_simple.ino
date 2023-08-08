/*
Fruit dehydratation system simple version - fds_simple

Grigorije Vesic EE10/2015
Software development started 6.8.2023.

*/


#include <LiquidCrystal.h>

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
        data[j/8] <<= 1;
          if(counter > 10) //if reading zeros reduce number to compare
          data[j/8] |= 1; //initially j=0 means in first cycle of FOR loop data[0] gets value
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
