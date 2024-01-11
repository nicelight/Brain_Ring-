// for OTA go 
// http://192.168.3.201/ota_update
// the file firmware.bin is here 
// D:\nicelight_HDD_2021\projects\brainRing_NSK\BrainRing_hand\.pio\build\esp32doit-devkit-v1

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
IPAddress staticIP(192, 168, 3, 201);
IPAddress gateway(192, 168, 3, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 3, 1);
IPAddress dns2(8, 8, 8, 8);
#endif

// определение пинов 
#define PHOTOFLASH1 32 //white 
#define FOCUS 33 // red
#define SHOOTER 25 //black
//#define LED_PIN 2 // esp32 dev module LED_BUILTIN
#define STRIP_PIN 2// пин подключения ленты ws2812
#define NUM_LEDS 40 // сколько светодиодов в ленте ws2812


// #define USEASYNCHTTP
#ifdef USEASYNCHTTP
//для отправки асинхронных ( глючных)  HTTP запросов GET \ POST 
#define ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN_TARGET      "AsyncHTTPRequest_Generic v1.10.2"
#define ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN             1010002
// Seconds for timeout, default is 3s
#define DEFAULT_RX_TIMEOUT           10
// Uncomment for certain HTTP site to optimize
//#define NOT_SEND_HEADER_AFTER_CONNECTED        true
#endif

#include <FastLED.h>
CRGBArray<NUM_LEDS> leds;
#define DATA_PIN 111 // чтобы компилятор не матюгался
#define CLOCK_PIN 112

#include <EEPROM.h>
#include <EEManager.h>

// Подключем библиотеки
#include <WiFiClient.h>
#include <GyverPortal.h>
#include <LittleFS.h>
GyverPortal ui(&LittleFS); // для проверки файлов

#define COLORTHEMERELAX "#3995db" //цвет интерфейса зоны отдыха голубенький
#define COLORTHEMEGAME "#db7d39" //цвет интерфейса игры оранжевый
#define COLORTHEDISCO "#aa39db" // дискотека фиолетовый
// Создаём объект UDP cоединения
#include <WiFiUdp.h>
WiFiUDP udp;

// #define UDPDEBUG //отладка UDP

const uint16_t listenUDPport = 12345; // Определяем UDP порт, чет он не работает, происходит работа на 2390
const char* but203ip = "192.168.3.203";
const char* but204ip = "192.168.3.204";
uint16_t buttonsUDPport = 2390;
char       incomeBuffer[100]; // буфер отправки UDP пакета

#ifdef USEASYNCHTTP
// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include <AsyncHTTPRequest_Generic.h>             // https://github.com/khoih-prog/AsyncHTTPRequest_Generic
AsyncHTTPRequest request;
// test for checking HTTP requests
const char POST_ServerAddress[] = "dweet.io";
String dweetName = "/dweet/for/pinA0-Read?";
#else
// не асинхронный HTTP сдшуте
#include <HTTPClient.h>
#endif
//для отправки запроса на потолок -> 192.168.3.202
const char GET_ServerAddress[] = "http://192.168.3.202/win&";
char GETparamCHAR[7] = "PL=1"; // для тестов GET на потолок
char UDPparamCHAR[8] = "$8 0 1;"; // для тестов UDP кнопкам 

// #define HTTPDEBUG // дебажим GET коннекшны с потолком

// структура настроек
struct Settings {
  // фото
  uint16_t shooterTime = 500;
  uint16_t afterSensorTime = 100;
  uint16_t afterFlashDel = 10;
  uint16_t afterFocusDel = 100;
  // uint16_t sld = 1;
  ////////////// основные настройки игры
  uint32_t StartGameDelay = 5000;   // время отрисовки анимации на старте игры
  uint32_t AnswerReactDelay = 4000; // время отрисовки реакции на прав\неправ ответ 
  uint32_t FanfariDelay = 4000; // отрисовка фанфар
  uint32_t gameOverDelay = 3000; // отрисовка финальных салютов
  uint32_t buttonsDisabled = 3000; // отрисовка нажатия на кнопку какой либо команды
  ////////////// тонкие настройки игры
  // ========= ИГРА ===========================
  // ===============
  // =   Потолок   =
  // ===============
  // * START
  uint16_t Roof_start_dim = 200;    //потолок яркость на запуске игры
  uint16_t Roof_start_ef = 22;      //потолок эффект на запуске игры
  //ROUND игровой процесс
  uint16_t Roof_round_dim = 50; //потолок
  uint16_t Roof_round_ef = 21;   //потолок
  //ОТВЕЧАЮТ огонь
  uint16_t Roof_Answer_dim = 100;
  uint16_t Roof_203Answer_ef = 31;
  uint16_t Roof_204Answer_ef = 41;
  // ПРАВИЛЬНО переливы
  uint16_t Roof_trues_dim = 250;
  uint16_t Roof_203trues_ef = 32;
  uint16_t Roof_204trues_ef = 42;
  // НЕ_ВЕРНО снег
  uint16_t Roof_false_dim = 100;
  uint16_t Roof_false_ef = 33;
  // FANFARS Перебивка 
  uint16_t Roof_fanfar_dim = 200;
  uint16_t Roof_fanfar_ef = 22;
  // WINNER победа
  uint16_t Roof_wins_dim = 200;
  uint16_t Roof_wins1_ef = 23;
  uint16_t Roof_wins2_ef = 24;
  uint16_t Roof_wins3_ef = 25;
  // ================
  // = НЕБЕСНЫЕ 203 =
  // ================
  // * START
  uint16_t Btn203_round_par = 140;
  uint16_t Btn_Fanfars_par = 70;
  uint16_t Btn203_answer_par = 150;
  uint16_t Btn_start_dim = 200;
  uint16_t Btn_start_ef = 45;
  // * ROUND шторы 
  uint16_t Btn203_round_dim = 50;
  uint16_t Btn203_round_ef = 44;
  // * ОТВЕЧАЮТ огонь
  uint16_t Btn203_answer_dim = 100;
  uint16_t Btn203_answer_ef = 6;
  // * ПРАВИЛЬНО переливы
  uint16_t Btn203_trues_dim = 250;
  uint16_t Btn203_trues_ef = 19;
  // * НЕ_ВЕРНО снег
  uint16_t Btn_false_ef = 35;
  // * FANFARS Перебивка 
  uint16_t Btn_Fanfars_dim = 250;
  uint16_t Btn_Fanfars_ef = 17;
  // * WINNER победа
  uint16_t Btn_Win_dim = 250;
  uint16_t Btn_Win1_ef1 = 23;
  uint16_t Btn_Win1_ef2 = 21;
  uint16_t Btn_Win1_ef3 = 22;
  // ================== =
  // = МАЛИНОВЫЕ  204 =
  // ================== =
  // *START
  uint16_t Btn204_round_par = 210;
  uint16_t Btn204_answer_par = 230;
  // * ROUND шторы
  uint16_t Btn204_round_dim = 50;
  uint16_t Btn204_round_ef = 44;
  // * ОТВЕЧАЮТ огонь
  uint16_t Btn204_answer_dim = 100;
  uint16_t Btn204_answer_ef = 6;
  // *ПРАВИЛЬНО переливы
  uint16_t Btn204_trues_dim = 250;
  uint16_t Btn204_trues_ef = 13;
  //////////////////////////////////// конец преднастроек игры 

  uint8_t handLevel_gap = 5; // порог чувствительности сенсора руки


  // uint16_t 

  char GETrequest[50] = "A=10"; // для тестовой отправки GET
};
Settings sett; // инициализация структуры для  mem
EEManager memory(sett); // инициализация памяти

uint8_t photo_proc = 0; // автомат состояний фотографирования
uint32_t ms = 0, photoMs = 0;
bool needMakePhoto = 0; //разрешение сфоткать еще раз


// автомат игрового процесса game_proc
enum {
  INIT,
  START,
  STARTDEL,
  ROUND,
  ROUNDAWAIT,
  ANSWERED_DEL,
  RIGHTANSWER,
  ANSWERRESULTDEL,
  FANFARE,
  FANFAREDEL,
  GAMEOVER1,
  GAMEOVER2,
  GAMEOVER3,
  GAMEOVERSTOP
} game_proc;
uint32_t gameMs = 0;
bool gameIs = 0; //если игра запущена = 1
bool buttonsAllowed = 0; // флаг разрешения принятия нажатий кнопок
bool areWin = 0; // 1 команда ответила правильно
uint16_t whatButton = 0; // флаг 203 или 204 смотря кто нажал кнопку ( инфа прилетает с UDP)
// bool areWin = 0; // 1 команда выиграла, 2 команда проиграла. 0 - нет ответа
uint8_t pointsOf203 = 0, pointsOf204 = 0; // чет в игре 
String strUdp = " "; // для отправки UDP запроса
String strGET = " "; // для формирования GET запроса
uint16_t str_len = 0; // для формирования из стринга strGET массива char
bool allowHand = 1; // разрешить прикладывать руку


// автомат обработки руки 
enum {
  HANDINIT,
  HANDAWAIT,
  HAND1,
  HAND2,
  HAND3,
  HAND4,
  HAND5,
  HAND6,
  HAND7,
  HAND8,
  HAND9,
  HAND10,
  HANDRESTORE,
  READY1,
  READY2,
  READY3,
  TAKESHOT,
} hand_proc;
uint32_t handMs;
uint16_t totalPhotos = 0;
uint8_t getCurDim = 30; // единожды, когда руку приложили,  запомним, какой была яркость
uint8_t handLevel = 0; // значение датчика по умолчанию.  
uint8_t handCurrent = 0; // считываем показания сюда 
// sett.handLevel_gap = 5; // порог чувствительности 


// общие настройки
GPtime upd_UpTime;
uint8_t uptimeHour = 0, uptimeMin = 0, uptimeSec = 0;
uint32_t prevMs = 0, sec = 1;
uint32_t tmr; // для отладки

uint8_t prevgame_proc; // отладка
uint8_t prevhand_proc; // отладка
uint8_t prevhandCurrent; // отладка

// управление  светом 
uint8_t roofDim = 30; // управлене яркостью потолка с веб интерфейса слайдером
uint8_t roofEff = 1; // выбор эффектов на толокое
uint8_t BtnsDim = 30; // управлене яркостью кнопок
uint8_t BtnsEff = 1; // выбор эффектов кнопок
uint8_t roofDimIs = 100; // текущая яркость на потолке. ее получаем по GET запросу перед анимацией затухания
uint8_t hueHand = 27; // цвет руки 30
uint8_t dimHand = 50; // яркость руки 55


// заполнение ws2812 ленты цветом 
void ledsFill(uint8_t color, uint8_t bright) {
  for (int i = 0; i < NUM_LEDS; i++) {
    // let's set an led value
    // hue = 240 - красно малиноватый
    // hue = 135 - голубой
    // leds[i] = CHSV(hue++, 255, 255);
    leds[i] = CHSV(color, 255, bright);
  }
  FastLED.show();
}



// Мигаем, если данные пришли
void ledBlink() {
  ledsFill(135, 200); // голубой ярко
  delay(40);
  ledsFill(135, 0);
  delay(40);
  ledsFill(27, 55);
}//ledBlink()



void makePhoto() {
  delay(sett.afterSensorTime);
  ledsFill(135, 255); // голубой ярко мигнем
  digitalWrite(FOCUS, 1);
  delay(sett.afterFocusDel); // после фокусировки задержка
  //digitalWrite(LED_PIN, 1); // восстанавливаем сигнальный светодиод
  digitalWrite(SHOOTER, 1); // ФОТОГРАФИРУЕМ
  delay(sett.afterFlashDel); // задержка вспышки после шутера
  digitalWrite(PHOTOFLASH1, 1);  // сигнал на вспышку
  ledsFill(135, 0); // рукой потухнем
  delay(sett.shooterTime); // задержка удержания затвора
  ledsFill(27, 55); // голубой ярко мигнем

  digitalWrite(FOCUS, 0);
  digitalWrite(SHOOTER, 0);
  digitalWrite(PHOTOFLASH1, 0);
  totalPhotos++;
  sec = 0; // обнуляем инфо-таймер последней фотки
}// makePhoto()


