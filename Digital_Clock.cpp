#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Define hardware type for MAX7219 (assuming FC-16 module, generic matrix)
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4 // Adjust this based on your matrix size (e.g., 4 for 32x8 display)

// MAX7219 pins
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 12

// DHT22 pin
#define DHT_PIN 2
#define DHT_TYPE DHT22

// RTC
RTC_DS1307 rtc;

// DHT
DHT dht(DHT_PIN, DHT_TYPE);

// Parola object for scrolling text
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Timezone offset for Brasília (UTC-3)
const int TZ_OFFSET = -3; // Hours

// Arrays for day and month names in Portuguese
const char* daysOfWeek[] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"};
const char* months[] = {"Janeiro", "Fevereiro", "Março", "Abril", "Maio", "Junho", "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro"};


void setup() {
    Serial.begin(3600); // For debugging

    // Initialize DHT
    dht.begin();

    // Initialize RTC
    if (!rtc.begin()){
        Serial.println("Não pode encontrar RTC!");
        while (1);
    }

    if (!rtc.isrunning()){
        Serial.println("RTC não está rodando!");
        // Set the RTC to the date & time this sketch was compiled 
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // Initialize Parola
    P.begin();
    P.setIntensity(8); // Brightness 0-15
    P.displayClear();
    P.setTextAlignment(PA_CENTER); // Optional, can be LEFT, RIGHT, CENTER

void loop() {
    // Read DHT22
    float humidty = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Check if readings failed
    if (isnan(hmidity) || isnan(temperature)){
        Serial.println("Falha em ler do sensor DHT!");
        delay(2000);
        return;
    }

    // Get current time from RTC, adjust for timezone if needed 
    DateTime now = rtc.now();
    // If RTC is in UTC, add: now = now + TimeSpan(TZ_OFFSET * 3600);

    // Format strings
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    char dateStr[20];
    sprintf(dateStr, "%02d %s %04d", now.day(), months[now.month() - 1], now.year());

    const char* dayStr = daysOfWeek[now.dayOfTheWeek()];
    
    char tempStr[20];
    sprintf(tempStr, "Temp: %.1f C", temperature);

    char humStr[20];
    sprintf(humStr, "Umid: %.1f %%", humidity);

    // Combine all info into one scrolling message
    char message[100];
    sprintf(message, "%s  |  %s  |  %s  |  %s  |  %s  |  %s",
            timeStr, dateStr, dayStr, tempStr, humStr, " "); // Add space for separation
    
    // Display scrolling text
    if (P.displayAnimate()) {
        P.displayText(message, PA_CENTER, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT); // Speed 50, scroll left
    }

    delay(100); //Small delay to avoid overwhelming the loop
}


