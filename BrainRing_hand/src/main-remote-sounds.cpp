// for OTA go 
// http://192.168.3.209/ota_update

// the file firmware.bin is here 
// D:\nicelight_HDD_2021\projects\brainRing_NSK\BrainRing_hand\.pio\build\remote-sounds


//в VLC плеере треки 14-19 
//
//кнопки 
#define BUT1 4
#define BUT2 16
#define BUT3 17
#define BUT4 5
#define BUT5 18
#define BUT6 19


#define LED_BOARD 2
#include <WiFi.h>
#include <WiFiAP.h>
// раскомментировать для создания точки доступа
//#define ESP32_WIFIAP 
#ifdef ESP32_WIFIAP                
bool wifiHidden = 1;      // 1 -скрытая точка, 0 - видимая
byte maxConnection = 4;   // макимальное количество подключений 1..8
bool wifiChanel = 1;      // канал 1..11
const char* ssidAP = "esp32";
// const char* ssidAP = "esp32";
const char* passwordAP = "1234567812345678"; // пароль от 1 до 8 два раза
// Определяем точкy доступа
IPAddress wifiAPip(192, 168, 4, 1);
#else
// если не точка, то wifi client 
const char* ssid = "Showmix";
const char* password = "tol9igor";
IPAddress staticIP(192, 168, 3, 209);
IPAddress gateway(192, 168, 3, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 3, 1);
IPAddress dns2(8, 8, 8, 8);
#endif



// Подключем библиотеки
#include <WiFiClient.h>
#include <GyverPortal.h>
#include <LittleFS.h>
GyverPortal ui(&LittleFS); // для проверки файлов

// не асинхронный HTTP 
#include <HTTPClient.h>

const char GET_MediaServer[] = "http://192.168.3.210:8080/requests/playlist.xml?command=pl_play&id=";
uint16_t media_203 = 4; // номер трека в плейлисте. он там по идее с четверочки начинается

#define HTTPDEBUG // дебажим GET коннекшны

uint32_t ms = 0, photoMs = 0;


void tripleBlink() {
    digitalWrite(LED_BOARD, 0);
    delay(20);
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BOARD, 1);
    delay(50);
    digitalWrite(LED_BOARD, 0);
    delay(20);
  }
}//tripleBlink

// GET запрос медиа серверу с VLC player
void GETtoMedia(uint16_t param) {
  // формируем строку отправки на медиа сервер
  // http://192.168.3.210:8080/requests/playlist.xml?command=pl_play&id= 4
  String GETData = GET_MediaServer + String(param);
#ifdef HTTPDEBUG
  Serial.print(GETData);
  Serial.print("\n");
#endif
  for (int i = 0; i < 3; i++) {
#ifdef HTTPDEBUG
    Serial.print(i);
    Serial.printf(" try");
    Serial.printf("\n sending param GET: ");
#endif
    HTTPClient http;
    http.begin(GETData); //HTTP
    http.setAuthorization("", "123456789123456789"); // защищенное соединение, пароль указываем
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        i = 3;
        String payload = http.getString();
        tripleBlink(); //помигаем что все хорошо отправилось
#ifdef HTTPDEBUG    
        Serial.println(payload);
#endif  
        // // parsing GET response
        // int contentStart = payload.indexOf("\" du") - 1;
        // int contentEnd = payload.indexOf("\" du");
        // String content = payload.substring(contentStart, contentEnd);
        // // пытаемся найти первый здравый айдишник песни
        // int firstSongIDis = content.toInt();
      } else {
        Serial.printf(" sending GET err: %s\n", http.errorToString(httpCode).c_str());
        delay(700);
      }
    }
    http.end();
  }//for 5 times
}//GETtoMedia()


// калибровка сенсора приложенной руки 
uint8_t getTouchLevel(uint8_t skolkoProb) {
  uint16_t summ = 0;
  for (int i = 0; i < skolkoProb; i++) {
    summ += touchRead(13); // gpio13
  }
  summ /= skolkoProb;
  //  Serial.print(F("Hand sensor = "));
   // Serial.println(summ);
  return summ;
}