#ifdef USEASYNCHTTP
// отправка GET | POST запроса на потолок 
void sendGETRequest(char* getParam1) {
  static bool requestOpenResult;
  if (request.readyState() == readyStateUnsent || request.readyState() == readyStateDone) {

    // пример отправка GET запроса
    //requestOpenResult = request.open("GET", "http://worldtimeapi.org/api/timezone/Europe/London.txt");

    String GETData = "";
    GETData += getParam1;
#ifdef HTTPDEBUG
    Serial.print("\n new char* GET: ");
    Serial.print(GET_ServerAddress);
    Serial.print(GETData);
#endif
    requestOpenResult = request.open("GET", (GET_ServerAddress + GETData).c_str());

    if (requestOpenResult) {
      // Only send() if open() returns true, or crash
      request.send();
    } else Serial.println(F("Can't send bad request"));
  } else Serial.println(F("Can't send request"));
}//sendGETRequest()



// отправка GET | POST запроса на потолок 
//в коде есть вот такая через передачу самого запроса аргументом
// void sendGETRequest(char* getParam1) {
  // а есть вот такая, через 2 параметра, которые застрингятся внутри
void sendParamGET(uint8_t type, uint16_t param) {
  static bool requestOpenResult;
  if (request.readyState() == readyStateUnsent || request.readyState() == readyStateDone) {
    // пример отправка GET запроса
    //requestOpenResult = request.open("GET", "http://worldtimeapi.org/api/timezone/Europe/London.txt");

    String GETData = "";

    switch (type) {
      // 1 -- отправляем на потолок яркость A=1
    case 1:
      GETData = "A=" + String(param);
      break;
      // 2 -- отправляем на потолок номер эффекта PL=1
    case 2:
      GETData = "PL=" + String(param);
      break;
    }
    GETData = GET_ServerAddress + GETData;
#ifdef HTTPDEBUG
    Serial.print("\nsending param GET: ");
    Serial.print(GET_ServerAddress);
    Serial.print(GETData);
#endif
    for (int i = 0; i < 3; i++) {
      // requestOpenResult = request.open("GET", (GET_ServerAddress + GETData).c_str());
      requestOpenResult = request.open("GET", GETData.c_str());
      if (requestOpenResult) {
        // Only send() if open() returns true, or crash
        request.send();
        i = 5;
      } else {
        Serial.print(F("Can't send GET "));
        Serial.print(i);
        Serial.println(F("-th time. Bad request"));
        Serial.print("\nGET req.: ");
        Serial.print(GETData);
        delay(1000);
      }
    }//for 5 times
  } else Serial.println(F("Can't send GET - Req. sender not ready"));
}//send_NEW_GETRequest()

#else 
// обычный не async клиент HTTP
// отправка GET | POST запроса на потолок 
void sendGETRequest(char* getParam1) {


  String GETData = GET_ServerAddress;
  GETData += getParam1;
#ifdef HTTPDEBUG
  Serial.print("\n new char* GET: ");
  Serial.print(GETData);
#endif

  for (int i = 0; i < 3; i++) {
    HTTPClient http;
    http.begin(GETData); //HTTP

    int httpCode = http.GET();
    if (httpCode > 0) {
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        i = 3;
#ifdef HTTPDEBUG
        String payload = http.getString();
        Serial.println(payload);
#endif
      }
    } else {
      Serial.printf("[HTTP] GET err: %s\n", http.errorToString(httpCode).c_str());
      delay(700);
    }
    http.end();
  }//for
}//sendGETRequest()




void sendParamGET(uint8_t type, uint16_t param) {
  String GETData = GET_ServerAddress;
  switch (type) {
    // 1 -- отправляем на потолок яркость A=1
  case 1:
    GETData += "A=" + String(param);
    break;
    // 2 -- отправляем на потолок номер эффекта PL=1
  case 2:
    GETData += "PL=" + String(param);
    break;
  }
  // GETData = GET_ServerAddress + GETData;
#ifdef HTTPDEBUG
  Serial.print("\n sending param GET: ");
  Serial.print(GETData);
  Serial.print("\n");
#endif
  for (int i = 0; i < 3; i++) {

    HTTPClient http;
    http.begin(GETData); //HTTP

    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        i = 3;
        String payload = http.getString();
#ifdef HTTPDEBUG    
        Serial.println(payload);
#endif  
        int contentStart = payload.indexOf("ac>");
        int contentEnd = payload.indexOf("</ac");
        String content = payload.substring(contentStart + 3, contentEnd);
        // если надо получить какое сейчас значение яркости на потолке ( это надо перед началом гашения)
        int somthing = content.toInt();
      } else {
        Serial.printf(" sending GET err: %s\n", http.errorToString(httpCode).c_str());
        delay(700);
      }
    }
    http.end();
  }//for 5 times
}//sendParamGET()



// GET запрос и распарсивание яркости потолка 
uint8_t getRoofDim() {
  for (int i = 0; i < 3; i++) {
    HTTPClient http;
    http.begin(GET_ServerAddress); //HTTP

    int httpCode = http.GET();
    // если получен ответ от сервера и он нормальный
    if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
      i = 3;
      String payload = http.getString();
      int contentStart = payload.indexOf("ac>");
      int contentEnd = payload.indexOf("</ac");
      // String content = responseText.substring(contentStart + 3, contentEnd);
      // вернем распарсенное значение яркости

      return (payload.substring(contentStart + 3, contentEnd)).toInt();
#ifdef HTTPDEBUG
      Serial.println(payload);
#endif
    } else {
      Serial.printf("[HTTP] GET err: %s\n", http.errorToString(httpCode).c_str());
      if (i != 2) delay(700); // задержка перед след GET пробой
    }
    http.end();
  }//for
  return 30; // отдадим предположительную яркость 30, так как не получили ответ от 
}//sendGETRequest()

#endif

// обработчик HTTP ответа от сервера ( от WLED потолка )
void parseHTTPResponse(String responseText) {
  //получаем значение яркости
  int contentStart = responseText.indexOf("ac>");
  int contentEnd = responseText.indexOf("</ac");
  String content = responseText.substring(contentStart + 3, contentEnd);
  // если надо получить какое сейчас значение яркости на потолке ( это надо перед началом гашения)
  // if (needrecieveBrightnes) {
  //   needrecieveBrightnes = 0;
  //   roofBrightness = content.toInt();
  // }
#ifdef HTTPDEBUG

  Serial.print("\n\nAC content is: ");
  Serial.println(content.toInt());
  Serial.println("roofBrightness: ");
  Serial.println(roofBrightness);
  Serial.print("\n\n");
#endif
}

#ifdef USEASYNCHTTP
// обработчик ответа от сервера на отправленный GET\POST запрос
void requestCB(void* optParm, AsyncHTTPRequest* request, int readyState) {
  (void)optParm;
  if (readyState == readyStateDone) {
    AHTTP_LOGDEBUG(F("\n**************************************"));
    AHTTP_LOGDEBUG1(F("Response Code = "), request->responseHTTPString());

    if (request->responseHTTPcode() == 200) {
      String responseText = request->responseText();
#ifdef HTTPDEBUG
      Serial.println(F("\nCODE==200***************************"));
      Serial.println(responseText);
      Serial.println(F("**************************************"));
#endif
      /*   реализация обработки HTTP ответа  тут
         https://github.com/khoih-prog/AsyncHTTPRequest_Generic/blob/master/examples/AsyncDweetPost_STM32/AsyncDweetPost_STM32.ino
        внутри ЭТОЙ функции добавим
        parseHTTPResponse(responseText);
        и выше опишем реализацию функции  parseHTTPResponse
      */
      parseHTTPResponse(responseText);
    } else {
      AHTTP_LOGERROR(F("HTTP Response error"));
    }
  }
}//requestCB

#endif



// const char * but203ip = "192.168.3.203";
// const char * but204ip = "192.168.3.204";
// uint16_t buttonsUDPport = 2390;

// // //отправка на айпишник ip  данных data ( не могу реализовать)
// void sendUDPchars(uint16_t dest, char& chars) {
//   String buttonIp = "";
//   if (dest == 203) {
//     udp.beginPacket(but203ip, buttonsUDPport);
//   }
//   udp.write((const uint8_t*)chars, strlen((const char*)chars)); //долго мучался
//   udp.endPacket();
//   delay(0);

//   // udp.printf(buf);
// }//sendUDPmsg()

