/*
Fruit dehydratation system simple version - fds_simple

Grigorije Vesic EE10/2015
Software development started 6.8.2023.
*/


/*Libraries*/

/*2x16 LCD display library*/
#include <LiquidCrystal.h>

/*I2C library*/
#include <Wire.h>

/*WatchDog timer library*/
#include <avr/wdt.h>

/*EEPROM library*/
#include <EEPROM.h>

/*------------------------ MACROS -------------------------*/

#define TEMP_DEFAULT 55
#define HOURS_DEFAULT 24
#define HEAT_RANGE 10
#define HUMI_RANGE 100
#define HUMI_LIMIT 750
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
byte init3231_Store[7]={0x01,0x01,0x00,0x01,0x01,0x01,0x01};//setting time to 00:01:01, day 1

bool heater_permit = 1;
bool cooler_permit = 0;

int temperature = 0;
int humidity = 0;

bool rst_flag = 0;
bool rst_type = 0;

byte recovery_counter = 0;

LiquidCrystal lcd(8,9,4,5,6,7);


//------------------- function declarations -----------------------

void system_idle(byte *);
void system_running(void);

byte button_read(void);

void set_parameters(byte *);
void set_temp(byte *);
void set_hours(byte *);
void set_feature(byte *);
void start_system(byte *);
void idle_print(void);
void dht_read(void);

void DS3231_settime(void);
void DS3231_init(void);
void DS3231_Readtime(void);

byte time_display(void);

void heater_control(int *);
void circulation_control(void);
void back_cooler_control(int *);

void running_temp_print(void);
void running_humi_print(void);    


void hours_left_backup(byte *);
void target_temp_backup(void);
void rst_recovery(void);
void rst_recovery_manage(byte *);
void rst_print(void);

void clear_backup_data(void);
//------------------------ SETUP FUNCTION -------------------------

void setup() {

  //LIBRARY INITIALIZATION
  Wire.begin();
  lcd.begin(16,2);
  
  lcd.setCursor(0,0);
  lcd.print("System starting..");
  //DS3231_init();
  delay(500);
  //lcd.clear();
  
  // put your setup code here, to run once:

  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, HIGH);//relay module is turned off with HIGH signal


  pinMode(CIRCULATION_PIN, OUTPUT);
  digitalWrite(CIRCULATION_PIN, LOW);//air circulation is turned off

  pinMode(COOLER_PIN, OUTPUT);
  digitalWrite(COOLER_PIN, LOW);//back cooler is turned off

  //watchdog timer functionality
  wdt_disable();

  rst_recovery(); //loading values from rtc backup registers
  
  delay(3000); //important delay to avoid reboot loop
  wdt_enable(WDTO_8S); //set watchdog timer at 8 seconds


  lcd.clear();
}



//---------WHILE(1)----------

