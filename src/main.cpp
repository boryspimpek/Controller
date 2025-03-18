#include <esp_now.h>
#include <WiFi.h>

// Definicje pinów przycisków
#define L1_PIN 18  
#define L2_PIN 16
#define L3_PIN 17
#define L4_PIN 21

#define R1_PIN 14
#define R2_PIN 27
#define R3_PIN 13
#define R4_PIN 26

// Definicje pinów joysticków
#define JOY1_X 32
#define JOY1_Y 33
#define JOY2_X 34
#define JOY2_Y 35

// Przycisk do przełączania między odbiornikami
#define SWITCH_DEVICE_PIN 21  

// Adresy MAC odbiorników
uint8_t macOdbiorcaESP32[] = {0x5C, 0x01, 0x3B, 0x6C, 0x1C, 0x48};  // Adres MAC odbiornika ESP32
uint8_t macOdbiorcaESP8266[] = {0x48, 0xE7, 0x29, 0x46, 0x66, 0x8D};  // Wpisz tutaj prawidłowy adres MAC odbiornika ESP8266

// Zmienna określająca, który odbiornik jest aktywny
bool useESP32Receiver = true;  // true = ESP32, false = ESP8266

typedef struct struct_message {
    bool L1Pressed;
    bool L2Pressed;
    bool L3Pressed;
    bool L4Pressed;
    bool R1Pressed;
    bool R2Pressed;
    bool R3Pressed;
    bool R4Pressed;
    int joy1_x;
    int joy1_y;
    int joy2_x;
    int joy2_y;
} struct_message;

struct_message command;
struct_message lastCommand;  // Przechowuje poprzedni stan dla porównania

esp_now_peer_info_t peerInfo;

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    
    //Serial.print("Wysyłanie do ");
    //Serial.print(macStr);
    //Serial.print(": ");
    //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUKCES" : "BŁĄD");
}

float mapJoystick(int rawValue, int rawMin, int rawMax, int center, int mappedMin, int mappedMax) {
    if (rawValue < center) {  
        return map(rawValue, rawMin, center, mappedMin, 0);  
    } else {  
        return map(rawValue, center, rawMax, 0, mappedMax);  
    }
}

void addPeer(uint8_t* macAddress) {
    // Usuń wszystkich odbiorców przed dodaniem nowego
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    
    // Kopiuj adres MAC odbiorcy
    memcpy(peerInfo.peer_addr, macAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    // Usuń poprzedni peer jeśli istnieje
    esp_now_del_peer(macAddress);
    
    // Dodaj nowego odbiorcy
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Nie można dodać odbiorcy!");
    } else {
        Serial.print("Aktywny odbiornik: ");
        Serial.print(macAddress[0], HEX);
        for (int i = 1; i < 6; i++) {
            Serial.print(":");
            Serial.print(macAddress[i], HEX);
        }
        Serial.println();
    }
}

void switchReceiver() {
    useESP32Receiver = !useESP32Receiver;
    
    if (useESP32Receiver) {
        Serial.println("Przełączono na odbiornik ESP32");
        addPeer(macOdbiorcaESP32);
    } else {
        Serial.println("Przełączono na odbiornik ESP8266");
        addPeer(macOdbiorcaESP8266);
    }
}

bool lastSwitchState = HIGH;

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);  

    Serial.print("Mój adres MAC: ");
    Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("Błąd inicjalizacji ESP-NOW!");
        return;
    }

    esp_now_register_send_cb(onSent);

    // Domyślnie ustaw ESP32 jako odbiornik
    addPeer(macOdbiorcaESP32);

    // Konfiguracja pinów przycisków
    pinMode(L1_PIN, INPUT_PULLUP);
    pinMode(L2_PIN, INPUT_PULLUP);
    pinMode(L3_PIN, INPUT_PULLUP);
    pinMode(L4_PIN, INPUT_PULLUP);
    pinMode(R1_PIN, INPUT_PULLUP);
    pinMode(R2_PIN, INPUT_PULLUP);
    pinMode(R3_PIN, INPUT_PULLUP);
    pinMode(R4_PIN, INPUT_PULLUP);
    pinMode(SWITCH_DEVICE_PIN, INPUT_PULLUP);  // Przycisk przełączania

    // Konfiguracja pinów joysticków
    pinMode(JOY1_X, INPUT);
    pinMode(JOY1_Y, INPUT);
    pinMode(JOY2_X, INPUT);
    pinMode(JOY2_Y, INPUT);
    
    Serial.println("Kontroler gotowy. Domyślny odbiornik: ESP32");
    Serial.println("Naciśnij przycisk na pinie " + String(SWITCH_DEVICE_PIN) + " aby przełączyć odbiornik");
}

