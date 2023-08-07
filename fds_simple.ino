/*
Fruit dehydratation system simple version - fds_simple

Grigorije Vesic EE10/2015
Software development started 6.8.2023.

*/


#include <LiquidCrystal.h>

#define TEMP_DEFAULT 55
#define HOURS_DEFAULT 24

byte target_temp = TEMP_DEFAULT;
byte target_hours = HOURS_DEFAULT;
byte target_feature = 1;
bool start_sig = 0;

LiquidCrystal lcd(8,9,4,5,6,7);

byte button_read(void);
void set_temp(byte *);
void set_hours(byte *);
void set_feature(byte *);

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
  button = button_read();
  
  
  lcd.setCursor(0,0);
  //set_feature(&button);


  if(button != 1){
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
    start_sig = 1;
    lcd.clear();
  }
  

  if(start_sig == 1)
  {
    lcd.print("System working..");
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
      if(target_temp == 70) break;
      target_temp = target_temp + 1;
      break;
      }
    case 4: {
      lcd.print("Pressed: DOWN");
      if(target_temp == 20) break;
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
      if(target_hours == 48) break;
      target_hours = target_hours + 1;
      break;
      }
    case 4: {
      lcd.print("Pressed: DOWN");
      if(target_hours == 6) break;
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
