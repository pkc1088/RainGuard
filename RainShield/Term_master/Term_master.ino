#include <SoftwareSerial.h>
//#include <LiquidCrystal.h>
#include <pitches.h>
#include <DHT.h>

#define DHTPIN 2       
#define DHTTYPE DHT22  
DHT dht(DHTPIN, DHTTYPE);

SoftwareSerial mySerial_2(6,7); // (TX, RX)
String password = "";
char psw_buff[5] = {0};
char rtn_check;
int btnpin2 = 21, btnpin3 = 3, btnpin4 = 20; // 핀 3번, 20번은 인터럽트 됨

int motor_num = 100;
int motorE3 = 52, motorP3 = 10, motorD3 = 48;
volatile float humidity, illuminance;
bool user_mode = false;
volatile bool working_state = false;

void setup() {
  Serial.begin(9600); 
  dht.begin();
  
  mySerial_2.begin(9600); // 영진한테 보냄
  Serial3.begin(9600);   // 영진한테 읽는거임
  //pinMode(btnpin1, INPUT);
  pinMode(btnpin2, INPUT);
  pinMode(btnpin3, INPUT);
  pinMode(btnpin4, INPUT);
  
  digitalWrite(motorE3, HIGH);
  digitalWrite(motorD3, HIGH);
  analogWrite(motorP3, 255); // 정방향은 high가 속도 0
  // attachInterrupt(digitalPinToInterrupt(btnpin1), btn1Handler, RISING);  // 인터럽트 설정
  attachInterrupt(digitalPinToInterrupt(btnpin2), btn2Handler, RISING);
  attachInterrupt(digitalPinToInterrupt(btnpin3), btn3Handler, RISING);
  attachInterrupt(digitalPinToInterrupt(btnpin4), btn4Handler, RISING);
}

// power off
void btn2Handler() {
  user_mode = true;
  working_state = true;
  motorOPChange(0);
  Serial.println("User mode 2 by interrupt");
}

// power max
void btn3Handler() {
  user_mode = true;
  working_state = true;
  motorOPChange(3);
  Serial.println("User mode 3 by interrupt");
}

// system mode
void btn4Handler() {
  user_mode = false;
  working_state = true;
  motorOPChange(0);
  Serial.println("system mode by interrupt");
}

void readhumidity() {
  humidity = dht.readHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed to read humidity!");
    return;
  }
}

void readIlluminance() {
  int reading = analogRead(56); // A2
  illuminance = reading * 5.0 / 1024.0;
}

void LCDdisplay() {
  Serial.println("hum : " + String(humidity) 
            + ", ill : " + String(illuminance));
}

void finishSpeaker() {
  Serial.println("Dry Done");
}

void motorOPChange(int num) {
  Serial.println("motor change : " + String(num));
  if (num == 3) {
    motor_num = 3;
    analogWrite(motorP3, 0); // 0이면 최고속도
  } else if (num == 2) {
    motor_num = 2;
    analogWrite(motorP3, 50); //50
  } else if (num == 1) {
    motor_num = 1;
    analogWrite(motorP3, 70); //80
  } else if (num == 0) {
    motor_num = 0;
    analogWrite(motorP3, 255);   // 모터 3 OFF
  }
}

void tossed_psw_register() {
  memset(psw_buff, 0, sizeof(psw_buff));
  int idx = 0;
  while (1) {
    if(Serial3.read() == 'p') {
      Serial.println("umbrella received");
      mySerial_2.write('p'); // p라는 신호는 비번을 입력하라는 뜻
      break;
    }
  }  // 영진한테서 읽는거
  Serial.println("wait until password entered....");
  while (1) {
    if(Serial3.available()) {
      psw_buff[idx] = Serial3.read();
      password += psw_buff[idx];
      if(idx == 3) break; 
      else idx++;
    }
  }
}

bool passwordCheck() {
  int idx = 0;
  while (1) {
    if(Serial3.available()) {
      if(password.charAt(idx++) == Serial3.read()) {
        if(idx == 4) {
          mySerial_2.write('c');
          Serial.println("password correct!");
          return true;
        } 
      } 
      else {
        mySerial_2.write('w'); // 비밀번호 불일치하다고 시그널 줌
        Serial.println("password wrong....!");
        return false;
      }
    }
  }
  Serial.println("password check error");
  return false;
}

void anticipated_time() {
  char buff[5] = {0};
  memset(buff, 0, sizeof(buff));
  String str = String(humidity);

  for(int i = 0; i < 5; i++) {
    buff[i] = str.charAt(i);
    Serial.print(buff[i]);
  }
  buff[5] = '%';
  Serial.println("%");

  for(int i = 0; i < 6; i++) {
    mySerial_2.write(buff[i]);
  }
}

void loop() {

  tossed_psw_register(); 
  Serial.println("Received password : " + password);

  while(1) {
    if(Serial3.read() == 'r') {
      Serial.println("return request");
      
      mySerial_2.write('r');
      anticipated_time(); 

      while(1) {
        if(Serial3.available()) {
          rtn_check = Serial3.read();
          break;
        }
      }

      if(rtn_check == 'y') {
        if(passwordCheck()) {
          motorOPChange(0);
          Serial.println("umbrella returned!");
          password = "";
          break;
        } 
      } else if(rtn_check == 'n') {
        Serial.println("not yet");
      } 
    }
  
    readhumidity();
    readIlluminance();
    LCDdisplay();
    
    // 시스템 건조
    if(!user_mode) {    
      if(humidity >= 40 && illuminance >= 1) {
        working_state = true; 
      }
      
      noInterrupts();  
      if(humidity >= 50 && working_state) { //40
        if(motor_num != 3) motorOPChange(3);
      }
      else if(humidity >= 40 && working_state) { //35
        if(motor_num != 2) motorOPChange(2);
      }
      else if(humidity >= 35 && working_state) { //30
        if(motor_num != 1) motorOPChange(1);
      }
      else if(humidity < 35 && working_state) { //30
        motorOPChange(0);
        finishSpeaker();
        working_state = false;
      }
      interrupts();  
      
    }
    else { 
      // 사용자 모드는 인터럽트 핸들러로 대체
    }
    delay(500);
  }
}



