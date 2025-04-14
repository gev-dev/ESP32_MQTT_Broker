#include <LittleFS.h>

void setup() {
    // Инициализация Serial
    Serial.begin(115200);

    // Форматирование LittleFS
    if (!LittleFS.begin(true)) {  // true — форматировать, если не удается монтировать
        Serial.println("Ошибка инициализации LittleFS");
        return;
    }
    Serial.println("LittleFS успешно инициализировано!");
}
void loop(){}