/*
Fruit dehydratation system simple version - fds_simple

Grigorije Vesic EE10/2015
Software development started 6.8.2023.
*/


/*Libraries*/

/*2x16 LCD display*/
#include <LiquidCrystal.h>
/*I2C library*/
#include <Wire.h>



/*------------------------ MACROS -------------------------*/

#define TEMP_DEFAULT 55
#define HOURS_DEFAULT 24
#define HEAT_RANGE 10
#define HUMI_RANGE 100
/*All temps are multipled by 10 that is why HEAT_RANGE is 10 -> 1C*/
/*All humidities are multipled by 10 that is why HUMI_RANGE is 100 -> 10%*/


#define DHTPIN 2
#define HEATER_PIN A1
#define CIRCULATION_PIN A2
#define COOLER_PIN A3

#define DS3231_I2C_ADDRESS 0x68
#define changeHexToInt(hex) ((((hex)>>4)*10)+((hex)%16))



/*--------------------- GLOBAL VARIABLES -------------------*/

byte target_temp = TEMP_DEFAULT;
byte target_hours = HOURS_DEFAULT;
byte target_feature = 1;
bool start_sig = 0;

byte ds3231_Store[7] = {0};
byte init3231_Store[7]={0x01,0x01,0x00,0x01,0x01,0x01,0x01};

bool heater_permit = 1;
bool cooler_permit = 0;

LiquidCrystal lcd(8,9,4,5,6,7);


//------------------- function declarations -----------------

void system_idle(byte *);
void system_running(void);

byte button_read(void);

void set_temp(byte *);
void set_hours(byte *);
void set_feature(byte *);

int dht_read(bool);

void DS3231_settime(void);
void DS3231_init(void);
void DS3231_Readtime(void);

void heater_control(int *);
void circulation_control(void);
void back_cooler_control(int *);

//------------------------ SETUP FUNCTION -------------------------

void setup() {

  //LIBRARY INITIALIZATION
  Wire.begin();
  lcd.begin(16,2);
  
  lcd.setCursor(0,0);
  lcd.print("System starting..");
  DS3231_init();
  delay(1000);
  lcd.clear();
  // put your setup code here, to run once:

  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);//relay module is turned off with HIGH signal

  pinMode(CIRCULATION_PIN, OUTPUT);
  digitalWrite(CIRCULATION_PIN, LOW);

  pinMode(COOLER_PIN, OUTPUT);
  digitalWrite(COOLER_PIN, LOW);
}



//---------WHILE(1)----------

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


  //SYSTEM OPERATING IN REGARD START/STOP
  if(start_sig == 1)
  {
    //SYSTEM RUNNING
    system_running();
    
  }
  else
  {
    //SYSTEM IDLING
    system_idle(&button);

  }

}

//--------------------- SYSTEM OPERATING FUNCTIONS -----------------------

//system running
void system_running(void)
{
    lcd.print("System working..");

    //maybe better with independent variable and pointer?????
    int temperature;
    temperature = dht_read(0);//returning temperature for 0

    int humidity;
    humidity = dht_read(1);//reurning humidity for 1
    
    lcd.setCursor(0,1);
    lcd.print("T=");
    lcd.setCursor(2,1);
    lcd.print(temperature);

    //HEATER CONTROL FUNCTION    
    heater_control(&temperature);

    //CIRCULATION FAN CONTROL FUNCTION
    circulation_control();

    //BACK-COOLER CONTROL FUNCTION
    back_cooler_control(&humidity);

    DS3231_Readtime();    
    lcd.setCursor(7,1);
    lcd.print("t=");
    byte t = ds3231_Store[0]; //accessing global variable
    lcd.setCursor(9,1);
    lcd.print(t);
    delay(600);  
  
}


//system idling
void system_idle(byte *btn)
{
  byte tmp_btn = *btn;

  //SYSTEM IDLING
    lcd.setCursor(1,1);
    lcd.print("T[C]=");
    lcd.setCursor(6,1);
    lcd.print(target_temp);

    lcd.setCursor(9,1);
    lcd.print("t[h]=");
    lcd.setCursor(14,1);
    lcd.print(target_hours);
  
    if(tmp_btn!=0) lcd.clear();
  
}