void loop() {
  // put your main code here, to run repeatedly:

  //reading buttons
  byte button = 0;
  button = button_read();


  if(rst_flag == 1){
    if(recovery_counter<200){
      rst_recovery_manage(&button);
      recovery_counter++;
      delay(60); //system will wait for 6 seconds after reset for some action from user, if not it will resume like it was automatic reset 
      if(rst_flag == 0) {recovery_counter = 0; goto label;}
    }
    else
    {
      rst_flag = 0;
      recovery_counter = 0;
      start_sig = 1; //system starts automatically after 6 seconds, if no user actions
    }
  }


  if(rst_flag == 0)
  {  
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
  //important label if user chooses user reset to prevent system from starting in system_idle function with default values
  label:
  
  /***********WATCHDOG TIMER RESET*********/
  wdt_reset();

}




//--------------------- SYSTEM OPERATING FUNCTIONS -----------------------

//system running
void system_running(void)
{
    dht_read();
    
    lcd.clear();
    lcd.print("RUNNING:");

    //TEMPERATURE PRINTING
    running_temp_print();
    
    //HUMIDITY PRINTING
    running_humi_print();
    
    
    //HEATER CONTROL FUNCTION    
    heater_control(&temperature);

    //CIRCULATION FAN CONTROL FUNCTION
    circulation_control();

    //BACK-COOLER CONTROL FUNCTION
    back_cooler_control(&humidity);

    //TIME DISPLAYING FUNCTION
    byte hours = time_display();
    if(hours == 0) start_sig = 0;
    
    hours_left_backup(&hours);//save remaining hours to rtc register at every iteration
    
    delay(1000); //if it is 200 code does not work??????? WHY???? PROBABLY INTERFERING WITH SENSOR oR RTC?
}



//system idling
void system_idle(byte *btn)
{
  byte tmp_btn = *btn;
  lcd.setCursor(0,0);
  
    
  heater_control(&temperature);
  circulation_control();
  back_cooler_control(&humidity);

  set_parameters(&tmp_btn);

  idle_print();

  start_system(&tmp_btn);
  
  if(tmp_btn!=0) lcd.clear();

}


//------------------------ SYSTEM FUNCTIONS -------------------------

void idle_print(void)
{
//SYSTEM IDLING
  lcd.setCursor(1,1);
  lcd.print("T[C]=");
  lcd.setCursor(6,1);
  lcd.print(target_temp);

  lcd.setCursor(9,1);
  lcd.print("t[h]=");
  lcd.setCursor(14,1);
  lcd.print(target_hours);
  
}

void start_system(byte *btn)
{
  byte tmp_btn = *btn;
  if(tmp_btn == 1)
  {
    //start, button = 1 -> SELECT pressed
    start_sig = 1;
    DS3231_init(); //INITIALIZE TIME AS 00:01:01 day 1 when system start working IMPORTANT
    lcd.clear();
    
    target_temp_backup(); //save determined target temperature to EEPROM, just once before start
  }  
}


void set_parameters(byte *btn)
{
  byte tmp_btn = *btn;
  if(tmp_btn != 1){
    //do not start, button != 1 -> SELECT not pressed
      
        lcd.print("Set parameters:");
        set_feature(&tmp_btn);
        switch(target_feature)
        {
          case 1:{
            set_temp(&tmp_btn);//SETTING OF TEMPERATURE
            lcd.setCursor(0,1);
            lcd.print(">");
            break;
            }
          case 2:{
            set_hours(&tmp_btn);//SETTING OF RUNNING HOURS
            lcd.setCursor(8,1);
            lcd.print(">");
            break;
            }
          default: break; 
        }
      
    }
}


void running_temp_print(void)
{
    //TEMPERATURE PRINTING
    byte t_main = 0;
    byte t_decimal = 0;
    t_main = temperature / 10;
    t_decimal = temperature % 10;

    lcd.setCursor(0,1);
    lcd.print("T=");
    lcd.setCursor(2,1);
    lcd.print(t_main);
    lcd.setCursor(4,1);
    lcd.print(".");
    lcd.setCursor(5,1);
    lcd.print(t_decimal);
    lcd.setCursor(6,1);
    lcd.print("C");    
}


void running_humi_print(void)
{
    //HUMIDITY PRINTING
    byte h_main = 0;
    byte h_decimal = 0;
    h_main = humidity / 10;
    h_decimal = humidity % 10;
    
    lcd.setCursor(8,1);
    lcd.print("H=");
    lcd.setCursor(10,1);
    lcd.print(h_main);
    lcd.setCursor(12,1);
    lcd.print(".");
    lcd.setCursor(13,1);
    lcd.print(h_decimal);
    lcd.setCursor(14,1);
    lcd.print("%");  
}


byte time_display(void)
{
    DS3231_Readtime();//current time read and placed into global array
    
    byte seconds = 0;
    byte minutes = 0;
    byte hours = 0;

    byte hours_print_tmp = 0;
    
    seconds = abs(ds3231_Store[0] - 59);
    minutes = abs(ds3231_Store[1] - 59);
    
    
    switch(ds3231_Store[3])
    {
      //just initialize time at 1 hour from the start???????? no, hours_tmp = hours - 1;
      case 1: {hours = target_hours - ds3231_Store[2]; break;} //eventually subtract 1 from target_hours - ds3231_Store[2], make hours int and STOP working at -1 hours value????
      case 2: {hours = (target_hours - ds3231_Store[2]) - 24; break;}
      case 3: {hours = (target_hours - ds3231_Store[2]) - 48; break;}
      default: {hours = 99; break;} //99 is code for error
    }

    hours_print_tmp = hours - 1;
    
    lcd.setCursor(8,0);
    lcd.print(hours_print_tmp);
    lcd.setCursor(10,0);
    lcd.print(":");
    lcd.setCursor(11,0);
    lcd.print(minutes);
    lcd.setCursor(13,0);
    lcd.print(":");
    lcd.setCursor(14,0);
    lcd.print(seconds);

    return hours;
}


//back cooler control function
void back_cooler_control(int *humi)
{
  if(start_sig == 1)
  {
  
  //COOLER_PIN is A3
  int curr_humi = *humi;
  //cooling of backplate works regarding to humidity
  /*cooler system does not work until limit is exceeded, after that it will turn off when humidity value is lower than limit - range
  after which it can only be turned back on again after limit is exceeded*/
  if(curr_humi > (HUMI_LIMIT-HUMI_RANGE) && cooler_permit == 1)
    {
      digitalWrite(COOLER_PIN, HIGH); //set cooler on
    }
    else
    {
      digitalWrite(COOLER_PIN, LOW); //set cooler off
    }

    //HUMI_LIMIT = 750, fixed value 75% humidity, HUMI_RANGE 100 -> 10%
    if(curr_humi >= HUMI_LIMIT) 
          cooler_permit = 1; //up range exceeded, permit turning on 
    if(curr_humi <= (HUMI_LIMIT-HUMI_RANGE) && cooler_permit == 1) 
          cooler_permit = 0; //down range exceeded, turn off cooler

  }
  else
  {
    digitalWrite(COOLER_PIN, LOW); //set cooler off 
  }
  
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
void heater_control(int *temp)
{
  
  if(start_sig == 1){
    //HEATER_PIN is A1
    int tmp_temp = *temp;
    int temp_limit = target_temp*10;
      //bool heater_permit;//should be global variable? initially set on 0
      if(tmp_temp < (temp_limit+HEAT_RANGE) && heater_permit == 1)
      {
        digitalWrite(HEATER_PIN, LOW); //set heater on
        //relay module turns relay ON for LOW signal
      }
      else
      {
        digitalWrite(HEATER_PIN, HIGH); //set heater off
        //relay module turns relay OFF for HIGH signal
      }

    if(tmp_temp >= (temp_limit+HEAT_RANGE) && heater_permit == 1) 
          heater_permit = 0; //up range exceeded
    if(tmp_temp <= (temp_limit-HEAT_RANGE) && heater_permit == 0) 
          heater_permit = 1; //down range exceeded
  }
  else
  {
    digitalWrite(HEATER_PIN, HIGH); //set heater relay off
  }

}


//button read
byte button_read()
{
  int tmp = analogRead(0);  
  if(tmp<730) delay(140); //button debouncing 140ms, works for tmp<1023 as well
  
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
      //lcd.print("Pressed: UP");
      if(target_temp == 70) break; //max temp is 70
      target_temp = target_temp + 1;
      break;
      }
    case 4: {
      //lcd.print("Pressed: DOWN");
      if(target_temp == 20) break; //min temp is 20
      target_temp = target_temp - 1;
      break;
      }
    default: {break;}
  }
  
}