//FROM GYVERpanel example of sending UDP string
void sendStringUDP(uint16_t dest, String& str) {

  uint16_t max_text_size = sizeof(incomeBuffer);        // Размер приемного буфера формирования текста отправляемой строки
  memset(incomeBuffer, '\0', max_text_size);
  str.toCharArray(incomeBuffer, str.length() + 1);
  //выбор кому отправлять
  switch (dest) {
  case 203:
    udp.beginPacket(but203ip, buttonsUDPport);
    break;
  case 204:
    udp.beginPacket(but204ip, buttonsUDPport);
    break;
  default:
#ifdef UDPDEBUG
    Serial.print(F("no IP destination of UDP. cancel"));
    return;
#endif
    break;
  }//switch(dest)
  udp.write((const uint8_t*)incomeBuffer, str.length() + 1);
  udp.endPacket();
  delay(0);
#ifdef UDPDEBUG
  Serial.print(String(F("sent UDP ")) + udp.remoteIP().toString() + ":" + String(udp.remotePort()) + " >> " + String(incomeBuffer));
#endif
}


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
  if (ui.uri("/set")) {
    // страница тонких настроек
    GP.BLOCK_BEGIN(GP_THIN, "", "Тонкие настройки", GP_RED); // блок с кр окантовкой
    GP.BREAK();
    GP.BREAK();
    GP.LABEL("Изменять допустимо");
    GP.LABEL("Только хозяину");
    GP.LABEL(" или директору!");
    GP.LABEL(" Ведется логирование");
    GP.LABEL(" и видео наблюдение");
    GP.LABEL("   :)   ");
    GP.BREAK();
    GP.BREAK();
    GP.BUTTON_LINK("/", "НАЗАД");
    GP.BREAK();
    GP.BREAK();
    GP.BLOCK_BEGIN(GP_TAB, "", "PHOTO"); // блок с зеленым заголовком
    GP.LABEL("Пауза между ");
    GP.LABEL("фокусировкой и шутером");
    GP.NUMBER("afterFocusDel", "number", sett.afterFocusDel);
    GP.LABEL(" мс");
    GP.BREAK();
    GP.LABEL("Удержание затвора");
    GP.NUMBER("uiShooterTime", "number", sett.shooterTime);
    GP.LABEL(" мс");
    GP.BREAK();
    GP.LABEL("Задержка вспышки ");
    GP.NUMBER("uiFlashDel", "number", sett.afterFlashDel);
    GP.LABEL(" мс");
    GP.BLOCK_END(); // блок с зеленым заголовком PHOTO

    GP.BREAK();
    GP.BREAK();
    GP.BREAK();

    GP.BLOCK_BEGIN(GP_TAB, "", "GAME"); // блок с зеленым заголовком
    //здесь тонкие настроки эффектов, применяемых в игре
    GP.LABEL("Тонкие настройки ");
    GP.LABEL("потолка и кнопок:");
    GP.LABEL("Эффекты(id) ,");
    GP.LABEL("их параметры(param) ,");
    GP.LABEL("и яркость(dim) .");
    GP.BLOCK_BEGIN(GP_TAB, "", "ПОТОЛОК ", GP_RED); // блок с КРАСНЫМ заголовком
    GP.BREAK();
    // * START
    GP.LABEL("Старт dim:");
    GP.NUMBER("Roof_start_dim", "number", sett.Roof_start_dim);
    GP.LABEL("Старт id:");
    GP.NUMBER("Roof_start_ef", "number", sett.Roof_start_ef);
    // * ROUND игровой процесс
    GP.LABEL("Раунд dim: ");
    GP.NUMBER("Roof_round_dim", "number", sett.Roof_round_dim);
    GP.LABEL("Раунд id: ");
    GP.NUMBER("Roof_round_ef", "number", sett.Roof_round_ef);
    // * ОТВЕЧАЮТ огонь
    GP.LABEL(" Отвечают dim: ");
    GP.NUMBER("Roof_Answer_dim", "number", sett.Roof_Answer_dim);
    GP.LABEL(" Отвечают 203ие id: ");
    GP.NUMBER("Roof_203Answer_ef", "number", sett.Roof_203Answer_ef);
    GP.LABEL(" Отвечают 204ые id:");
    GP.NUMBER("Roof_204Answer_ef", "number", sett.Roof_204Answer_ef);
    // * ПРАВИЛЬНО переливы
    GP.LABEL("Правильный ответ dim:");
    GP.NUMBER("Roof_trues_dim", "number", sett.Roof_trues_dim);
    GP.LABEL("Прав. ответ у 203их id: ");
    GP.NUMBER("Roof_203trues_ef", "number", sett.Roof_203trues_ef);
    GP.LABEL("Прав. ответ у 204ых id: ");
    GP.NUMBER("Roof_204trues_ef", "number", sett.Roof_204trues_ef);
    // * НЕ_ВЕРНО снег
    GP.LABEL(" не верный ответ dim: ");
    GP.NUMBER("Roof_false_dim", "number", sett.Roof_false_dim);
    GP.LABEL(" не верный ответ id: ");
    GP.NUMBER("Roof_false_ef", "number", sett.Roof_false_ef);
    // * FANFARS Перебивка 
    GP.LABEL(" Фанфары dim: ");
    GP.NUMBER("Roof_fanfar_dim", "number", sett.Roof_fanfar_dim);
    GP.LABEL("Фанфары id: ");
    GP.NUMBER("Roof_fanfar_ef", "number", sett.Roof_fanfar_ef);
    // * WINNER победа
    GP.LABEL("Победа dim:");
    GP.NUMBER("Roof_wins_dim", "number", sett.Roof_wins_dim);
    GP.LABEL("Победа I id: ");
    GP.NUMBER("Roof_wins1_ef", "number", sett.Roof_wins1_ef);
    GP.LABEL("Победа II id: ");
    GP.NUMBER("Roof_wins2_ef", "number", sett.Roof_wins2_ef);
    GP.LABEL("Победа III id: ");
    GP.NUMBER("Roof_wins3_ef", "number", sett.Roof_wins3_ef);
    GP.BLOCK_END(); // блок с КРАСНЫМ заголовком ПОТОЛОК

    GP.BREAK();
    GP.BREAK();

    GP.BLOCK_BEGIN(GP_TAB, "", "Кнопка 203 ", GP_RED); // блок с КРАСНЫМ заголовком
    GP.LABEL("Команда \"Небесные\" ");
    GP.HR();
    // ===================
    //  =  НЕБЕСНЫЕ  203 = 
    // ===================
    // * START
    GP.LABEL(" Старт dim: ");
    GP.LABEL("(для всех команд)");
    GP.NUMBER("Btn_start_dim", "number", sett.Btn_start_dim);
    GP.LABEL(" Старт id: ");
    GP.LABEL("(для всех команд)");
    GP.NUMBER("Btn_start_ef", "number", sett.Btn_start_ef);
    // * ROUND шторы 
    GP.LABEL("Раунд dim:");
    GP.NUMBER("Btn203_round_dim", "number", sett.Btn203_round_dim);
    GP.LABEL("Раунд id:");
    GP.NUMBER("Btn203_round_ef", "number", sett.Btn203_round_ef);
    GP.LABEL("param of \'Раунд \':");
    GP.NUMBER("Btn203_round_par", "number", sett.Btn203_round_par);
    // * ОТВЕЧАЮТ огонь
    GP.LABEL("Отвечают dim:");
    GP.NUMBER("Btn203_answer_dim", "number", sett.Btn203_answer_dim);
    GP.LABEL("Отвечают id:");
    GP.NUMBER("Btn203_answer_ef", "number", sett.Btn203_answer_ef);
    GP.LABEL("param of \'Отвечают \':");
    GP.NUMBER("Btn203_answer_par", "number", sett.Btn203_answer_par);
    // * ПРАВИЛЬНО переливы
    GP.LABEL("Правильно отв. dim:");
    GP.NUMBER("Btn203_trues_dim", "number", sett.Btn203_trues_dim);
    GP.LABEL("Правильно отв. id:");
    GP.NUMBER("Btn203_trues_ef", "number", sett.Btn203_trues_ef);
    // * НЕ_ВЕРНО снег
    GP.LABEL("Не правильно id:");
    GP.LABEL("(для всех команд)");
    GP.NUMBER("Btn_false_ef", "number", sett.Btn_false_ef);
    // * FANFARS Перебивка 
    GP.LABEL("Фанфары dim:");
    GP.LABEL("(для всех команд)");
    GP.NUMBER("Btn_Fanfars_dim", "number", sett.Btn_Fanfars_dim);
    GP.LABEL("Фанфары id:");
    GP.LABEL("(для всех команд)");
    GP.NUMBER("Btn_Fanfars_ef", "number", sett.Btn_Fanfars_ef);
    GP.LABEL("param of \'Фанфары \':");
    GP.LABEL("(для всех команд)");
    GP.NUMBER("Btn_Fanfars_par", "number", sett.Btn_Fanfars_par);
    // * WINNER победа
    GP.LABEL("Победа dim:");
    GP.LABEL("(для всех команд)");
    GP.NUMBER("Btn_Win_dim", "number", sett.Btn_Win_dim);
    GP.LABEL("Победа I id:");
    GP.LABEL("(для всех команд)");
    GP.NUMBER("Btn_Win1_ef1", "number", sett.Btn_Win1_ef1);
    GP.LABEL("Победа II id:");
    GP.LABEL("(для всех команд)");
    GP.NUMBER("Btn_Win1_ef2", "number", sett.Btn_Win1_ef2);
    GP.LABEL("(для всех команд)");
    GP.LABEL("Победа III id:");
    GP.NUMBER("Btn_Win1_ef3", "number", sett.Btn_Win1_ef3);
    GP.BLOCK_END();// блок с КРАСНЫМ заголовком Кнопка 203

    GP.BREAK();
    GP.BREAK();


    GP.BLOCK_BEGIN(GP_TAB, "", "Кнопка 204 ", GP_RED); // блок с КРАСНЫМ заголовком
    GP.LABEL(" Команда \"Малиновые\"");
    GP.HR();
    // ===================
    //  =  МАЛИНОВЫЕ  204 = 
    // ===================
    // * ROUND шторы 
    GP.LABEL("Раунд dim:");
    GP.NUMBER("Btn204_round_dim", "number", sett.Btn204_round_dim);
    GP.LABEL("Раунд id:");
    GP.NUMBER("Btn204_round_ef", "number", sett.Btn204_round_ef);
    GP.LABEL("param of \'Раунд \':");
    GP.NUMBER("Btn204_round_par", "number", sett.Btn204_round_par);
    // * ОТВЕЧАЮТ огонь
    GP.LABEL("Отвечают dim:");
    GP.NUMBER("Btn204_answer_dim", "number", sett.Btn204_answer_dim);
    GP.LABEL("Отвечают id:");
    GP.NUMBER("Btn204_answer_ef", "number", sett.Btn204_answer_ef);
    GP.LABEL("param of \'Отвечают \':");
    GP.NUMBER("Btn204_answer_par", "number", sett.Btn204_answer_par);
    // * ПРАВИЛЬНО переливы
    GP.LABEL("Правильно отв. dim:");
    GP.NUMBER("Btn204_trues_dim", "number", sett.Btn204_trues_dim);
    GP.LABEL("Правильно отв. id:");
    GP.NUMBER("Btn204_trues_ef", "number", sett.Btn204_trues_ef);
    GP.BLOCK_END();// блок с КРАСНЫМ заголовком Кнопка 204

    GP.BREAK();
    GP.BREAK();

    GP.BLOCK_END();// блок с зеленым заголовком GAME
    GP.BREAK();
    GP.BREAK();
    GP.BREAK();

    GP.BREAK();GP.BREAK();


    GP.BLOCK_BEGIN(GP_TAB, "", "Обновить прошивку по wifi ", GP_RED);
    GP.BREAK();
    // GP_TEXT_HREF("/ota_update", "здесь");
    String s;
    s += F("<a href='/ota_update'>здесь</a>");
    GP.SEND(s);


    GP.BREAK();
    GP.BLOCK_END();
    GP.BREAK();
    GP.BUTTON_LINK("/", "НАЗАД");
    GP.BREAK();
  }// страница тонких настроек


  // главная страница, корень, "/"
  else {
    GP.UPDATE("label1,label2,hh,mm,ss,gameled,GameStarted,pointsOf203,pointsOf204,HandWorks,roofBrightness,handLevel,handCurrent,handLevel_gap ");// какие поля нужно обновлять

    GP.PAGE_TITLE("Nicelight");
    GP.NAV_TABS_M("nav1", "Релакс,Игра,Дискотека,.", COLORTHEMERELAX);

    //============ вкладка РЕЛАКС =====================
    GP.NAV_BLOCK_BEGIN("nav1", 0);
    GP.TITLE("Комната отдыха");
    GP.BREAK();
    GP.LABEL("Аптайм:");
    GP.LABEL("hh", "hh");
    GP.LABEL("ч");
    GP.LABEL("mm", "mm");
    GP.LABEL("м");
    GP.LABEL("ss", "ss");
    GP.BREAK();
    GP.BREAK();
    GP.HR();
    GP.BOX_BEGIN(GP_JUSTIFY);
    GP.LABEL("разрешить РУКУ: ");
    GP.SWITCH("HandWorks", allowHand, COLORTHEMERELAX);
    GP.BOX_END();
    GP.BREAK();
    GP.LABEL("Нафоткал уже:");
    GP.LABEL("NAN", "label1");
    GP.LABEL("раз");
    GP.BREAK();
    GP.LABEL("Посл. фото:");
    GP.LABEL("NAN", "label2");
    GP.LABEL("сек. назад");
    GP.BREAK();
    GP.BUTTON("btn", "ФОТО :)", "", COLORTHEMERELAX);
    GP.BREAK();
    // вызов алерта по переключателю
    GP.ALERT("alertNoHand", "\n Сначала выключите игру. \n\n Рукой нельзя пользоваться во время игры.\n\n");
    GP.UPDATE_CLICK("alertNoHand", "HandWorks");

    GP.HR();
    GP.BREAK();
    GP.BREAK();

    GP.BLOCK_BEGIN(GP_TAB);
    GP.LABEL(" Яркость ### потолок ");
    GP.SLIDER("roofDim", roofDim, 0, 255, 1, 0, COLORTHEMERELAX); GP.BREAK();
    GP.BREAK();
    GP.BREAK();
    GP.LABEL(" Потолок id (1-99): ");
    GP.NUMBER("roofEff", "number", roofEff);
    GP.BLOCK_END();

    GP.BREAK();
    GP.HR();
    GP.BREAK();

    GP.BLOCK_BEGIN(GP_TAB);
    GP.LABEL("* Кнопки яркость *");
    GP.SLIDER("BtnsDim", BtnsDim, 0, 255, 1, 0, COLORTHEMERELAX); GP.BREAK();
    GP.BREAK();
    GP.BREAK();
    GP.LABEL("Кнопки id (1-47): ");
    GP.NUMBER("BtnsEff", "number", BtnsEff);
    GP.BLOCK_END();

    // потолок эффекты Relax
    GP.BLOCK_BEGIN(GP_TAB);
    GP.LABEL("Потолок ### пресеты: ");
    GP.HR();
    GP.BREAK();
    GP.BREAK();
    GP.BUTTON("relaxRoofef1", "1 Общий свет", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxRoofef2", "2 Лаунж", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxRoofef3", "3 Релакс", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxRoofef4", "4 Релакс", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxRoofef5", "5 Релакс", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxRoofef6", "6 Переливы", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxRoofef7", "7 Свечи", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxRoofef8", "8 Небо", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxRoofef9", "9 Звезды", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxRoofef10", "10 Лето", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BLOCK_END();

    // кнопки эффекты Relax
    GP.BLOCK_BEGIN(GP_TAB);
    GP.LABEL("* Кнопки пресеты: * ");
    GP.HR();
    GP.BREAK();
    GP.BREAK();
    GP.BUTTON("relaxButef1", "Моноцвет", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxButef2", "Матрица", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxButef3", "Снег", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxButef4", "Огонь", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxButef5", "Море", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxButef6", "Лава", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxButef7", "Небо", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxButef8", "Радуга", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxButef9", "Облака", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BUTTON("relaxButef10", "Феерверки", "", COLORTHEMERELAX);
    GP.BREAK();
    GP.BLOCK_END();

    GP.BREAK();
    GP.BREAK();
    GP.NAV_BLOCK_END();

    //============ вкладка ИГРА  =====================
    GP.NAV_BLOCK_BEGIN("nav1", 1);
    GP.TITLE("Брейн Ринг 2024");
    GP.BREAK();
    GP.BOX_BEGIN(GP_JUSTIFY);
    GP.LABEL("Идет игра: ");
    GP.SWITCH("GameStarted", gameIs, COLORTHEMEGAME);
    GP.BOX_END();
    GP.BREAK();
    GP.BREAK();
    GP.BUTTON("StartGame", "СТАРТ", "", COLORTHEMEGAME);
    GP.BREAK();
    GP.BREAK();
    GP.BREAK();

    GP.LABEL("  Счет: ");
    // GP.LABEL("NAN", "label1");
    GP.LABEL("204", "pointsOf204");
    GP.LABEL(" : ");
    GP.LABEL("203", "pointsOf203");
    GP.BREAK();
    GP.BREAK();
    GP.BREAK();
    GP.BREAK();
    GP.BREAK();
    GP.LABEL("правильный ответ?: ");
    // M_BOX(GP.BUTTON("AnswerNo", "НЕТ", "", "#c27d4c"); GP.BUTTON("AnswerYes", "ДА", "", "#f0863a"););
    M_BOX(GP.BUTTON("team204right", " 204 ", "", "#e10ec9"); GP.BUTTON("team203right", " 203 ", "", "#0e86e1"););
    GP.BREAK();
    GP.BREAK();
    GP.BREAK();
    GP.BREAK();
    GP.BREAK();
    GP.LABEL("Рекламная пауза");
    GP.BUTTON("Fanfars", "ФАНФАРЫ", "", COLORTHEMEGAME);
    GP.BREAK();
    GP.BREAK();
    GP.BREAK();
    GP.LABEL("Огласим победителя: ");
    GP.BUTTON("Winner", "ПОБЕДА !!!", "", COLORTHEMEGAME);
    GP.BREAK();
    GP.NAV_BLOCK_END();


    //============ вкладка ДИСКОТЕКА =====================
    GP.NAV_BLOCK_BEGIN("nav1", 2);
    GP.BREAK();

    GP.BLOCK_BEGIN(GP_TAB);
    GP.LABEL("Эффекты потолка");
    GP.HR();
    GP.BREAK();
    GP.BREAK();
    GP.BUTTON("discoRoofef1", "11 Веселый", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoRoofef2", "12 Быстрый", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoRoofef3", "13 Агро", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoRoofef4", "14 Мигалка", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoRoofef5", "15 СТРОБОСКОП", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoRoofef6", "16 Переливы", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoRoofef7", "17 Спокойный", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoRoofef8", "18 Нежный", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoRoofef9", "19 монотонный", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoRoofef10", "20 тьма", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BREAK();
    GP.BLOCK_END();

    GP.BREAK();
    GP.BREAK();

    GP.BLOCK_BEGIN(GP_TAB);
    GP.LABEL("Эффекты Кнопок");
    GP.HR();
    GP.BREAK();
    GP.BREAK();
    GP.BUTTON("discoButef1", "Светомузыка", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoButef2", "мячик", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoButef3", "синусы", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoButef4", "наливает RGB", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoButef5", "нервный Fire", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoButef6", "вагончики", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoButef7", "Полоски", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoButef8", "Шары", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoButef9", "звездопад", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BUTTON("discoButef10", "Лава", "", COLORTHEDISCO);
    GP.BREAK();
    GP.BLOCK_END();

    GP.BREAK();
    GP.NAV_BLOCK_END();


    //============ вкладка . НАСТРОЙКИ =====================
    GP.NAV_BLOCK_BEGIN("nav1", 3);
    GP.LABEL("Основные настройки");
    GP.BREAK();

    GP.BLOCK_BEGIN(GP_TAB, "", "   ИГРА   "); // блок с зеленым заголовком
    GP.BREAK();
    GP.LABEL("Длительность эффектов:");
    GP.BREAK();
    //здесь настроки таймингов, применяемых в игре
    GP.LABEL(" СТАРТ игры");
    GP.NUMBER("uiStartGameDelay", "number", sett.StartGameDelay);
    GP.LABEL(" мс");
    GP.BREAK();
    GP.LABEL(" Удержание нажатой кнопки");
    GP.NUMBER("uiButtonsDisabled", "number", sett.buttonsDisabled);
    GP.LABEL(" мс");
    GP.BREAK();
    GP.LABEL(" Правильный ответ");
    GP.NUMBER("uiAnswerReactDelay", "number", sett.AnswerReactDelay);
    GP.LABEL(" мс");
    GP.LABEL(" Фанфары");
    GP.NUMBER("uiFanfariDelay", "number", sett.FanfariDelay);
    GP.LABEL(" мс");
    GP.BREAK();
    GP.LABEL("Победные салюты");
    GP.LABEL(" 3 раза по: ");
    GP.NUMBER("uigameOverDelay", "number", sett.gameOverDelay);
    GP.LABEL(" мс");
    GP.BREAK();
    GP.BREAK();
    GP.BLOCK_END(); // блок с зеленым заголовком ИГРА


    GP.BREAK();
    GP.BREAK();

    GP.BLOCK_BEGIN(GP_TAB, "", "   ФОТО   "); // блок с зеленым заголовком
    //здесь настроки тайминга руки
    GP.BREAK();
    GP.LABEL("Датчик руки. ");
    GP.BREAK();
    GP.BREAK();
    GP.LABEL("сейчас значение:");
    GP.LABEL("handCurrent", "handCurrent");
    GP.BREAK();
    GP.LABEL("Уровень покоя: ");
    GP.LABEL("handLevel", "handLevel");
    GP.BREAK();
    GP.LABEL("Порог:");
    GP.NUMBER("handLevel_gap", "number", sett.handLevel_gap);
    GP.LABEL("Если значение ниже, чем");
    GP.LABEL("уровень минус порог,");
    GP.LABEL("датчик сработает.");
    GP.BREAK();
    GP.BREAK();
    GP.LABEL("Если постоянные");
    GP.LABEL("ложные  срабатывания,");
    GP.LABEL("установи выше порог или ");
    GP.BUTTON("calibrateHand", "перекалибруй ", "", "#474e41");
    GP.LABEL("уровень покоя руки.");
    GP.BREAK();

    GP.BREAK();
    GP.BREAK();
    GP.LABEL("Задержка после руки");
    GP.NUMBER("uiPhotoGap", "number", sett.afterSensorTime);
    GP.LABEL(" мс");
    GP.BREAK();
    GP.BLOCK_END(); // блок с зеленым заголовком ФОТО


    // GP.BLOCK_BEGIN(GP_TAB, "", "РЕЛАКС пресеты потолка "); // блок с зеленым заголовком
    // GP.BLOCK_END(); // блок с зеленым заголовком RELAX

    // GP.BLOCK_BEGIN(GP_TAB, "", " ДИСКОТЕКА  пресеты потолка"); // блок с зеленым заголовком
    // GP.BLOCK_END(); // блок с зеленым заголовком DISCO


    GP.BREAK();GP.BREAK();GP.BREAK();
    GP.HR();  GP.HR();  GP.HR();
    GP.BREAK();GP.BREAK();GP.BREAK();

    GP.BLOCK_BEGIN(GP_TAB, "", "CONNECTIONS API"); // блок с зеленым заголовком
    GP.BREAK();
    GP.LABEL(" Для тестов и проверки связи");
    GP.LABEL(" с потолком и кнопками");
    GP.BREAK();
    GP.BUTTON("get1-btn", "GET PL=1");
    GP.HR();
    GP.LABEL("GET для потолка to mem");
    GP.TEXT("getReq", "", sett.GETrequest); GP.BREAK();
    GP.BUTTON("get2-btn", "GET from mem");
    GP.HR();
    GP.LABEL("отправить GET: ");
    GP.TEXT("getNow", "", GETparamCHAR); GP.BREAK();

    GP.BREAK();
    GP.HR();
    GP.BREAK();

    GP.LABEL("UDP запрос к  3.203");
    GP.BUTTON("udp1-btn", "UDP \'$8 0 1;\'");
    // GP.BUTTON("udp2-btn", "UDP запрос(mem)");
    // GP.LABEL("GET для потолка(mem)");
    // GP.TEXT("getReq", "", sett.GETrequest); GP.BREAK();
    GP.LABEL("UDP to 3.203 ");
    GP.TEXT("udp203Now", "", UDPparamCHAR); GP.BREAK();
    GP.LABEL("UDP to 3.204 ");
    GP.TEXT("udp204Now", "", UDPparamCHAR); GP.BREAK();
    GP.BLOCK_END(); //// блок с кр окантовкой


    GP.BREAK();

    GP.BUTTON_LINK("/set", "Тонкие настройки");
    GP.BREAK();
    GP.NAV_BLOCK_END();

    //GP.SLIDER("sld", sett.sld);


  } // главная страница, корень, "/"

  GP.BUILD_END();
} // webPageBuild()


// обрабатываем действия на гайвер портале
void webPageAction() {

  if (ui.update()) {
    ui.updateInt("pointsOf203", pointsOf203);
    ui.updateInt("pointsOf204", pointsOf204);
    ui.updateInt("label1", totalPhotos);
    ui.updateInt("label2", sec);
    ui.updateInt("hh", uptimeHour);
    ui.updateInt("mm", uptimeMin);
    ui.updateInt("ss", uptimeSec);
    //  игровой процесс
    ui.updateBool("GameStarted", gameIs);
    ui.updateBool("HandWorks", allowHand);
    // датчик руки 
    ui.updateInt("handLevel", handLevel);
    ui.updateInt("handCurrent", handCurrent);
    ui.updateInt("sensorHand_gap", sett.handLevel_gap);
    // алерт о том что рукой нельзя пользоваться если игра идет 
    if (ui.update("alertNoHand")) {
      if (gameIs) {
        allowHand = 0;
        hand_proc = HANDINIT; // сброс автомата руки 
        ui.answer(1); // показать алерт (текст у него задан в конструкторе)
      }
    }
  }//update()

  if (ui.click()) {
    Serial.println("UI CLICK detected");

    // кнопки зоны RELAX - потолок
    if (ui.click("relaxRoofef1")) {
      sendParamGET(2, 1);    //эффект потолка 1
    }
    if (ui.click("relaxRoofef2")) {
      sendParamGET(2, 2);    //эффект потолка 2
    }
    if (ui.click("relaxRoofef3")) {
      sendParamGET(2, 3);    //эффект потолка 3
    }
    if (ui.click("relaxRoofef4")) {
      sendParamGET(2, 4);    //эффект потолка 
    }
    if (ui.click("relaxRoofef5")) {
      sendParamGET(2, 5);    //эффект потолка 
    }
    if (ui.click("relaxRoofef6")) {
      sendParamGET(2, 6);    //эффект потолка 
    }
    if (ui.click("relaxRoofef7")) {
      sendParamGET(2, 7);    //эффект потолка 
    }
    if (ui.click("relaxRoofef8")) {
      sendParamGET(2, 8);    //эффект потолка 
    }
    if (ui.click("relaxRoofef9")) {
      sendParamGET(2, 9);    //эффект потолка 
    }
    if (ui.click("relaxRoofef10")) {
      sendParamGET(2, 10);    //эффект потолка
    }

    // кнопки зоны RELAX - Кнопки
    if (ui.click("relaxButef1")) {
      strUdp = "$8 0 1;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("relaxButef2")) {
      strUdp = "$8 0 7;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("relaxButef3")) {
      strUdp = "$8 0 2;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("relaxButef4")) {
      strUdp = "$8 1 6 1;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      strUdp = "$8 0 6;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("relaxButef5")) {
      strUdp = "$8 0 19;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("relaxButef6")) {
      strUdp = "$8 0 13;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("relaxButef7")) {
      strUdp = "$8 0 12;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("relaxButef8")) {
      strUdp = "$8 0 11;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("relaxButef9")) {
      strUdp = "$8 0 17;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("relaxButef10")) {
      strUdp = "$8 0 23;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }


    // по нажатию на кнопку сфотографируем
    if (ui.click("btn")) {
      ledBlink(); // мигнем рукой синим цветом
      makePhoto(); // тестовая фотка 
    }
    // включатель прикладывания руки
    if (ui.clickBool("HandWorks", allowHand)) {
    }
    //  слайдер яркости потолка 
    if (ui.clickInt("roofDim", roofDim)) {
      // Serial.print("roof dim: ");
      // Serial.println(roofDim);
      sendParamGET(1, roofDim);   // 1 -- A=xxx, 2 -- PL=xxx 
    }
    //  слайдер яркости потолка 
    if (ui.clickInt("BtnsDim", BtnsDim)) {
      // Serial.print("butt dim: ");
      // Serial.println(roofDim);
      // установим яркость кнопкам
      strUdp = "$4 0 " + String(BtnsDim) + ";"; // подготовка шторы 203
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.clickInt("roofEff", roofEff)) {
      if ((roofEff > 0) && (roofEff < 100)) {
        sendParamGET(2, roofEff);   // 1 -- A=xxx, 2 -- PL=xxx 
      }
    }
    if (ui.clickInt("BtnsEff", BtnsEff)) {
      // Serial.print("BtnsEff");
      // Serial.println(BtnsEff);
      if ((BtnsEff > 0) && (BtnsEff <= 47)) {
        strUdp = "$8 0 " + String(BtnsEff) + ";"; // подготовка шторы 203
        sendStringUDP(203, strUdp);
        sendStringUDP(204, strUdp);
      }
    }



    // кнопки для игры
    //переключатель игру включить
    if (ui.clickBool("GameStarted", gameIs)) {
      if (gameIs) allowHand = 0; // запретим прикладывать руку
      else {
        game_proc = INIT;
        allowHand = 1;
      }
    }
    if (ui.click("StartGame")) {
      gameIs = 1; // игру начнем
      allowHand = 0; // запретим прикладывать руку
      game_proc = START;
    }
    if (ui.click("team204right")) {
      areWin = 1; // правильный ответ
      whatButton = 204;
    }
    if (ui.click("team203right")) {
      areWin = 1;  // не правильный ответ
      whatButton = 203;
    }
    if (ui.click("Fanfars")) {
      game_proc = FANFARE;  //рекламная пауза
    }
    if (ui.click("Winner")) {
      game_proc = GAMEOVER1;  //мигаем победителю. конец игры
    }


    //////////////////// Кнопки ДИСКОТЕКА
     // кнопки DISCO - потолок
    if (ui.click("discoRoofef1")) {
      sendParamGET(2, 11);    //эффект потолка 1
    }
    if (ui.click("discoRoofef2")) {
      sendParamGET(2, 12);    //эффект потолка 1
    }
    if (ui.click("discoRoofef3")) {
      sendParamGET(2, 13);    //эффект потолка 1
    }
    if (ui.click("discoRoofef4")) {
      sendParamGET(2, 14);    //эффект потолка 1
    }
    if (ui.click("discoRoofef5")) {
      sendParamGET(2, 15);    //эффект потолка 1
    }
    if (ui.click("discoRoofef6")) {
      sendParamGET(2, 16);    //эффект потолка 1
    }
    if (ui.click("discoRoofef7")) {
      sendParamGET(2, 17);    //эффект потолка 1
    }
    if (ui.click("discoRoofef8")) {
      sendParamGET(2, 18);    //эффект потолка 1
    }
    if (ui.click("discoRoofef9")) {
      sendParamGET(2, 19);    //эффект потолка 1
    }
    if (ui.click("discoRoofef110")) {
      sendParamGET(2, 20);    //эффект потолка 1
    }
    // кнопки DISCO - кнопки
    if (ui.click("discoButef1")) {
      strUdp = "$8 0 32;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("discoButef2")) {
      strUdp = "$8 0 5;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("discoButef3")) {
      strUdp = "$8 0 33;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("discoButef4")) {
      strUdp = "$8 0 34;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("discoButef5")) {
      strUdp = "$8 0 36;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("discoButef6")) {
      strUdp = "$8 0 37;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("discoButef7")) {
      strUdp = "$8 0 45;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("discoButef8")) {
      strUdp = "$8 0 22;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("discoButef9")) {
      strUdp = "$8 0 9;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }
    if (ui.click("discoButef10")) {
      strUdp = "$8 0 13;"; //эффект
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
    }



    /////////////// ВСЕ НАСТРОЙКИ ЗОНЫ RELAX
    ////////// основные настройки фотографирования и руки
        // перезапись из web в Memory 
    if (ui.clickInt("uiPhotoGap", sett.afterSensorTime)) {
      memory.updateNow(); //обновление настроек в EEPROM памяти
    }
    if (ui.clickInt("afterFocusDel", sett.afterFocusDel))  memory.updateNow();
    if (ui.clickInt("uiFlashDel", sett.afterFlashDel))  memory.updateNow();
    // перезапишем в shooterTime что ввели в интерфейсе
    if (ui.clickInt("uiShooterTime", sett.shooterTime))  memory.updateNow();
    // порог срабатывания руки
    if (ui.clickInt("handLevel_gap", sett.handLevel_gap))  memory.updateNow();
    // перекалибровка датчика руки
    if (ui.click("calibrateHand")) handLevel = getTouchLevel(100);


    ////////// основные настройки игры
         // перезапись из web в Memory 
    if (ui.clickInt("uiStartGameDelay", sett.StartGameDelay))  memory.updateNow();
    if (ui.clickInt("uiAnswerReactDelay", sett.AnswerReactDelay))  memory.updateNow();
    if (ui.clickInt("uiFanfariDelay", sett.FanfariDelay))  memory.updateNow();
    if (ui.clickInt("uigameOverDelay", sett.gameOverDelay))  memory.updateNow();
    if (ui.clickInt("uiButtonsDisabled", sett.buttonsDisabled))  memory.updateNow();

    ////////// тонкие настройки игры 
    // Потолок обновляем в память параметры игры
    if (ui.clickInt("Roof_start_dim", sett.Roof_start_dim))  memory.updateNow();
    if (ui.clickInt("Roof_start_ef", sett.Roof_start_ef))  memory.updateNow();
    if (ui.clickInt("Roof_round_dim", sett.Roof_round_dim))  memory.updateNow();
    if (ui.clickInt("Roof_round_ef", sett.Roof_round_ef))  memory.updateNow();
    if (ui.clickInt("Roof_Answer_dim", sett.Roof_Answer_dim))  memory.updateNow();
    if (ui.clickInt("Roof_203Answer_ef", sett.Roof_203Answer_ef))  memory.updateNow();
    if (ui.clickInt("Roof_204Answer_ef", sett.Roof_204Answer_ef))  memory.updateNow();
    if (ui.clickInt("Roof_trues_dim", sett.Roof_trues_dim))  memory.updateNow();
    if (ui.clickInt("Roof_203trues_ef", sett.Roof_203trues_ef))  memory.updateNow();
    if (ui.clickInt("Roof_204trues_ef", sett.Roof_204trues_ef))  memory.updateNow();
    if (ui.clickInt("Roof_false_dim", sett.Roof_false_dim))  memory.updateNow();
    if (ui.clickInt("Roof_false_ef", sett.Roof_false_ef))  memory.updateNow();
    if (ui.clickInt("Roof_fanfar_dim", sett.Roof_fanfar_dim))  memory.updateNow();
    if (ui.clickInt("Roof_fanfar_ef", sett.Roof_fanfar_ef))  memory.updateNow();
    if (ui.clickInt("Roof_wins_dim", sett.Roof_wins_dim))  memory.updateNow();
    if (ui.clickInt("Roof_wins1_ef", sett.Roof_wins1_ef))  memory.updateNow();
    if (ui.clickInt("Roof_wins2_ef", sett.Roof_wins2_ef))  memory.updateNow();
    if (ui.clickInt("Roof_wins3_ef", sett.Roof_wins3_ef))  memory.updateNow();


    //"Команда \"Небесные\"   обновляем в память параметры игры
    if (ui.clickInt("Btn_start_dim", sett.Btn_start_dim))  memory.updateNow();
    if (ui.clickInt("Btn_start_ef", sett.Btn_start_ef))  memory.updateNow();
    if (ui.clickInt("Btn203_round_dim", sett.Btn203_round_dim))  memory.updateNow();
    if (ui.clickInt("Btn203_round_ef", sett.Btn203_round_ef))  memory.updateNow();
    if (ui.clickInt("Btn203_round_par", sett.Btn203_round_par))  memory.updateNow();
    if (ui.clickInt("Btn203_answer_dim", sett.Btn203_answer_dim))  memory.updateNow();
    if (ui.clickInt("Btn203_answer_ef", sett.Btn203_answer_ef))  memory.updateNow();
    if (ui.clickInt("Btn203_answer_par", sett.Btn203_answer_par))  memory.updateNow();
    if (ui.clickInt("Btn203_trues_dim", sett.Btn203_trues_dim))  memory.updateNow();
    if (ui.clickInt("Btn203_trues_ef", sett.Btn203_trues_ef))  memory.updateNow();
    if (ui.clickInt("Btn_false_ef", sett.Btn_false_ef))  memory.updateNow();
    if (ui.clickInt("Btn_Fanfars_dim", sett.Btn_Fanfars_dim))  memory.updateNow();
    if (ui.clickInt("Btn_Fanfars_ef", sett.Btn_Fanfars_ef))  memory.updateNow();
    if (ui.clickInt("Btn_Fanfars_par", sett.Btn_Fanfars_par))  memory.updateNow();
    if (ui.clickInt("Btn_Win_dim", sett.Btn_Win_dim))  memory.updateNow();
    if (ui.clickInt("Btn_Win1_ef1", sett.Btn_Win1_ef1))  memory.updateNow();
    if (ui.clickInt("Btn_Win1_ef2", sett.Btn_Win1_ef2))  memory.updateNow();
    if (ui.clickInt("Btn_Win1_ef3", sett.Btn_Win1_ef3))  memory.updateNow();

    //"Команда \"Малиновые\"   обновляем в память параметры игры
    if (ui.clickInt("Btn204_round_dim", sett.Btn204_round_dim))  memory.updateNow();
    if (ui.clickInt("Btn204_round_ef", sett.Btn204_round_ef))  memory.updateNow();
    if (ui.clickInt("Btn204_round_par", sett.Btn204_round_par))  memory.updateNow();
    if (ui.clickInt("Btn204_answer_dim", sett.Btn204_answer_dim))  memory.updateNow();
    if (ui.clickInt("Btn204_answer_ef", sett.Btn204_answer_ef))  memory.updateNow();
    if (ui.clickInt("Btn204_answer_par", sett.Btn204_answer_par))  memory.updateNow();
    if (ui.clickInt("Btn204_trues_dim", sett.Btn204_trues_dim))  memory.updateNow();
    if (ui.clickInt("Btn204_trues_ef", sett.Btn204_trues_ef))  memory.updateNow();


    ////////// тонкие настройки CONNECTIONS API 
    // ПРИМЕР ячейки для записи get запроса в  память
    // допустим "PL=2"
    if (ui.clickStr("getReq", sett.GETrequest)) {
      memory.updateNow(); //обновление настроек в EEPROM памяти
    }
    // отправка GET2 запроса который сохранен в память из строки выше
    if (ui.click("get2-btn")) {
      sendGETRequest(sett.GETrequest);
    }
    //отправка строки GET запросом
    if (ui.clickStr("getNow", GETparamCHAR)) {
      sendGETRequest(GETparamCHAR);
    }
    // отправка статического GET запроса 
    if (ui.click("get1-btn")) {
      strcpy(GETparamCHAR, "PL=1");
      sendGETRequest(GETparamCHAR);
    }

    //отправка статического UDP запроса на 203
    if (ui.click("udp1-btn")) {
      String temp = "$4 0 255;"; //4 0 N яркость
      sendStringUDP(203, temp);
      delay(20);
      temp = "$8 0 1;"; //$8 0 N; - включить эффект 1 моноцвет0
      sendStringUDP(203, temp);
    }
    if (ui.clickStr("udp203Now", UDPparamCHAR)) {
      String temp = UDPparamCHAR;
      sendStringUDP(203, temp);
    }
    if (ui.clickStr("udp204Now", UDPparamCHAR)) {
      String temp = UDPparamCHAR;
      sendStringUDP(204, temp);
    }

  }//click()
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



//распарсивание udp пакета 
void parseUdpMsg() {

  char incomeBuffer[50];  // buffer for incoming packets

  int packetSize = udp.parsePacket();
  if (packetSize) {
    // receive incoming UDP packets
#ifdef UDPDEBUG
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, udp.remoteIP().toString().c_str(), udp.remotePort());
#endif
    String cmd = "";
    if (udp.read(incomeBuffer, 48) > 0) {
      cmd = incomeBuffer;
#ifdef UDPDEBUG
      Serial.printf("UDP String content   cmd: %s\n", cmd);
#endif
      //смотрим, не пришел ли сигнал от кнопки
      // ибо еще приходит всякая отладка
      int index = cmd.indexOf("button");
      if (index > 0) {
        cmd = cmd.substring(index + 6, index + 9); //обрежем, чтобы получить 203 или 204 
        if (buttonsAllowed) {
          whatButton = cmd.toInt();   // получаем 203 или 204
        }
#ifdef UDPDEBUG
        Serial.print("start index of command \'button\': ");
        Serial.println(index);
        Serial.print("String cmd is: ");
        Serial.println(cmd);
#endif 
      } else {
#ifdef UDPDEBUG
        Serial.printf("not BUTTON content: %s\n", cmd);
#endif 
      }
#ifdef UDPDEBUG
      Serial.printf("Button: %i\n", whatButton);
#endif
      udp.flush(); // важно чистить буфер после прочтения
    }//read
  }//if packet
}//parseUdpMsg()


void pinsBegin() {
  pinMode(PHOTOFLASH1, OUTPUT);
  digitalWrite(PHOTOFLASH1, 0);
  pinMode(FOCUS, OUTPUT);
  digitalWrite(FOCUS, 0);
  pinMode(SHOOTER, OUTPUT);
  digitalWrite(SHOOTER, 0);
}  //pinsBegin()


void setup() {
  // Инициируем последовательный порт
  Serial.begin(115200);
  EEPROM.begin(2000);  // выделить память (больше или равно размеру даты)
  memory.begin(0, 'c'); // изменить символ чтобы память переинициализировалась из скетча
  wifiInit();
  webUI_Init();

  Serial.println("\n\nБрейнринг сервер 2024\n\n");
  FastLED.addLeds<NEOPIXEL, STRIP_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
  pinsBegin(); // все пины инициализируем
  handLevel = getTouchLevel(100); // калибровка сенсора руки

  udp.begin(listenUDPport); // слушаем UDP на 12345 порту 
#ifdef USEASYNCHTTP
  //инициализация АСИНХРОННЫХ HTTP запросов на потолок 
  // чет они глючно работают, библиотека мудреная, стек у нее зависает
  // request.setDebug(false);
  request.setDebug(true);
  //обработчик ответов на HTTP запросы
  request.onReadyStateChange(requestCB);
#endif

  parseUdpMsg(); // слушаем входящие по udp 
  makePhoto(); // тестовая фотка 
  game_proc = INIT;// автомат игрового процесса 

  //для отладки, увидеть что есп перезагрузилась
  // strcpy(GETparamCHAR, "PL=42");
  // sendGETRequest(GETparamCHAR);
  strUdp = "$8 0 35;";   // отправим на кнопки мигалку
  sendStringUDP(203, strUdp); // отправка на кнопки 
  sendStringUDP(204, strUdp);
} // setup()



void loop() {
  ui.tick();
  memory.tick();
  ms = millis();
  parseUdpMsg();

  // отладочка 
  static bool printIt = 1; // отладка вообще нужна?
  if (printIt && ((millis() - tmr >= 1000ul))) {
    tmr = millis();
    if (game_proc != prevgame_proc) {
      prevgame_proc = game_proc;
      Serial.print("\t\t\t\t game: ");
      Serial.print(game_proc);
      Serial.print(",\t buttonsAllowed: ");
      Serial.print(buttonsAllowed);

      Serial.print(",\t whatButton ");
      Serial.print(whatButton);
      Serial.print("\n");
    }//game_proc

      // // hand_proc
      // prevhand_proc = hand_proc;
      // prevhandCurrent = handCurrent;
      // Serial.print(F("\t\t\t\t\t\thand: "));
      // Serial.print(hand_proc);
      // Serial.print(F(";\thandLevel: "));
      // Serial.print(handLevel);
      // Serial.print(F(",  gap: "));
      // Serial.print(sett.handLevel_gap);
      // Serial.print(F(",  cur: "));
      // Serial.print(handCurrent);
      // Serial.print(F(",  gotfromRoof: "));
      // Serial.print(getCurDim);
      // Serial.print("\n");

  }// ms



  // инкрементируем Аптайм
  if ((ms - prevMs) > 1000) {
    prevMs = ms;
    sec++;
    uptimeSec++;
    //Serial.print("photo_proc:");
    //Serial.println(photo_proc);
    if (uptimeSec > 59) {
      uptimeSec = 0;
      uptimeMin++;
      if (uptimeMin > 59) {
        uptimeMin = 0;
        uptimeHour++;
        Serial.printf("%lu hours %lu mins\n", uptimeHour, uptimeMin);
      }
    }  //if sec
  }//ms 1000



//////////////////////////////// процесс РУКА 
  switch (hand_proc) {
    // установим свет руки в обычный 
  case HANDINIT:
    ledsFill(27, 55);
    handMs = ms;
    hand_proc = HANDAWAIT;
    break;

    //ждем руку
  case HANDAWAIT:
    if ((ms - handMs) > 500ul) {
      handMs = ms;
      //если рука РАЗРЕШЕНА
      if (allowHand) {
        // находим уровень датчика руки
        handCurrent = getTouchLevel(5); // 5 проб
        // руку приложили уровень упал
        if (handCurrent < (handLevel - sett.handLevel_gap)) {
          for (int i = 0; i < NUM_LEDS; i++) {// отрисуем бегущую черную точку
            leds[i] = CHSV(27, 255, 0);
            FastLED.show();
            delay(3);
          }//for
          ledsFill(27, 55); // заполнение оранжевым
          //
          getCurDim = getRoofDim(); // получим яркость с потолка 
          //
          hueHand = 27; // начальный цвет руки оранжевый
          dimHand = 55; // начальная яркость руки
          ledsFill(hueHand, dimHand);
          hand_proc = HAND1;
        }//if hand touched
      }//if allow
      else hand_proc = HANDINIT;
    }//if ms 
    break;

    // рука приложена продолжаем, яркость тушим до 200
  case HAND1:
    if ((ms - handMs) > 500ul) {
      handMs = ms;
      handCurrent = getTouchLevel(5); // находим уровень датчика руки, 5 проб
      if (allowHand && (handCurrent < (handLevel - sett.handLevel_gap))) {
        if (getCurDim > 200) sendParamGET(1, 200);    //яркость потолка 200
        hueHand -= 3;
        dimHand += 10;
        ledsFill(hueHand, dimHand);
        hand_proc = HAND2;
      }//if hand touched
      else {
        hand_proc = HANDRESTORE;
      }
    }//if ms
    break;

    // рука приложена -> dim decrement to 100
  case HAND2:
    if ((ms - handMs) > 500ul) {
      handMs = ms;
      handCurrent = getTouchLevel(5); // находим уровень датчика руки, 5 проб
      if (allowHand && (handCurrent < (handLevel - sett.handLevel_gap))) {
        if (getCurDim > 100) sendParamGET(1, 100);    //яркость потолка 100
        hueHand -= 3;
        dimHand += 10;
        ledsFill(hueHand, dimHand);
        hand_proc = HAND3;

      }//if hand touched
      else {
        hand_proc = HANDRESTORE;
      }
    }//if ms
    break;

    // рука приложена -> dim decrement to 50
  case HAND3:
    if ((ms - handMs) > 500ul) {
      handMs = ms;
      handCurrent = getTouchLevel(5); // находим уровень датчика руки, 5 проб
      if (allowHand && (handCurrent < (handLevel - sett.handLevel_gap))) {
        if (getCurDim > 50) sendParamGET(1, 50);    //яркость потолка 50
        hueHand -= 3;
        dimHand += 10;
        ledsFill(hueHand, dimHand);
        hand_proc = HAND4;

      }//if hand touched
      else {
        hand_proc = HANDRESTORE;
      }
    }//if ms
    break;

    // рука приложена -> dim decrement to 10
  case HAND4:
    if ((ms - handMs) > 500ul) {
      handMs = ms;
      handCurrent = getTouchLevel(5); // находим уровень датчика руки, 5 проб
      if (allowHand && (handCurrent < (handLevel - sett.handLevel_gap))) {
        if (getCurDim > 10) sendParamGET(1, 10);    //яркость потолка 10
        hueHand -= 3;
        dimHand += 10;
        ledsFill(hueHand, dimHand);
        hand_proc = HAND5;

      }//if hand touched
      else {
        hand_proc = HANDRESTORE;
      }
    }//if ms
    break;

    // рука приложена -> dim decrement to 5
  case HAND5:
    if ((ms - handMs) > 500ul) {
      handMs = ms;
      handCurrent = getTouchLevel(5); // находим уровень датчика руки, 5 проб
      if (allowHand && (handCurrent < (handLevel - sett.handLevel_gap))) {
        if (getCurDim > 5) sendParamGET(1, 5);    //яркость потолка 5
        hueHand -= 3;
        dimHand += 10;
        ledsFill(hueHand, dimHand);
        hand_proc = HAND6;

      }//if hand touched
      else {
        hand_proc = HANDRESTORE;
      }
    }//if ms
    break;

    // рука приложена -> dim decrement to 4
  case HAND6:
    if ((ms - handMs) > 500ul) {
      handMs = ms;
      handCurrent = getTouchLevel(5); // находим уровень датчика руки, 5 проб
      if (allowHand && (handCurrent < (handLevel - sett.handLevel_gap))) {
        if (getCurDim > 4) sendParamGET(1, 4);    //яркость потолка 4
        hueHand -= 3;
        dimHand += 10;
        ledsFill(hueHand, dimHand);
        hand_proc = HAND7;

      }//if hand touched
      else {
        hand_proc = HANDRESTORE;
      }
    }//if ms
    break;

    // рука приложена -> dim decrement to 3
  case HAND7:
    if ((ms - handMs) > 500ul) {
      handMs = ms;
      handCurrent = getTouchLevel(5); // находим уровень датчика руки, 5 проб
      if (allowHand && (handCurrent < (handLevel - sett.handLevel_gap))) {
        if (getCurDim > 3) sendParamGET(1, 3);    //яркость потолка 3
        hueHand -= 3;
        dimHand += 10;
        ledsFill(hueHand, dimHand);
        hand_proc = HAND8;

      }//if hand touched
      else {
        hand_proc = HANDRESTORE;
      }
    }//if ms
    break;
    // рука приложена -> dim decrement to 2
  case HAND8:

    if ((ms - handMs) > 500ul) {
      handMs = ms;
      handCurrent = getTouchLevel(5); // находим уровень датчика руки, 5 проб
      if (allowHand && (handCurrent < (handLevel - sett.handLevel_gap))) {
        if (getCurDim > 2) sendParamGET(1, 2);    //яркость потолка 2
        hueHand -= 3;
        dimHand += 10;
        ledsFill(hueHand, dimHand);
        hand_proc = HAND9;

      }//if hand touched
      else {
        hand_proc = HANDRESTORE;
      }
    }//if ms
    break;
    // рука приложена -> dim decrement to 1
  case HAND9:

    if ((ms - handMs) > 500ul) {
      handMs = ms;
      handCurrent = getTouchLevel(5); // находим уровень датчика руки, 5 проб
      if (allowHand && (handCurrent < (handLevel - sett.handLevel_gap))) {
        if (getCurDim > 1) sendParamGET(1, 1);    //яркость потолка 1
        hueHand -= 3;
        dimHand += 10;
        ledsFill(hueHand, dimHand);
        hand_proc = HAND10;

      }//if hand touched
      else {
        hand_proc = HANDRESTORE;
      }
    }//if ms
    break;
    // рука приложена -> dim decrement to 0
  case HAND10:

    if ((ms - handMs) > 500ul) {
      handMs = ms;
      handCurrent = getTouchLevel(5); // находим уровень датчика руки, 5 проб
      if (allowHand && (handCurrent < (handLevel - sett.handLevel_gap))) {
        sendParamGET(1, 0);    //яркость потолка 0
        hueHand -= 3;
        dimHand += 10;
        ledsFill(hueHand, dimHand);
        strUdp = "$4 0 0;"; //яркость лент 0 
        sendStringUDP(203, strUdp);
        sendStringUDP(204, strUdp);
        sendStringUDP(203, strUdp);
        sendStringUDP(204, strUdp);
        hand_proc = READY1;
      }//if hand touched
      else {
        hand_proc = HANDRESTORE;
      }
    }//if ms
    break;

    //готовим ребят, руку больше не проверяем, пыхаем им в лицо
  case READY1:
    if ((ms - handMs) > 700ul) {
      handMs = ms;
      strUdp = "$4 0 50;"; //мигаем
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      delay(200);
      strUdp = "$4 0 0;"; //мигаем
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      hand_proc = READY2;
    }
    break;
    //готовим ребят, руку больше не проверяем, пыхаем им в лицо
  case READY2:
    if ((ms - handMs) > 700ul) {
      handMs = ms;
      strUdp = "$4 0 100;"; //мигаем
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      delay(200);
      strUdp = "$4 0 0;"; //мигаем
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      hand_proc = READY3;
      strcpy(GETparamCHAR, "TT=0");
      sendGETRequest(GETparamCHAR); // &TT=0

    }
    break;
    //готовим 3 раз, пыхаем им в лицо
  case READY3:
    if ((ms - handMs) > 700ul) {
      handMs = ms;
      strUdp = "$4 0 250;"; //мигаем
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      delay(200);
      strUdp = "$4 0 0;"; //мигаем
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      sendParamGET(1, 250); // потолок на полную
      hand_proc = TAKESHOT;
    }
    break;
    //фоткаем наконец то 
  case TAKESHOT:

    makePhoto();

    //восстановим яркость столбов 
    strUdp = "$4 0 50;"; //мигаем
    sendStringUDP(203, strUdp);
    sendStringUDP(204, strUdp);
    sendStringUDP(203, strUdp);
    sendStringUDP(204, strUdp);
    // восстановим транзишн потолка
    strcpy(GETparamCHAR, "TT=500");
    sendGETRequest(GETparamCHAR); // &TT=500
    handMs = ms;
    hand_proc = HANDRESTORE;
    break;

    // восстановим яркость потолка если рука не приложена
  case HANDRESTORE:
    if ((ms - handMs) > 500ul) {
      handMs = ms;

      sendParamGET(1, getCurDim);    //яркость восстановим до прошлого раза
      hand_proc = HANDINIT;
    }//ms
    break;
  }//hand_proc



    //автомат игровой процесс
  switch (game_proc) {
  case INIT:
    gameMs = ms;
    break;
  case START:
    // ПОДГОТОВКИ кнопок
    //отправляем несколько раз так как нет подтверждения доставки пакета
    for (uint8_t i = 0; i < 2; i++) {
      // strUdp = "$8 1 44 140;"; // подготовка шторы 203
      strUdp = "$8 1 44 " + String(sett.Btn203_round_par) + ";"; // подготовка шторы 203
      sendStringUDP(203, strUdp);
      // strUdp = "$8 1 44 210;"; // подготовка шторы 204
      strUdp = "$8 1 44 " + String(sett.Btn204_round_par) + ";"; // подготовка шторы 204
      sendStringUDP(204, strUdp);
      delay(20);
      // strUdp = "$8 1 17 70;";  // подготовка перебивки 
      strUdp = "$8 1 17 " + String(sett.Btn_Fanfars_par) + ";";  // подготовка перебивки 
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      delay(20);
      // strUdp = "$8 1 6 150;";  //  подготовка огонь 203
      strUdp = "$8 1 6 " + String(sett.Btn203_answer_par) + ";";  //  подготовка огонь 203
      sendStringUDP(203, strUdp);
      // strUdp = "$8 1 6 230;";  //  подготовка огонь 204
      strUdp = "$8 1 6 " + String(sett.Btn204_answer_par) + ";";  //  подготовка огонь 204
      sendStringUDP(204, strUdp);
      delay(20);
      //  ЗАПУСК кнопок
      // strUdp = "$4 0 200;";  //  яркость
      strUdp = "$4 0 " + String(sett.Btn_start_dim) + ";";  //  яркость
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      delay(20);
      // strUdp = "$8 0 45;";  //  эффект START RGB worms
      strUdp = "$8 0 " + String(sett.Btn_start_ef) + ";";  //  эффект START RGB worms
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      delay(50);
    }
    // // отправим на потолок команду.
    // старый вариант
    // strcpy(GETparamCHAR, "PL=42");
    // sendGETRequest(GETparamCHAR);

    // новый вариант
    // sendParamGET(1, sett.Roof_start_dim);   // 1 -- A=xxx, 2 -- PL=xxx
    sendParamGET(2, sett.Roof_start_ef);    // 
    game_proc = STARTDEL;
    pointsOf203 = 0;
    pointsOf204 = 0;
    gameMs = ms;
    break;

  case STARTDEL:
    // ждем пока отиграет эффект старта 
    if ((ms - gameMs) > sett.StartGameDelay) {
      gameMs = ms;
      game_proc = ROUND;
    }
    break;
  case ROUND:
    // запускаем шторы Раунда
    for (uint8_t i = 0; i < 2; i++) {
      // strUdp = "$4 0 50;";  //  яркость
      strUdp = "$4 0 " + String(sett.Btn203_round_dim) + ";";  //  яркость
      sendStringUDP(203, strUdp);
      strUdp = "$4 0 " + String(sett.Btn204_round_dim) + ";";  //  яркость
      sendStringUDP(204, strUdp);
      delay(20);
      // strUdp = "$8 0 44;";  //  эффект Шторы
      strUdp = "$8 0 " + String(sett.Btn203_round_ef) + ";";  //  эффект Шторы
      sendStringUDP(203, strUdp);
      strUdp = "$8 0 " + String(sett.Btn204_round_ef) + ";";  //  эффект Шторы
      sendStringUDP(204, strUdp);
      delay(50);
    }
    // //потолок белый
    // sendParamGET(1, sett.Roof_round_dim);
    sendParamGET(2, sett.Roof_round_ef);
    whatButton = 0; // кто именно нажал
    buttonsAllowed = 1; // разрешаем нажатия на кнопки 
    areWin = 0; // сброс флага ответа от тамады
    gameMs = ms;
    game_proc = ROUNDAWAIT;
    break;

    //  + String(sett.Btn203_round_par) + ";"

  // ждем нажатия на кнопку
  case ROUNDAWAIT:
    //админ нажал кнопку правильного ответа
    if (areWin) {
      gameMs = ms;
      game_proc = RIGHTANSWER;
    }
    //пришло нажатие от 203 или 204
    else if (whatButton) {
      if (whatButton == 203) {
        //мигнем поярче моноцветом
        strUdp = "$4 0 200;";  //  яркость
        sendStringUDP(whatButton, strUdp);
        strUdp = "$8 0 1;"; // моноцвет - FLASH
        sendStringUDP(whatButton, strUdp);
        delay(50);
        for (uint8_t i = 0; i < 2; i++) {
          // strUdp = "$4 0 150;";  //  яркость
          strUdp = "$4 0 " + String(sett.Btn203_answer_dim) + ";";  //  яркость
          sendStringUDP(whatButton, strUdp);
          delay(20);
          // strUdp = "$8 0 6;";  //  эффект Огонь
          strUdp = "$8 0 " + String(sett.Btn203_answer_ef) + ";";  //  эффект Огонь
          sendStringUDP(whatButton, strUdp);
          delay(50);
        }
        // //отправляем на потолок
        sendParamGET(2, sett.Roof_203Answer_ef);    // 1 -- A=xxx, 2 -- PL=xxx
      }//if 203
      // нажатие от 204
      else if (whatButton == 204) {
        //мигнем поярче моноцветом
        strUdp = "$4 0 200;";  //  яркость
        sendStringUDP(whatButton, strUdp);
        strUdp = "$8 0 1;"; // моноцвет - FLASH
        sendStringUDP(whatButton, strUdp);
        delay(50);
        for (uint8_t i = 0; i < 2; i++) {
          // strUdp = "$4 0 150;";  //  яркость
          strUdp = "$4 0 " + String(sett.Btn204_answer_dim) + ";";  //  яркость
          sendStringUDP(whatButton, strUdp);
          delay(20);
          // strUdp = "$8 0 6;";  //  эффект Огонь
          strUdp = "$8 0 " + String(sett.Btn204_answer_ef) + ";";  //  эффект Огонь
          sendStringUDP(whatButton, strUdp);
          delay(50);
        }
        // //отправляем на потолок
        sendParamGET(2, sett.Roof_204Answer_ef);    // 1 -- A=xxx, 2 -- PL=xxx 
      }//if 204
      whatButton = 0; // чтобы снова не прорисовывать эффект нажатия
      buttonsAllowed = 0; // кнопки не реагируют  установленное время
      areWin = 0; // новое ожидание ответа ведущего
      gameMs = ms;
      game_proc = ANSWERED_DEL;
    }//whatButton
    // если тамада нажал "Правы 203" или "Правы 204"
    break;

    // ожидаем отрисовку нажатой кнопки
  case ANSWERED_DEL:
    // ждем пока отиграет эффект нажатой кнопки 
    if ((ms - gameMs) > sett.buttonsDisabled) {
      gameMs = ms;
      whatButton = 0; // никакая кнопка не активна
      buttonsAllowed = 1; // разрешим
      game_proc = ROUND;
    }
    break;

  case RIGHTANSWER:
    if (whatButton == 203) {
      pointsOf203++; // добавим одно очко команде
      for (uint8_t i = 0; i < 2; i++) {
        // strUdp = "$4 0 250;";  //  яркость 
        strUdp = "$4 0 " + String(sett.Btn203_trues_dim) + ";";  //  яркость 
        sendStringUDP(whatButton, strUdp);
        delay(20);
        // strUdp = "$8 0 19;";  // переливы 203их
        strUdp = "$8 0 " + String(sett.Btn203_trues_ef) + ";";  // переливы 203их
        sendStringUDP(whatButton, strUdp);
        delay(50);
      }
      // на потолок
      sendParamGET(2, sett.Roof_203trues_ef);          // 1 -- A=xxx, 2 -- PL=xxx
    } else if (whatButton == 204) {
      // правильно ответили 204ые
      pointsOf204++; // добавим одно очко команде
      for (uint8_t i = 0; i < 2; i++) {
        // strUdp = "$4 0 250;";  //  яркость 
        strUdp = "$4 0 " + String(sett.Btn204_trues_dim) + ";";  //  яркость 
        sendStringUDP(whatButton, strUdp);
        delay(20);
        // strUdp = "$8 0 13;";  // переливы 204ых
        strUdp = "$8 0 " + String(sett.Btn204_trues_ef) + ";";  // переливы 204ых
        sendStringUDP(whatButton, strUdp);
        delay(50);
      }
      // потолок
      sendParamGET(2, sett.Roof_204trues_ef);    // 1 -- A=xxx, 2 -- PL=xxx 
    }
    gameMs = ms;
    game_proc = ANSWERRESULTDEL;
    break;

  case ANSWERRESULTDEL:
    // отрисовка правильного ответа
    if ((ms - gameMs) > sett.AnswerReactDelay) {
      gameMs = ms;
      game_proc = ROUND;
    }
    break;

  case FANFARE:
    // фанфары ( рекламная пауза )
    for (uint8_t i = 0; i < 2; i++) {
      // strUdp = "$4 0 250;";  // снег 203их
      strUdp = "$4 0 " + String(sett.Btn_Fanfars_dim) + ";";  // снег 203их
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      delay(20);
      // strUdp = "$8 0 17;";  // снег 203их
      strUdp = "$8 0 " + String(sett.Btn_Fanfars_ef) + ";";  // снег 203их
      sendStringUDP(203, strUdp);
      sendStringUDP(204, strUdp);
      delay(50);
    }
    gameMs = ms;
    game_proc = FANFAREDEL;
    // потолок фанфары
    // sendParamGET(1, sett.Roof_fanfar_dim);   // 1 -- A=xxx, 2 -- PL=xxx
    sendParamGET(2, sett.Roof_fanfar_ef);    // 
    break;
  case FANFAREDEL:
    if ((ms - gameMs) > sett.FanfariDelay) {
      gameMs = ms;
      game_proc = ROUND;
    }
    break;
  case GAMEOVER1:
    //победили 203ие
    if (pointsOf203 > pointsOf204) {
      for (uint8_t i = 0; i < 2; i++) {
        // strUdp = "$4 0 250;";  // яркость МАКС
        strUdp = "$4 0 " + String(sett.Btn_Win_dim) + ";";  // яркость МАКС
        sendStringUDP(203, strUdp);
        delay(20);
        // strUdp = "$8 0 23;";  // победа
        strUdp = "$8 0 " + String(sett.Btn_Win1_ef1) + ";";  // победа
        sendStringUDP(203, strUdp);
        delay(50);
      }
    }
    // победили 204е
    else if (pointsOf204 > pointsOf203) {
      for (uint8_t i = 0; i < 2; i++) {
        // strUdp = "$4 0 250;";  // яркость МАКС
        strUdp = "$4 0 " + String(sett.Btn_Win_dim) + ";";  // яркость МАКС
        sendStringUDP(204, strUdp);
        delay(20);
        // strUdp = "$8 0 23;";  // победа
        strUdp = "$8 0 " + String(sett.Btn_Win1_ef1) + ";";  // победа
        sendStringUDP(204, strUdp);
        delay(50);
      }
    }
    //ничья, мигаем обоими 
    else {
      for (uint8_t i = 0; i < 2; i++) {
        // strUdp = "$4 0 250;";  // яркость МАКС
        strUdp = "$4 0 " + String(sett.Btn_Win_dim) + ";";  // яркость МАКС
        sendStringUDP(203, strUdp);
        sendStringUDP(204, strUdp);
        delay(20);
        // strUdp = "$8 0 23;";  // победа
        strUdp = "$8 0 " + String(sett.Btn_Win1_ef1) + ";";  // победа
        sendStringUDP(203, strUdp);
        sendStringUDP(204, strUdp);
        delay(50);
      }
    }
    // strcpy(GETparamCHAR, "PL=2");
    // на потолок  победа
    // sendParamGET(1, sett.Roof_wins_dim);   // 1 -- A=xxx, 2 -- PL=xxx
    sendParamGET(2, sett.Roof_wins1_ef);    // 
    gameMs = ms;
    game_proc = GAMEOVER2;
    break;
  case GAMEOVER2:
    if ((ms - gameMs) > sett.gameOverDelay) {
      //победили 203ие
      if (pointsOf203 > pointsOf204) {
        for (uint8_t i = 0; i < 2; i++) {
          // strUdp = "$4 0 250;";  // яркость МАКС
          strUdp = "$4 0 " + String(sett.Btn_Win_dim) + ";";  // яркость МАКС
          sendStringUDP(203, strUdp);
          delay(20);
          // strUdp = "$8 0 23;";  // победа
          strUdp = "$8 0 " + String(sett.Btn_Win1_ef2) + ";";  // победа
          sendStringUDP(203, strUdp);
          delay(50);
        }
      }
      // победили 204е
      else if (pointsOf204 > pointsOf203) {
        for (uint8_t i = 0; i < 2; i++) {
          // strUdp = "$4 0 250;";  // яркость МАКС
          strUdp = "$4 0 " + String(sett.Btn_Win_dim) + ";";  // яркость МАКС
          sendStringUDP(204, strUdp);
          delay(20);
          // strUdp = "$8 0 23;";  // победа
          strUdp = "$8 0 " + String(sett.Btn_Win1_ef2) + ";";  // победа
          sendStringUDP(204, strUdp);
          delay(50);
        }
      }
      //ничья, мигаем обоими 
      else {
        for (uint8_t i = 0; i < 2; i++) {
          // strUdp = "$4 0 250;";  // яркость МАКС
          strUdp = "$4 0 " + String(sett.Btn_Win_dim) + ";";  // яркость МАКС
          sendStringUDP(203, strUdp);
          sendStringUDP(204, strUdp);
          delay(20);
          // strUdp = "$8 0 23;";  // победа
          strUdp = "$8 0 " + String(sett.Btn_Win1_ef2) + ";";  // победа
          sendStringUDP(203, strUdp);
          sendStringUDP(204, strUdp);
          delay(50);
        }
      }
      // strcpy(GETparamCHAR, "PL=2");
      // на потолок  победа
      // sendParamGET(1, sett.Roof_wins_dim);   // 1 -- A=xxx, 2 -- PL=xxx
      sendParamGET(2, sett.Roof_wins2_ef);    // 
      gameMs = ms;
      game_proc = GAMEOVER3;
    }
    break;
  case GAMEOVER3:
    if ((ms - gameMs) > sett.gameOverDelay) {
      //победили 203ие
      if (pointsOf203 > pointsOf204) {
        for (uint8_t i = 0; i < 2; i++) {
          // strUdp = "$4 0 250;";  // яркость МАКС
          strUdp = "$4 0 " + String(sett.Btn_Win_dim) + ";";  // яркость МАКС
          sendStringUDP(203, strUdp);
          delay(20);
          // strUdp = "$8 0 23;";  // победа
          strUdp = "$8 0 " + String(sett.Btn_Win1_ef3) + ";";  // победа
          sendStringUDP(203, strUdp);
          delay(50);
        }
      }
      // победили 204е
      else if (pointsOf204 > pointsOf203) {
        for (uint8_t i = 0; i < 2; i++) {
          // strUdp = "$4 0 250;";  // яркость МАКС
          strUdp = "$4 0 " + String(sett.Btn_Win_dim) + ";";  // яркость МАКС
          sendStringUDP(204, strUdp);
          delay(20);
          // strUdp = "$8 0 23;";  // победа
          strUdp = "$8 0 " + String(sett.Btn_Win1_ef3) + ";";  // победа
          sendStringUDP(204, strUdp);
          delay(50);
        }
      }
      //ничья, мигаем обоими 
      else {
        for (uint8_t i = 0; i < 2; i++) {
          // strUdp = "$4 0 250;";  // яркость МАКС
          strUdp = "$4 0 " + String(sett.Btn_Win_dim) + ";";  // яркость МАКС
          sendStringUDP(203, strUdp);
          sendStringUDP(204, strUdp);
          delay(20);
          // strUdp = "$8 0 23;";  // победа
          strUdp = "$8 0 " + String(sett.Btn_Win1_ef3) + ";";  // победа
          sendStringUDP(203, strUdp);
          sendStringUDP(204, strUdp);
          delay(50);
        }
      }
      // strcpy(GETparamCHAR, "PL=2");
      // на потолок  победа
      // sendParamGET(1, sett.Roof_wins_dim);   // 1 -- A=xxx, 2 -- PL=xxx
      sendParamGET(2, sett.Roof_wins3_ef);    // 
      gameMs = ms;
      game_proc = GAMEOVERSTOP;
    }
    break;
  case GAMEOVERSTOP:
    if ((ms - gameMs) > sett.gameOverDelay) {
      gameIs = 0; // выключаем игру 
      allowHand = 1; // активируем прикладывание руки
      gameMs = ms;
      game_proc = INIT;
    }
    break;

  }//switch(game_proc)



  //автомат:  ловим единичку, фоткаем, отдыхаем 3 сек
  switch (photo_proc) {
  case 0:
    photo_proc = 1;
    break;
  case 1:
    if (needMakePhoto) {
      //digitalWrite(LED_PIN, 1);
      makePhoto(); // фотографируем 
      Serial.printf("\t%lu:%lu .%lu \t  => PHOTO \n", uptimeHour, uptimeMin, uptimeSec);
      photoMs = ms;
      photo_proc = 5;
    }
    break;
  case 5:
    // пауза 1 секунду, чтобы игнорировать повторные сообщения
    if ((ms - photoMs) > 1000ul) {
      photoMs = ms;
      //digitalWrite(LED_PIN, 0);
      needMakePhoto = 0; // обнуляем 
      photo_proc = 1; // GO на исходную
    }//ms 3000
    break;
  }//switch(photo_proc)

} // loop()
