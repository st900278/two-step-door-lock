#include <Keypad.h>
#include <Arduino_FreeRTOS.h>

#include <LiquidCrystal_I2C.h>
#define KEY_ROWS 4
#define KEY_COLS 4

char keymap[KEY_ROWS][KEY_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte colPins[KEY_COLS] = {9, 8, 7, 6};
// Column pin 1~4
byte rowPins[KEY_ROWS] = {13, 12, 11, 10}; 
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
char key;
TaskHandle_t xTaskNormalHandle;
TaskHandle_t xTaskEnterHandle;
TaskHandle_t xTaskChPasswordHandle;

char password[3] = "123";
int buzzer = 5;
int ledPin = 2;
int i = 0;
const int trigPin = 4;  const int echoPin = 3;
long duration;           int distance;
String readStr;

Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, KEY_ROWS, KEY_COLS);
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  noInterrupts(); // disable all interrupts
  TCCR1A = 0;    TCCR1B = 0;    TCNT1 = 0;
  OCR1A = 31248;  // target for counting
  TCCR1B |= (1 << WGM12); // turn on CTC
  TCCR1B |= (1<<CS12) | (1<<CS10); // 1024 prescaler
  TIMSK1 |= (1<<OCIE1A); // enable timer compare int.
  interrupts();

  
  xTaskCreate(TaskNormal,  (const portCHAR *)"Normal",  64,  NULL,  3,  &xTaskNormalHandle );

  xTaskCreate(TaskEnter,  (const portCHAR *)"AbNormal",  64,  NULL,  1,  &xTaskEnterHandle );
  xTaskCreate(TaskChPassword,  (const portCHAR *)"Password",  64,  NULL,  1,  &xTaskChPasswordHandle );
  //xTaskCreate(TaskToken,  (const portCHAR *)"Token",  128,  NULL,  1,  &xTaskTokenHandle );
  vTaskStartScheduler();
  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}
ISR(TIMER1_COMPA_vect) {
  i += 1;
}

void loop(){
}
void TaskNormal(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  pinMode(ledPin, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  for (;;) // A Task shall never return or exit.
  {
    
    vTaskPrioritySet(xTaskEnterHandle, 1);
    vTaskPrioritySet(xTaskChPasswordHandle, 1);
    vTaskPrioritySet(xTaskNormalHandle, 2);
    Serial.println("Stage:0");
    for(;;){
      lcd.noBacklight();
      lcd.clear();
      int pr = analogRead(A0);
    
      digitalWrite(trigPin, LOW); // Clears the trigPin
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);    delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
      duration = pulseIn(echoPin, HIGH);
      distance = duration*0.034/2;
      if(distance < 40 && pr < 100){
        tone(buzzer,400);
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
        noTone(buzzer);
        break;
      }
      
    }
    Serial.println("Stage:1");
    for(;;)
    {
      lcd.backlight();
      lcd.clear();
      lcd.setCursor(0, 0);  // setting cursor   
      lcd.print("Enter number");
      lcd.setCursor(0, 1);
      if(i%3 == 0)
        lcd.print("A:Enter Pass");
      else if(i%3 == 1)
        lcd.print("B:Change Pass");
      else
        lcd.print("C:Time");
      key = myKeypad.getKey();
      if(key){
        Serial.println(key);
        if(key == 'A'){
          vTaskPrioritySet(xTaskEnterHandle, 3);
          break;
        }
        else if(key == 'B'){
          vTaskPrioritySet(xTaskChPasswordHandle, 3);
          break;
        }
        else if(key == 'C'){
          Serial.println("Stage:9");
          while(Serial.available() == 0);
          lcd.clear();
          
          int lcd_Ptr = 0;
          int lcd_Loc = 0;
          while (Serial.available())
          {
            lcd.setCursor(lcd_Ptr++, lcd_Loc);
            delay(30);  //delay to allow buffer to fill 
            if (Serial.available() >0)
            {
              char c = Serial.read();  //gets one byte from serial buffer
              if(c == ' '){
                lcd_Loc = 1;
                lcd_Ptr = 0;
              }
              lcd.print(c);
            }
          }
          delay(3000);
        }
        
      }
    }

  }
}