void set_hours(byte *btn){
  
  byte tmp_btn = *btn;
  switch(tmp_btn)
  {
    case 3: {
      //lcd.print("Pressed: UP");
      if(target_hours == 48) break; //max running hours is 48
      target_hours = target_hours + 1;
      break;
      }
    case 4: {
      //lcd.print("Pressed: DOWN");
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
      //lcd.print("Pressed: LEFT");
      break;
      }
    case 5: {
      target_feature = 2;
      //lcd.print("Pressed: RIGHT"); 
      break;
      }
    default: {break;}  
  }  
}



//------------------------------- DHT22 ----------------------------------------------

void dht_read(void)
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

    
    int h = 0;
    int t = 0;
    if ((j >= 39) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) {
                    //checkig cheksum last 8 bits
      h = data[0] * 256 + data[1];
      t = (data[2] & 0x7F)* 256 + data[3]; //first bit of 8 temp bits is temperature sign +/-
      
      if (data[2] & 0x80)  t *= -1; //first bit value 1 means negative temperature
  }

    //placing temperature and humidity value into global variables
    humidity = h;
    temperature = t;

}



//-------------------------------------- RTC -----------------------------------------

void DS3231_settime(void)
{

    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0); // set next input to start at the SECONDS register (register 0)
    for(int i=0; i<=6; i++){
        Wire.write(ds3231_Store[i]);
    }
    Wire.endTransmission();
}

void DS3231_init(void)
{

    for(int i=0; i<=6; i++)
        ds3231_Store[i]=init3231_Store[i];

    DS3231_settime();
}