void loop() {
    // Sprawdź stan przycisku przełączania
    bool currentSwitchState = digitalRead(SWITCH_DEVICE_PIN);
    if (currentSwitchState == LOW && lastSwitchState == HIGH) {
        // Przycisk został wciśnięty - przełącz odbiornik
        switchReceiver();
        delay(200);  // Prosta debounce
    }
    lastSwitchState = currentSwitchState;

    // Odczyt stanu przycisków (LOW oznacza wciśnięty)
    command.L1Pressed = (digitalRead(L1_PIN) == LOW);
    command.L2Pressed = (digitalRead(L2_PIN) == LOW);
    command.L3Pressed = (digitalRead(L3_PIN) == LOW);
    command.L4Pressed = (digitalRead(L4_PIN) == LOW);
    command.R1Pressed = (digitalRead(R1_PIN) == LOW);
    command.R2Pressed = (digitalRead(R2_PIN) == LOW);
    command.R3Pressed = (digitalRead(R3_PIN) == LOW);
    command.R4Pressed = (digitalRead(R4_PIN) == LOW);

    // Odczyt surowych wartości joysticków
    int raw_joy1_x = analogRead(JOY1_X);
    int raw_joy1_y = analogRead(JOY1_Y);
    int raw_joy2_x = analogRead(JOY2_X);
    int raw_joy2_y = analogRead(JOY2_Y);

    // Mapowanie wartości joysticków do zakresu -100 do 100
    command.joy1_x = mapJoystick(raw_joy1_x, 668, 4095, 2728, -100, 100);
    command.joy1_y = mapJoystick(raw_joy1_y, 791, 4095, 2734, -100, 100);
    command.joy2_x = mapJoystick(raw_joy2_x, 1140, 4095, 2810, -100, 100);
    command.joy2_y = mapJoystick(raw_joy2_y, 1380, 4095, 2750, -100, 100);

    // Deadzone dla joysticków (-5 do 5 ustawiamy na 0)
    if (abs(command.joy1_x) <= 20) command.joy1_x = 0;
    if (abs(command.joy1_y) <= 20) command.joy1_y = 0;
    if (abs(command.joy2_x) <= 20) command.joy2_x = 0;
    if (abs(command.joy2_y) <= 20) command.joy2_y = 0;

    // Sprawdzanie czy przyciski zostały wciśnięte i wyświetlanie w konsoli
    if (command.L1Pressed != lastCommand.L1Pressed) Serial.println(command.L1Pressed ? "L1 Wciśnięty" : "L1 Puszczony");
    if (command.L2Pressed != lastCommand.L2Pressed) Serial.println(command.L2Pressed ? "L2 Wciśnięty" : "L2 Puszczony");
    if (command.L3Pressed != lastCommand.L3Pressed) Serial.println(command.L3Pressed ? "L3 Wciśnięty" : "L3 Puszczony");
    if (command.L4Pressed != lastCommand.L4Pressed) Serial.println(command.L4Pressed ? "L4 Wciśnięty" : "L4 Puszczony");
    if (command.R1Pressed != lastCommand.R1Pressed) Serial.println(command.R1Pressed ? "R1 Wciśnięty" : "R1 Puszczony");
    if (command.R2Pressed != lastCommand.R2Pressed) Serial.println(command.R2Pressed ? "R2 Wciśnięty" : "R2 Puszczony");
    if (command.R3Pressed != lastCommand.R3Pressed) Serial.println(command.R3Pressed ? "R3 Wciśnięty" : "R3 Puszczony");
    if (command.R4Pressed != lastCommand.R4Pressed) Serial.println(command.R4Pressed ? "R4 Wciśnięty" : "R4 Puszczony");

    // Sprawdzanie czy joysticki zostały przesunięte
    if (abs(command.joy1_x - lastCommand.joy1_x) > 5) Serial.printf("Joystick 1 X: %d\n", command.joy1_x);
    if (abs(command.joy1_y - lastCommand.joy1_y) > 5) Serial.printf("Joystick 1 Y: %d\n", command.joy1_y);
    if (abs(command.joy2_x - lastCommand.joy2_x) > 5) Serial.printf("Joystick 2 X: %d\n", command.joy2_x);
    if (abs(command.joy2_y - lastCommand.joy2_y) > 5) Serial.printf("Joystick 2 Y: %d\n", command.joy2_y);

    // Wysłanie danych ESP-NOW do aktualnie wybranego odbiornika
    uint8_t* currentReceiver = useESP32Receiver ? macOdbiorcaESP32 : macOdbiorcaESP8266;
    esp_err_t result = esp_now_send(currentReceiver, (uint8_t *)&command, sizeof(command));
    
    if (result != ESP_OK) {
        Serial.println("Błąd wysyłania!");
    }

    // Kopiuj aktualny stan jako poprzedni
    lastCommand = command;

    delay(50);  // Zmniejszono opóźnienie dla lepszej responsywności
}