void TaskEnter(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  int flag = 0;
  int ptr = 0;
  key = "";
  while(1){
    vTaskPrioritySet(xTaskNormalHandle, 1);
    vTaskPrioritySet(xTaskEnterHandle, 2);
    Serial.println("Stage:2");
    for(;;)
    {
      lcd.clear();
      lcd.setCursor(0, 0);  // setting cursor   
      lcd.print("Enter account");
      key = myKeypad.getKey();
      if(key){
        Serial.println(key);
        break;
      }
    }
  
    flag = 0;
    ptr = 0;
    Serial.println("Stage:3");
    for(;;)
    {
      lcd.clear();
      lcd.setCursor(0, 0);  // setting cursor   
      lcd.print("Enter password");
      key = myKeypad.getKey();
      if(key){
        Serial.println(key);
        if(key == 'D'){
          break;
        }
      }
     
    }
    Serial.println("Stage:4");
    for(;;)
    {
      lcd.clear();
      lcd.setCursor(0, 0);  // setting cursor   
      lcd.print("Enter two-step");
      key = myKeypad.getKey();
      if(key){
        Serial.println(key);
        if(key == 'D'){
          
          while(Serial.available() == 0);
          char getByte = Serial.read();
          if(getByte == '1'){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Correct");
            tone(buzzer,400);   
            digitalWrite(ledPin, HIGH);
            delay(3000);
            digitalWrite(ledPin, LOW);
            noTone(buzzer);
          }
          else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Wrong");
            tone(buzzer,800); 
            delay(3000);
            noTone(buzzer);
          }
          break;
        }
      }
    }
    vTaskPrioritySet(xTaskNormalHandle, 3);
  }
}

void TaskChPassword(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  int flag = 0;
  int ptr = 0;
  key = "";
  while(1){
    vTaskPrioritySet(xTaskNormalHandle, 1);
    vTaskPrioritySet(xTaskChPasswordHandle, 2);
    Serial.println("Stage:2");
    for(;;)
    {
      lcd.clear();
      lcd.setCursor(0, 0);  // setting cursor   
      lcd.print("Enter account");
      key = myKeypad.getKey();
      if(key){
        Serial.println(key);
        break;
      }
    }
  
    flag = 0;
    ptr = 0;
    Serial.println("Stage:3");
    for(;;)
    {
      lcd.clear();
      lcd.setCursor(0, 0);  // setting cursor   
      lcd.print("Enter password");
      key = myKeypad.getKey();
      if(key){
        Serial.println(key);
        if(key == 'D'){
          break;
        }
      }
     
    }

    Serial.println("Stage:5"); // new password
    for(;;)
    {
      lcd.clear();
      lcd.setCursor(0, 0);  // setting cursor   
      lcd.print("New password");
      key = myKeypad.getKey();
      if(key){
        Serial.println(key);
        if(key == 'D'){
          break;
        }
      }
     
    }
    
    Serial.println("Stage:6");
    for(;;)
    {
      lcd.clear();
      lcd.setCursor(0, 0);  // setting cursor   
      lcd.print("Enter two-step");
      key = myKeypad.getKey();
      if(key){
        Serial.println(key);
        if(key == 'D'){
          while(Serial.available() == 0);
          char getByte = Serial.read();
          if(getByte == '1'){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Success");
            tone(buzzer,400);   
            digitalWrite(ledPin, HIGH);
            delay(3000);
            digitalWrite(ledPin, LOW);
            noTone(buzzer);
          }
          else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Failed");
            tone(buzzer,800); 
            delay(3000);
            noTone(buzzer);
          }
          break;
          
        }
      }
    }
    vTaskPrioritySet(xTaskNormalHandle, 3);
  }
}


