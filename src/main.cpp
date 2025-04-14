#include <WiFi.h>
#include <PicoMQTT.h>
#include <GyverTimer.h>
#include <LittleFS.h>

#define SERIAL_BAUD 115200
#define LED_PIN 2

GTimer SendMsg(MS, 3000);
GTimer wifiCheckTimer(MS, 5000);
GTimer blinkTimer(MS, 500);
GTimer mqttBlinkTimer(MS, 100);

PicoMQTT::Server mqtt;

String ssid = "wifi";
String password = "wifiwifi";
String TOPIC = "server";
String MSG = "ESP Broker says Hello!";

bool wifiConnected = false;
bool ledState = false;
bool mqttActivity = false;

void saveWiFiCredentials(String s, String p) {
  File file = LittleFS.open("/wifi_config.txt", "w");
  if (file) {
    file.println(s);
    file.println(p);
    file.close();
    Serial.println("Wi-Fi данные сохранены в файл");
  } else {
    Serial.println("Ошибка при сохранении Wi-Fi данных");
  }
}

bool loadWiFiCredentials(String &s, String &p) {
  File file = LittleFS.open("/wifi_config.txt", "r");
  if (file) {
    s = file.readStringUntil('\n');
    p = file.readStringUntil('\n');
    file.close();
    s.trim();
    p.trim();
    return s.length() > 0 && p.length() > 0;
  }
  return false;
}

void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Пробуем подключиться к Wi-Fi...");
    WiFi.begin(ssid.c_str(), password.c_str());
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Инициализация LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Ошибка при монтировании LittleFS");
    return;
  }

  // Попытка загрузить сохранённые данные
  if (loadWiFiCredentials(ssid, password)) {
    Serial.println("Найдены сохранённые Wi-Fi данные");
    WiFi.mode(WIFI_STA);
    connectToWiFi();
  } else {
    Serial.println("Нет сохранённых данных Wi-Fi. Введите <SSID> <PASS> через пробел:");
    while (Serial.available() == 0) delay(100);
    String input = Serial.readStringUntil('\n');
    input.trim();
    int spaceIndex = input.indexOf(' ');
    if (spaceIndex > 0) {
      ssid = input.substring(0, spaceIndex);
      password = input.substring(spaceIndex + 1);
      saveWiFiCredentials(ssid, password);
      WiFi.mode(WIFI_STA);
      connectToWiFi();
    } else {
      Serial.println("Ошибка: неверный формат. Пример: MyWiFi mypassword");
    }
  }

  mqtt.subscribe("#", [](const char* topic, const char* payload) {
    Serial.printf("MQTT Приём из '%s': %s\n", topic, payload);
    mqttActivity = true;
    mqttBlinkTimer.reset();
    digitalWrite(LED_PIN, HIGH);
  });

  mqtt.begin();
}

void loop() {
  mqtt.loop();

  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      Serial.println("Wi-Fi отключён");
      wifiConnected = false;
    }

    if (wifiCheckTimer.isReady()) connectToWiFi();

    if (blinkTimer.isReady()) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }

  } else {
    if (!wifiConnected) {
      wifiConnected = true;
      Serial.printf("✅ Подключён к: %s, IP: %s\n", ssid.c_str(), WiFi.localIP().toString().c_str());
      digitalWrite(LED_PIN, HIGH);
    }

    if (SendMsg.isReady()) {
      mqtt.publish(TOPIC, MSG);
      // Serial.println("MQTT Отправка: " + MSG);
      mqttActivity = true;
      mqttBlinkTimer.reset();
      digitalWrite(LED_PIN, HIGH);
    }

    if (mqttActivity && mqttBlinkTimer.isReady()) {
      mqttActivity = false;
      digitalWrite(LED_PIN, HIGH);
    }
  }
}