//back cooler control function
void back_cooler_control(int *humi)
{
  //COOLER_PIN is A3
  int curr_humi = *humi;
  int humi_limit = 600;//should use macro here
  //cooling of backplate works regarding to humidity
  //cooler system does not work until determined humidity range is exceeded, initial cooler_permit = 0
  //NOT TESTED with LED
  if(curr_humi > (humi_limit+HUMI_RANGE) && cooler_permit == 1)
    {
      digitalWrite(COOLER_PIN, HIGH); //set cooler on
      //relay module turns relay ON for LOW signal
    }
    else
    {
      digitalWrite(COOLER_PIN, LOW); //set heater off
      //relay module turns relay OFF for HIGH signal
    }

    if(curr_humi >= (humi_limit+HUMI_RANGE) && cooler_permit == 0) 
          cooler_permit = 1; //up range exceeded
    if(curr_humi <= (humi_limit-HUMI_RANGE) && cooler_permit == 1) 
          cooler_permit = 0; //down range exceeded
  
}


//circulation control function
void circulation_control(void)
{
  //circulation pin is A2
  if(start_sig == 1)
  {
    digitalWrite(CIRCULATION_PIN, HIGH);
  }
  else
  {
    digitalWrite(CIRCULATION_PIN, LOW);
  }  
}


//heater control function
//tested with RED LED on A1 and works
/*Code does not work with RelayModule directly connected to A1 and arduino 5V
because relay needs 100-200mA to switch. Relay needs to be supplied by separate
5V supply, most likely with LM7805 and some capacitors to sustain the surge*/
void heater_control(int *temp)
{
  //HEATER_PIN is A1
  int tmp_temp = *temp;
  int temp_limit = target_temp*10;
  //bool heater_permit;//should be global variable? initially set on 0
    if(tmp_temp < (temp_limit+HEAT_RANGE) && heater_permit == 1)
    {
      digitalWrite(HEATER_PIN, HIGH); //set heater on
      //relay module turns relay ON for LOW signal
    }
    else
    {
      digitalWrite(HEATER_PIN, LOW); //set heater off
      //relay module turns relay OFF for HIGH signal
    }

    if(tmp_temp >= (temp_limit+HEAT_RANGE) && heater_permit == 1) 
          heater_permit = 0; //up range exceeded
    if(tmp_temp <= (temp_limit-HEAT_RANGE) && heater_permit == 0) 
          heater_permit = 1; //down range exceeded
}


//button read
byte button_read()
{
  int tmp = analogRead(0);  
  if(tmp<730) delay(140); //button debouncing works for <1023 as well
  
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


//-------------------------- PARAMETERS SETTING -------------------------

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
      if(target_hours == 48) break; //max running hours is 48
      target_hours = target_hours + 1;
      break;
      }
    case 4: {
      lcd.print("Pressed: DOWN");
      if(target_hours == 6) break; //min running hours is 6
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



//------------------------------- DHT22 ----------------------------------------------

int dht_read(bool th)
{
    //internal bool variable for decision over Temperature or Humidity
    bool temp_humi = th;
    
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

    
    int h = 0;
    int t = 0;
    if ((j >= 39) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) {
                    //checkig cheksum last 8 bits
      h = data[0] * 256 + data[1];
      t = (data[2] & 0x7F)* 256 + data[3]; //first bit of 8 temp bits is temperature sign +/-
      
      if (data[2] & 0x80)  t *= -1; //first bit value 1 means negative temperature
  }

  //returning temperature for parameter 0 or humidity for parameter 1
  if(!temp_humi)  
        return t;
  if(temp_humi)
        return h;
}



//-------------------------------------- RTC -----------------------------------------

void DS3231_settime(void){

    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0); // set next input to start at the seconds register (register 0)
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