// страницу web портала строим
void webPageBuild() {
  // GP.BUILD_BEGIN(600);
  GP.BUILD_BEGIN();
  GP.THEME(GP_DARK);
  // GP.ONLINE_CHECK();   // проверять статус платы в назв вкладки
  //сначала страницу настроек пропишем, после в elif будет основная страница со вкладками и ссылкой на эту 
  GP.BLOCK_BEGIN(GP_TAB, "", "Обновить прошивку по wifi ", GP_RED);
  GP.BREAK();
  // GP_TEXT_HREF("/ota_update", "здесь");
  String s;
  s += F("<a href='/ota_update'>здесь</a>");
  GP.SEND(s);
  GP.BLOCK_END(); // блок с зеленым заголовком PHOTO

  GP.BUILD_END();
} // webPageBuild()


// обрабатываем действия на гайвер портале
void webPageAction() {

}//webPageAction()


// инициализируем гайвер портал
void webUI_Init() {
  ui.attachBuild(webPageBuild);
  ui.attach(webPageAction);
  ui.start();
  ui.enableOTA();   // без пароля
  if (!LittleFS.begin()) Serial.println("FS Error");
  ui.downloadAuto(true);
}//webUI_Init()


void wifiInit() {

#ifdef ESP32_WIFIAP
  // Инициируем точку доступа WiFi

  WiFi.softAP(ssidAP, passwordAP, wifiChanel, wifiHidden, maxConnection);
  // IPAddress myIP = WiFi.softAPIP();
  Serial.print("wifi_AP IP: ");
  Serial.println(WiFi.softAPIP());
#else 
  // Подключаемся к Wi-Fi
  Serial.print("try conn to ");
  Serial.print(ssid);
  Serial.print(":");
  WiFi.mode(WIFI_STA);
  if (WiFi.config(staticIP, gateway, subnet, dns1, dns2) == false) {
    Serial.println("wifi config failed.");
    return;
  }
  WiFi.begin(ssid, password);
  uint8_t trycon = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (trycon++ < 30) delay(500);
    else {
      Serial.print("no connection to Wifi. Esp restarts NOW!");
      delay(1000);
      ESP.restart();
    }
  }
  Serial.println("Connected. \nIP: ");

  // Выводим IP ESP32
  Serial.println(WiFi.localIP());
#endif
}//wifiInit()


void pinsBegin() {
  pinMode(LED_BOARD, OUTPUT);
  digitalWrite(LED_BOARD, 0);
  pinMode(BUT1, INPUT_PULLUP);
  pinMode(BUT2, INPUT_PULLUP);
  pinMode(BUT3, INPUT_PULLUP);
  pinMode(BUT4, INPUT_PULLUP);
  pinMode(BUT5, INPUT_PULLUP);
  pinMode(BUT6, INPUT_PULLUP);
}  //pinsBegin()


void setup() {
  // Инициируем последовательный порт
  Serial.begin(115200);
  wifiInit();
  webUI_Init();

  pinsBegin(); // все пины инициализируем

} // setup()



void loop() {
  ui.tick();
  ms = millis();
  if (!digitalRead(BUT1)) {
    delay(20);
    if (!digitalRead(BUT1)) {
      GETtoMedia(14);
      delay(100);
      while (!digitalRead(BUT1)) delay(50);
    }
  }
  if (!digitalRead(BUT2)) {
    delay(20);
    if (!digitalRead(BUT2)) {
      GETtoMedia(15);
      delay(100);
      while (!digitalRead(BUT2)) delay(50);
    }
  }
    if (!digitalRead(BUT3)) {
    delay(20);
    if (!digitalRead(BUT3)) {
      GETtoMedia(16);
      delay(100);
      while (!digitalRead(BUT3)) delay(50);
    }
  }
    if (!digitalRead(BUT4)) {
    delay(20);
    if (!digitalRead(BUT4)) {
      GETtoMedia(17);
      delay(100);
      while (!digitalRead(BUT4)) delay(50);
    }
  }
    if (!digitalRead(BUT5)) {
    delay(20);
    if (!digitalRead(BUT5)) {
      GETtoMedia(18);
      delay(100);
      while (!digitalRead(BUT5)) delay(50);
    }
  }
    if (!digitalRead(BUT6)) {
    delay(20);
    if (!digitalRead(BUT6)) {
      GETtoMedia(19);
      delay(100);
      while (!digitalRead(BUT6)) delay(50);
    }
  }

  // GETtoMedia(14);



} // loop()