void DS3231_Readtime(void)
{
  
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


/************************** BACKUP AND RECOVERY FUNCTIONS ********************************/


void target_temp_backup(void)
{
    //DETERMINED TARGET TEMPERATURE TO BE SAVED TO EEPROM
    EEPROM.update(0, target_temp); //POSSIBLY BETTER TO USE EEPROM.update FUNCTION
    delay(10);//EEPROM takes 3.3ms to write into so i give him 10ms for any case
}


void hours_left_backup(byte *hrs)
{
    byte hours_left = *hrs;
    
    byte hours_div =  0;
    byte hours_mod = 0;
    hours_div = hours_left/10;
    hours_mod = hours_left%10;

    hours_left = 0;
    hours_left = ((hours_div << 4) | hours_mod);
    
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0x06); //set adress for first write
    Wire.write(hours_left); //saving remaining hours to YEAR register
    
    Wire.endTransmission();
}


//recovery function needs to be called after wdt_disable and before wdt_enable
void rst_recovery(void)
{
  
    byte temp_backup = 0;
    temp_backup = EEPROM.read(0);
    
    byte hours_backup_tens = 0;
    byte hours_backup_ones = 0;
    
    //we need to read backup register
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0x06);
    Wire.endTransmission();
    Wire.requestFrom(DS3231_I2C_ADDRESS, 1);//reading only 1 register, YEAR register

    byte hours_left = 0;
    hours_left = Wire.read(); //all hours left
    
    hours_backup_tens = (hours_left & 0xf0) >> 4;  //taking higher 4 bits
    hours_backup_ones = hours_left & 0x0f;         //taking lower 4 bits
    
    hours_backup_tens = changeHexToInt(hours_backup_tens);
    hours_backup_ones = changeHexToInt(hours_backup_ones);

    byte hours_backup = hours_backup_tens*10 + hours_backup_ones;//calculating remaining hours
    
    if(hours_backup >= 1)
    {
      target_temp  = temp_backup;
      target_hours = hours_backup;
      rst_flag = 1;
    }
    lcd.clear();
    lcd.print("TEMP:");
    lcd.setCursor(6,0);
    lcd.print(temp_backup);
    lcd.setCursor(0,1);
    lcd.print("HOURS_LEFT:");
    lcd.setCursor(12,1);
    lcd.print(hours_backup);
    delay(1000);
}


void rst_print(void)
{
  //lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("USER RESET ?");
  lcd.setCursor(4,1);
  lcd.print("YES");
  lcd.setCursor(9,1);
  lcd.print("NO");  

  if(rst_type == 0)
  {
      lcd.setCursor(3,1);
      lcd.print(">");
  }

  if(rst_type == 1)
  {
      lcd.setCursor(8,1);
      lcd.print(">");
  }
  
}


void rst_recovery_manage(byte *btn)
{
      byte tmp_btn = *btn;
      
      //CHOOSING RESET TYPE
      if(tmp_btn != 1)
      {
        switch(tmp_btn)
        {
          case 2: {
            rst_type = 0;
            lcd.clear();
            //lcd.print("Pressed: LEFT");
            break;
          }
          case 5: {
            rst_type = 1;
            lcd.clear();
            //lcd.print("Pressed: RIGHT"); 
            break;
          }
          default: {break;}  
        }    
      }

      //PRINTING RESET SCREEN
      rst_print();

      //EXITING RESET SCREEN IF SELECT PRESSED IN ORDER OF TYPE OF RESET
      if(tmp_btn == 1)//select pressed
      {
        lcd.clear();
        if(rst_type == 0) //MANUAL RESET LOAD DEFAULT VALUES AND GO TO IDLE STATE
        {
          rst_flag = 0;
          target_temp = TEMP_DEFAULT;
          target_hours = HOURS_DEFAULT;
          start_sig = 0;
          clear_backup_data();
        }

        if(rst_type == 1) //AUTOMATIC RESET LOAD BACKUP VALUES AND GO TO RUNNING STATE
        {
          rst_flag = 0;
          //target_temp and target_hours BACKUP VALUES ARE ALREADY LOADED IN SETUP() -> reset_recovery() function
          start_sig = 1;
        }
        //actions declared above are taken when select button is pressed
      }

}

void clear_backup_data(void)
{
  //no need for resetting target temp as it is not use in if clause, and to save cycles of EEPROM as much as possible  
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0x06); //set adress for first write
    
    Wire.write(0x00); //clearing remaining hours in YEAR register
    
    Wire.endTransmission();
    
}
