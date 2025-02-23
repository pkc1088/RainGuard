#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

// 핀 번호 (RS, E, DB4, DB5, DB6, DB7)
LiquidCrystal lcd(44, 45, 46, 47, 48, 49); 
SoftwareSerial mySerial(6, 7);
char buff[30];
char hmBuff[7];
volatile bool flag = false;
int trigPin = 8;
int echoPin = 9;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(3, INPUT); // 버튼을 외부 인터럽트 핀에 연결
  attachInterrupt(digitalPinToInterrupt(3), usermode, RISING);
  
  mySerial.begin(9600); // 보내는거
  Serial1.begin(9600);  // 받는거
  Serial.begin(9600);   // 내화면
  
  lcd.begin(16, 2); // LCD 초기화 (16x2 LCD 사용)
  lcd.print("System Ready"); // 초기 메시지 출력
  delay(2000);
  lcd.clear();
}
void check_password(){
  while (Serial.available() > 0) {
    Serial.read(); // 남아 있는 데이터를 모두 읽어서 버려버림
  }
  delay(2000);
  //비밀번호 받는과정
  lcd.clear();
  lcd.print("Input Password");
  lcd.setCursor(0, 1);
  int idx = 0;
  while (1) {
    if (Serial.available()) {
      if (idx == 4) {
        buff[idx] = '\0';
        break;
      }
      buff[idx] = Serial.read();
      lcd.print(buff[idx]);
      idx++;
    }
  }
  delay(2000);
  // 비밀번호 전송 과정
  lcd.clear();
  lcd.print("Sending Password");
  for (int i = 0; i < 4; i++) {
    mySerial.write(buff[i]);
  }
}

bool check_umbrella() {
    digitalWrite(trigPin, HIGH);
    delay(50);
    digitalWrite(trigPin, LOW);
    float duration = pulseIn(echoPin, HIGH);
    float distance = duration * 340 / 10000 / 2;
    return distance <= 20;
}

void usermode() {
  flag = true;
}

char ask_return() {
  while (1) {
    while (Serial.available() > 0) {
      Serial.read(); // 남아 있는 데이터를 모두 읽어서 버려버림
    }
    lcd.clear();
    lcd.print("Return Umbrella?");
    lcd.setCursor(0, 1);
    String base = "Y/N";
    String time = String(hmBuff);

    // 남은 공간(왼쪽에 붙일 공백 개수)을 계산
    int spaces = 16 - (base.length() + time.length());
    for (int i = 0; i < spaces; i++) {
      base += " "; // 공백 추가
    }
    base += time;
    lcd.print(base);
    //텍스트 받을때 까지 대기
    while (!Serial.available()){}
    char command = Serial.read();
    lcd.clear();
    lcd.print("Answer: ");
    if(command=='y'){
      lcd.print(command);
      return command;
    }
    else if (command == 'n') {
      lcd.print(command);
      flag= false;
      return command;
    } 
    else {
      lcd.clear();
      lcd.print("Enter y or n!");
      delay(2000);
    }
  }
}

void loop() {
  while (!check_umbrella()) {}
  lcd.clear();
  lcd.print("Umbrella Find!");
  delay(2000);
  // p 보내고 받는 과정
  mySerial.write('p');
  lcd.clear();
  lcd.print("Waiting for p");
  delay(2000);
  while (1) {
    char temp = Serial1.read();
    if (temp == 'p') {
      lcd.clear();
      lcd.print("Received: p");
      break;
    }
  }
  check_password();
  
  Shopping: // goto 구문이 이동할 레이블 정의
  delay(2000);
  // 쇼핑 중, 버튼을 눌러 요청
  lcd.clear();
  lcd.print("Shopping...");
  while (!flag) {}
  
  // 우산 요청
  lcd.clear();
  lcd.print("Req Umbrella!!!");
  Ask:
  delay(2000);
  mySerial.write('r');
  while (1) {
    char temp = Serial1.read();
    if (temp == 'r') {
      int idx = 0;
      memset(hmBuff, 0, sizeof(hmBuff));
      while (1) {
        if(Serial1.available()) {
          hmBuff[idx++] = Serial1.read();
          if(idx == 6){
            hmBuff[idx]='\0';
            break; 
          } 
        }
      }
      char answer = ask_return();
      mySerial.write(answer);
      if (answer=='n'){
        goto Shopping; // 우산이 감지되지 않으면 Start 레이블로 이동
      }
      else if(answer=='y'){
        check_password();
        delay(2000);
        char temp = Serial1.read();
        if(temp=='c'){
          lcd.clear();
          lcd.print("Password Correct");
          break;
        }
        else if(temp=='w'){
          lcd.clear();
          lcd.print("Password Wrong");
          goto Ask;
        }       
      }
    }
  }
  delay(2000);
  lcd.clear();
  lcd.print("GoodBye!!!");
  while (Serial.available() > 0) {
    Serial.read(); // 남아 있는 데이터를 모두 읽어서 버려버림
  }
  delay(2000);
  lcd.clear();
  flag= false;
}
