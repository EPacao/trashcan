#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

int echoPin = D7; 
int trigPin = D6; 

int distanceThreshold = 6;
int cm_state = 0;
int percentage;
int duration;

static bool State_r;

// Вставка сюда учетныех данных сети
const char* ssid = "2065-IoT";   // SSID
const char* password = "123123123";  // пароль
//const char* ssid = "iPhone Андрюша";
//const char* password = "klepa400";

/************************* MQTT *********************************/
#define AIO_SERVER      "192.168.21.183"
#define AIO_SERVERPORT  1883
WiFiClient client_mqtt;
Adafruit_MQTT_Client mqtt(&client_mqtt, AIO_SERVER, AIO_SERVERPORT);
Adafruit_MQTT_Publish test = Adafruit_MQTT_Publish(&mqtt, "home/room/test"); //Настройка сети


// Запускаем бота
#define BOTtoken "1628681311:AAE9croZDoxiq5U1FIwlKr0R2BHrH7fiz0I"  // Вставляем токен бота.

// Используйте @myidbot, чтобы получить ID пользователя или группы
// Помните, что бот сможет вам писать только после нажатия
// вами кнопки /start
#define CHAT_ID "-680384801"
//#define CHAT_ID "535799793"
X509List cert(TELEGRAM_CERTIFICATE_ROOT);

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOTtoken, secured_client);

int botRequestDelay = 100;
unsigned long lastTimeBotRan;


// Задаем действия при получении новых сообщений
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    // Идентификатор чата запроса
    String chat_id = String(bot.messages[i].chat_id);
    //if (chat_id != CHAT_ID) {
    //  bot.sendMessage(chat_id, "Unauthorized user", "");
    //  continue;
    //}

    // Выводим полученное сообщение
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/state" || text == "/статус") {
      if (cm_state >= 3 && cm_state  <= 40)
      {
        bot.sendMessage(chat_id, "Урна заполнена на " + String(percentage) + "%" , "");
      }
    }
    if (text == "/help" || text == "/помоги") {
      bot.sendMessage(chat_id, "Привет, вот команды, которые я понимаю:");
      bot.sendMessage(chat_id, "/help или /помоги- подскажу, что умею");
      bot.sendMessage(chat_id, "/state или /статус - скажу на сколько заполенна урна");
    }
  }
}


void setup() {
  Serial.begin(115200);
  configTime(0, 0, "pool.ntp.org");      // получаем всемирное координированное время (UTC) через NTP
  secured_client.setTrustAnchors(&cert); // Получаем сертификат api.telegram.org

  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT); 
    
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  delay(2000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  Serial.println("MQTT connecting...");

  bot.sendMessage(CHAT_ID, "Привет, я система мониторинга урны. Чтобы узнать, что я умею напишите /help или /помощь", "");
}


void loop() {
    digitalWrite(trigPin, LOW); 
    delayMicroseconds(2); 
    digitalWrite(trigPin, HIGH); 
    delayMicroseconds(10); 
    digitalWrite(trigPin, LOW); 
    duration = pulseIn(echoPin, HIGH); 
    cm_state = duration / 58;
    Serial.print(cm_state); 
    Serial.println(" cm"); 
    percentage = map(cm_state , 6, 32, 100, 0); // расчет заполненности

    
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  if (cm_state >= distanceThreshold && State_r == true) {
    bot.sendMessage(CHAT_ID, "Урна пуста, спасибо!", "");
    State_r = false;
  }

  if (cm_state < distanceThreshold && State_r == false) {
    bot.sendMessage(CHAT_ID, "Урна заполнена, необходимо опустошить", "");
    State_r = true;
  }
  const char* mqtt_pubS;
  if (State_r) {
    mqtt_pubS = "full";
  } else {
    mqtt_pubS = "empty";
  }

  MQTT_connect();


  // Now we can publish stuff!
  if (! test.publish(mqtt_pubS)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  // wait a couple seconds to avoid rate limit
  delay(1000);   

}
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }

  Serial.println("MQTT Connected!");
}
