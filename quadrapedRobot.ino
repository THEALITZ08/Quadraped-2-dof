#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire, -1);


struct mainServo{
  Servo cihuyy;
  float home;
  float start;
  float target;
  float current;
  float progress;
  unsigned long startTime;
  unsigned long duration;
  int pin;
  bool moving;
  
};
mainServo LF,RF,LB,RB;
mainServo LFK,RFK,LBK,RBK;


bool sp = false;
bool kanan = false;
bool kiri = false;
bool depan = false;
bool  belakang = false;
bool hai = false;
const char* ssid = "cihuyy";
const char* password = "123456789";
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* kiri_cmd = "simulation/kiri/cmd";
const char* kiri_status = "simulation/kiri/status";
const char* kanan_cmd = "simulation/kanan/cmd";
const char* kanan_status = "simulation/kanan/status";
const char* maju_cmd = "simulation/maju/cmd";
const char* maju_status = "simulation/maju/status";
const char* mundur_cmd = "simulation/mundur/cmd";
const char* mundur_status = "simulation/mundur_status";
const char* hallo_cmd = "simulation/hallo/cmd";
const char* hallo_status = "simulation/hallo/status";

WiFiClient espclient;
PubSubClient client(espclient);

unsigned long lastReconnect = 0;
unsigned long lastWiFi = 0;

const long intervalMqtt = 1000;
const long intervalWiFi = 3000;

enum state{
  LFK_START,
  LF_MOVE,
  LFK_DOWN,

  RBK_START,
  RB_MOVE,
  RBK_DOWN,

  RFK_START,
  RF_MOVE,
  RFK_DOWN,

  LBK_START,
  LB_MOVE,
  LBK_DOWN,

  ALL_HOME,
  HOME_WAIT,
  IDLE
};
state State = LFK_START;
state Change = LFK_START;
state front = LFK_START;
state rear = RBK_START;

enum SapaState {
  SAPA_START,
  SAPA_UP,
  SAPA_DOWN,
  SAPA_DONE
};

SapaState sapaState = SAPA_START;
int sapaCount = 0;

void initServo(mainServo& s, int pin, float home) {
  s.pin = pin;
  s.home = home;
  s.target = home;
  s.current = home;
  s.start = home;
  s.moving = false;
  s.startTime = 0;
  s.progress = 0.0;
  s.duration = 0;
  s.cihuyy.attach(pin);
  s.cihuyy.write(home);
}

void moveServo(mainServo&s, float target, unsigned long duration) {
  s.start = s.current;
  s.duration = duration;
  s.target = target;
  s.startTime = millis();
  s.progress = 0.0;
  s.moving = true;
  

}

void updateServo(mainServo &s) {
  if(!s.moving) return;

  unsigned long elapsed = millis() - s.startTime;

  s.progress = (float)elapsed / s.duration;
  
  if(s.progress >= 1.0) {
    s.progress = 1.0;
    s.moving = false;
  }

  float smooth = (1- cos(s.progress * PI)) / 2.0;

  s.current = s.start + (s.target - s.start) * smooth;
  s.cihuyy.write(s.current);
}

void updateAll() {
  updateServo(LF);
  updateServo(RF);
  updateServo(LB);
  updateServo(RB);
  updateServo(LFK);
  updateServo(RFK);
  updateServo(LBK);
  updateServo(RBK);
}

void connectMqtt() {
  if(client.connected()) return;

  String ClientID = "ESP32- " + WiFi.macAddress();

  if(millis() - lastReconnect > intervalMqtt) {
    if(client.connect(ClientID.c_str())) {
      Serial.print("berhasil!!!, address: ");
      Serial.println(WiFi.macAddress());
      client.subscribe(kiri_cmd);
      client.subscribe(kanan_cmd);
      client.subscribe(maju_cmd);
      client.subscribe(mundur_cmd);
      client.subscribe(hallo_cmd);
      client.publish(kiri_status, "kaki kiri siap");
      client.publish(kanan_status, "kaki kanan siap");
    } else{
      Serial.print("failed!!, rc: ");
      Serial.println(client.state());
    }
    lastReconnect = millis();
  }
}

