#include <LiquidCrystal.h>

#define TEMP_INITIAL 55


int temperature = 55;

LiquidCrystal lcd(8,9,4,5,6,7);

byte button_read();

void setup() {
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("Hello world");
  delay(1000);
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  byte button = 0;
  button = button_read();
  delay(120); //delay works like software debouncing
  
  lcd.setCursor(0,0);
  switch(button)
  {
    case 1: {
      lcd.print("Pressed: SELECT");
      break;
      }
    case 2: {
      lcd.print("Pressed: LEFT");
      break;
      }
    case 3: {
      lcd.print("Pressed: UP");
      temperature = temperature + 1;
      break;
      }
    case 4: {
      lcd.print("Pressed: DOWN");
      temperature = temperature - 1;
      break;
      }
    case 5: {lcd.print("Pressed: RIGHT"); break;}
    default: {lcd.print("None btn pressed"); break;}
  }
  lcd.setCursor(0,1);
  lcd.print("TEMP = ");
  lcd.setCursor(7,1);
  lcd.print(temperature);
  
}

byte button_read()
{
  int tmp = analogRead(0);  

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

void set_temp()
{
  
  
}