void callbackMqtt(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  memcpy(message,payload,length);
  message[length] = '\0';

  if(strcmp(topic,kiri_cmd) == 0) {
    if(strcmp(message, "KIRI") == 0) {
      State = LFK_START;
      kiri = true;
      kanan = false;
      sp = true;
      client.publish(kiri_status, "robot belok kiri");
    }
  }
  if(strcmp(topic, kanan_cmd) == 0) {
    if(strcmp(message, "KANAN") == 0) {
      Change = LFK_START;
      kiri = false;
      kanan = true;
      sp = true;
      client.publish(kanan_status, "robot belok kanan");
    }
  }
  if(strcmp(topic, maju_cmd) == 0) {
    if(strcmp(message, "MAJU") == 0) {
      front = LFK_START;
      depan = true;
      belakang = false;
      sp = true;
      client.publish(maju_status, "ROBOT MAJU");
    }
  }
  if(strcmp(topic, mundur_cmd) == 0) {
    if(strcmp(message, "MUNDUR") == 0) {
      rear = RBK_START;
      depan = false;
      belakang = true;
      sp = true;
      client.publish(mundur_status, "ROBOT MUNDUR");
    }
  }
  if(strcmp(topic, hallo_cmd) == 0) {
    if(strcmp(message, "SAPA") == 0) {
      hai = true;
      client.publish(hallo_status, "ROBOT MENYAPA");
    }
  }
}

void mukaSenyum() {
  display.clearDisplay();

  display.drawLine(32,25,48,25,WHITE);
  display.drawLine(80,25,96,25,WHITE);

  display.drawLine(58,48,70,48,WHITE);
  display.drawLine(56,46,58,48,WHITE);
  display.drawLine(70,48,72,46,WHITE);

  display.display();
}

void sleep() {
  display.clearDisplay();

  display.drawLine(30,30,52,30, SSD1306_WHITE);
  display.drawLine(76,30,98,30, SSD1306_WHITE);

  display.drawLine(58,45,70,45, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(100,10);
  display.print("z");
  display.setCursor(108,4);
  display.print("Z");

  display.display();

}

void excited() {
  display.clearDisplay();

  display.fillRoundRect(25,15,30,25,9,SSD1306_WHITE);
  display.fillRoundRect(73,15,30,25,9,SSD1306_WHITE);

  display.drawCircle(64,45,9,SSD1306_WHITE);
  display.fillRect(53,36,22,10,SSD1306_BLACK);

  display.display();
}
void belokkiri() {
  switch(State) {
    case LFK_START:
    if(!LFK.moving) {
      Serial.println("Kaki kiri depan angkat");
      moveServo(LFK, 130, 600);
      State = LF_MOVE;
    }
    break;
    case LF_MOVE:
    if(LFK.progress > 0.7 && !LF.moving) {
      Serial.println("kaki kiri depan geser");
      moveServo(LF, 65, 600);
      State = LFK_DOWN;
    }
    break;
    case LFK_DOWN:
    if(LF.progress > 0.8) {
      Serial.println("kaki kiri depan turun");
      moveServo(LFK, LFK.home, 600);
      State = RBK_START;
    }
    break;
    case RBK_START:
    if(!LFK.moving &&  !RBK.moving) {
      Serial.println("kaki kanan belakang angkat");
      moveServo(RBK, 130, 600);
      State = RB_MOVE;
    }
    break;
    case RB_MOVE:
    if(RBK.progress > 0.7 && !RB.moving) {
      Serial.println("kaki kanan belakang geser");
      moveServo(RB, 0, 600);
      State = RBK_DOWN;
    }
    break;
    case RBK_DOWN:
    if(RB.progress > 0.8) {
      Serial.println("kaki kanan belakang turun");
      moveServo(RBK, RBK.home, 600);
      State = RFK_START;
    }
    break;
    case RFK_START:
    if(!RFK.moving) {
      Serial.println("kanan depan angkat");
      moveServo(RFK, 75, 600);
      State = RF_MOVE;
    }
    break;
    case RF_MOVE:
    if(RFK.progress > 0.7 && !RF.moving) {
      Serial.println("kanan depan geser");
      moveServo(RF, 75, 600);
      State = RFK_DOWN;
    }
    break;
    case RFK_DOWN:
    if(RF.progress > 0.8) {
      Serial.println("kanan depan turun");
      moveServo(RFK, RFK.home, 600);
      State = LBK_START;
    }
    break;
    case LBK_START:
    if(!RFK.moving &&  !LBK.moving) {
      Serial.println("kiri belakang angkat");
      moveServo(LBK, 45, 600);
      State = LB_MOVE;
    }
    break;
    case LB_MOVE:
    if(LBK.progress > 0.7 && !LB.moving) {
      Serial.println("kiri belakang geser");
      moveServo(LB, 55, 600);
      State = LBK_DOWN;
    }
    break;
    case LBK_DOWN:
    if(LB.progress > 0.8) {
      Serial.println("kiri belakang turun");
      moveServo(LBK, LBK.home, 600);
      State = ALL_HOME;
    }
    break;
    case ALL_HOME:
    if(!LFK.moving && !RBK.moving && !RFK.moving && !LBK.moving) {
      Serial.println("kembali ke posisi awal");
      moveServo(LF, LF.home, 200);
      moveServo(RF, RF.home, 200);
      moveServo(LB, LB.home, 200);
      moveServo(RB, RB.home, 200);
      State = HOME_WAIT;
    }
    break;
    case HOME_WAIT:
    if(!LF.moving && !RF.moving && !RB.moving && !LB.moving) {
      Serial.println("STATE SELESAII");
      kiri = false;
      kanan = false;
      sp = false;
      State = IDLE;

    }
    break;

    case IDLE:
    break;
  }
}

void belokKanan() {
  switch(Change) {
    case LFK_START:
    if(!LFK.moving) {
      Serial.println("Kaki kiri depan angkat");
      moveServo(LFK, 130, 600);
      Change = LF_MOVE;
    }
    break;
    case LF_MOVE:
    if(LFK.progress > 0.7 && !LF.moving) {
      Serial.println("kaki kiri depan geser");
      moveServo(LF, 145, 600);
      Change = LFK_DOWN;
    }
    break;
    case LFK_DOWN:
    if(LF.progress > 0.8) {
      Serial.println("kaki kiri depan turun");
      moveServo(LFK, LFK.home, 600);
      Change = RBK_START;
    }
    break;
    case RBK_START:
    if(!LFK.moving &&  !RBK.moving) {
      Serial.println("kaki kanan belakang angkat");
      moveServo(RBK, 130, 600);
      Change = RB_MOVE;
    }
    break;
    case RB_MOVE:
    if(RBK.progress > 0.7 && !RB.moving) {
      Serial.println("kaki kanan belakang geser");
      moveServo(RB, 105, 600);
      Change = RBK_DOWN;
    }
    break;
    case RBK_DOWN:
    if(RB.progress > 0.8) {
      Serial.println("kaki kanan belakang turun");
      moveServo(RBK, RBK.home, 600);
      Change = RFK_START;
    }
    break;
    case RFK_START:
    if(!RFK.moving) {
      Serial.println("kanan depan angkat");
      moveServo(RFK, 75, 600);
      Change = RF_MOVE;
    }
    break;
    case RF_MOVE:
    if(RFK.progress > 0.7 && !RF.moving) {
      Serial.println("kanan depan geser");
      moveServo(RF, 160, 600);
      Change = RFK_DOWN;
    }
    break;
    case RFK_DOWN:
    if(RF.progress > 0.8) {
      Serial.println("kanan depan turun");
      moveServo(RFK, RFK.home, 600);
      Change = LBK_START;
    }
    break;
    case LBK_START:
    if(!RFK.moving &&  !LBK.moving) {
      Serial.println("kiri belakang angkat");
      moveServo(LBK, 45, 600);
      Change = LB_MOVE;
    }
    break;
    case LB_MOVE:
    if(LBK.progress > 0.7 && !LB.moving) {
      Serial.println("kiri belakang geser");
      moveServo(LB, 180, 600);
      Change = LBK_DOWN;
    }
    break;
    case LBK_DOWN:
    if(LB.progress > 0.8) {
      Serial.println("kiri belakang turun");
      moveServo(LBK, LBK.home, 600);
      Change = ALL_HOME;
    }
    break;
    case ALL_HOME:
    if(!LFK.moving && !RBK.moving && !RFK.moving && !LBK.moving) {
      Serial.println("kembali ke posisi awal");
      moveServo(LF, LF.home, 200);
      moveServo(RF, RF.home, 200);
      moveServo(LB, LB.home, 200);
      moveServo(RB, RB.home, 200);
      Change = HOME_WAIT;
    }
    break;
    case HOME_WAIT:
    if(!LF.moving && !RF.moving && !RB.moving && !LB.moving) {
      Serial.println("STATE SELESAII");
      Change = IDLE;
      sp = false;
      kiri = false;
      kanan = false;
    }
    break;

    case IDLE:
    break;
  }
}

void maju() {
  switch(front) {
    case LFK_START:
    if(!LFK.moving) {
      Serial.println("Kaki kiri depan angkat");
      moveServo(LFK, 130, 600);
      front = LF_MOVE;
    }
    break;
    case LF_MOVE:
    if(LFK.progress > 0.7 && !LF.moving) {
      Serial.println("kaki kiri depan geser");
      moveServo(LF, 80, 600);
      front = LFK_DOWN;
    }
    break;
    case LFK_DOWN:
    if(LF.progress > 0.8) {
      Serial.println("kaki kiri depan turun");
      moveServo(LFK, LFK.home, 600);
      front = RFK_START;
    }
    break;
    case RFK_START:
    if(!LFK.moving &&  !RFK.moving) {
      Serial.println("kaki kanan belakang angkat");
      moveServo(RFK, 75, 600);
      front = RF_MOVE;
    }
    break;
    case RF_MOVE:
    if(RFK.progress > 0.7 && !RF.moving) {
      Serial.println("kaki kanan belakang geser");
      moveServo(RF, 160, 600);
      front = RFK_DOWN;
    }
    break;
    case RFK_DOWN:
    if(RF.progress > 0.8) {
      Serial.println("kaki kanan belakang turun");
      moveServo(RFK, RFK.home, 600);
      front = RBK_START;
    }
    break;
    case RBK_START:
    if(!RBK.moving) {
      Serial.println("kanan depan angkat");
      moveServo(RBK, 105, 600);
      front = RB_MOVE;
    }
    break;
    case RB_MOVE:
    if(RBK.progress > 0.7 && !RB.moving) {
      Serial.println("kanan depan geser");
      moveServo(RB, 145, 600);
      front = RBK_DOWN;
    }
    break;
    case RBK_DOWN:
    if(RB.progress > 0.8) {
      Serial.println("kanan depan turun");
      moveServo(RBK, RBK.home, 600);
      front = LBK_START;
    }
    break;
    case LBK_START:
    if(!RBK.moving &&  !LBK.moving) {
      Serial.println("kiri belakang angkat");
      moveServo(LBK, 45, 600);
      front = LB_MOVE;
    }
    break;
    case LB_MOVE:
    if(LBK.progress > 0.7 && !LB.moving) {
      Serial.println("kiri belakang geser");
      moveServo(LB, 35, 600);
      front = LBK_DOWN;
    }
    break;
    case LBK_DOWN:
    if(LB.progress > 0.8) {
      Serial.println("kiri belakang turun");
      moveServo(LBK, LBK.home, 600);
      front = ALL_HOME;
    }
    break;
    case ALL_HOME:
    if(!LFK.moving && !RBK.moving && !RFK.moving && !LBK.moving) {
      Serial.println("kembali ke posisi awal");
      moveServo(LF, LF.home, 200);
      moveServo(RF, RF.home, 200);
      moveServo(LB, LB.home, 200);
      moveServo(RB, RB.home, 200);
      front = HOME_WAIT;
    }
    break;
    case HOME_WAIT:
    if(!LF.moving && !RF.moving && !RB.moving && !LB.moving) {
      Serial.println("STATE SELESAII");
      front = IDLE;
      sp = false;
      depan = false;
      belakang = false;
    }
    break;

    case IDLE:
    break;
  }
}

void mundur() {
  switch(rear) {
    case RBK_START:
    if(!RBK.moving) {
      Serial.println("Kaki kiri depan angkat");
      moveServo(RBK, 130, 600);
      rear = RB_MOVE;
    }
    break;
    case RB_MOVE:
    if(RBK.progress > 0.7 && !RB.moving) {
      Serial.println("kaki kiri depan geser");
      moveServo(RB, 35, 600);
      rear = RBK_DOWN;
    }
    break;
    case RBK_DOWN:
    if(RB.progress > 0.8) {
      Serial.println("kaki kiri depan turun");
      moveServo(RBK, RBK.home, 600);
      rear = LBK_START;
    }
    break;
    case LBK_START:
    if(!RBK.moving &&  !LBK.moving) {
      Serial.println("kaki kanan belakang angkat");
      moveServo(LBK, 45, 600);
      rear = LB_MOVE;
    }
    break;
    case LB_MOVE:
    if(LBK.progress > 0.7 && !LB.moving) {
      Serial.println("kaki kanan belakang geser");
      moveServo(LB, 160, 600);
      rear = LBK_DOWN;
    }
    break;
    case LBK_DOWN:
    if(LB.progress > 0.8) {
      Serial.println("kaki kanan belakang turun");
      moveServo(LBK, LBK.home, 600);
      rear = RFK_START;
    }
    break;
    case RFK_START:
    if(!RFK.moving) {
      Serial.println("kanan depan angkat");
      moveServo(RFK, 75, 600);
      rear = RF_MOVE;
    }
    break;
    case RF_MOVE:
    if(RFK.progress > 0.7 && !RF.moving) {
      Serial.println("kanan depan geser");
      moveServo(RF, 55, 600);
      rear = RFK_DOWN;
    }
    break;
    case RFK_DOWN:
    if(RF.progress > 0.8) {
      Serial.println("kanan depan turun");
      moveServo(RFK, RFK.home, 600);
      rear = LFK_START;
    }
    break;
    case LFK_START:
    if(!RFK.moving &&  !LFK.moving) {
      Serial.println("kiri belakang angkat");
      moveServo(LFK, 130, 600);
      rear = LF_MOVE;
    }
    break;
    case LF_MOVE:
    if(LFK.progress > 0.7 && !LF.moving) {
      Serial.println("kiri belakang geser");
      moveServo(LF, 165, 600);
      rear = LFK_DOWN;
    }
    break;
    case LFK_DOWN:
    if(LF.progress > 0.8) {
      Serial.println("kiri belakang turun");
      moveServo(LFK, LFK.home, 600);
      rear = ALL_HOME;
    }
    break;
    case ALL_HOME:
    if(!LFK.moving && !RBK.moving && !RFK.moving && !LBK.moving) {
      Serial.println("kembali ke posisi awal");
      moveServo(LF, LF.home, 200);
      moveServo(RF, RF.home, 200);
      moveServo(LB, LB.home, 200);
      moveServo(RB, RB.home, 200);
      rear = HOME_WAIT;
    }
    break;
    case HOME_WAIT:
    if(!LF.moving && !RF.moving && !RB.moving && !LB.moving) {
      Serial.println("STATE SELESAII");
      rear = IDLE;
      sp = false;
      depan = false;
      belakang = false;
    }
    break;

    case IDLE:
    break;
  }
}


void plotter() {
  static unsigned long lastPrint = 0;
  if(!sp) return;
  if(millis() - lastPrint > 20) {
    lastPrint = millis();



    Serial.print(LF.current);
    Serial.print(",");
    Serial.print(RB.current);
    Serial.print(",");
    Serial.print(RF.current);
    Serial.print(",");
    Serial.print(LB.current);
    Serial.print(",");
    Serial.print(LFK.current);
    Serial.print(",");
    Serial.print(RBK.current);
    Serial.print(",");
    Serial.print(RFK.current);
    Serial.print(",");
    Serial.println(LBK.current);
    
    

  }
}

void sapa() {
  switch (sapaState) {

    case SAPA_START:
      moveServo(LFK, 100, 400);
      sapaState = SAPA_UP;
      break;

    case SAPA_UP:
      if (!LFK.moving) {
        moveServo(LFK, 130, 400);
        sapaState = SAPA_DOWN;
      }
      break;

    case SAPA_DOWN:
      if (!LFK.moving) {
        sapaCount++;

        if (sapaCount >= 4) {
          sapaState = SAPA_DONE;
        } else {
          moveServo(LFK, 100, 400);
          sapaState = SAPA_UP;
        }
      }
      break;

    case SAPA_DONE:
      hai = false;
      sapaCount = 0;
      sapaState = SAPA_START;
      break;
  }
}

void updateOled() {
  if(!client.connected()) {
    sleep();
  } else if(hai) {
    excited();
  } else{
    mukaSenyum();
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  WiFi.begin(ssid, password);
if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
  while(1);
}
display.clearDisplay();
display.display();

  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callbackMqtt);
  connectMqtt();
  
  initServo(LF, 13, 105);
  initServo(RF, 12, 115);
  initServo(RB, 25, 65);
  initServo(LB, 15, 95);
  initServo(LFK, 17, 160);
  initServo(RFK, 16, 50);
  initServo(RBK, 19, 155);
  initServo(LBK, 18, 20);

}

void loop() {
  if(!client.connected()) {
    connectMqtt();
    sleep();
  } else{
    client.loop();
    mukaSenyum();
  }

  updateOled();

  updateAll();
  if(kiri) belokkiri();
  if(kanan) belokKanan();
  if(depan) maju();
  if(belakang) mundur();
  if(hai) sapa();

  plotter();

}
