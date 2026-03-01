#include <Wire.h>
#include <DHT.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Define hardware type for MAX7219 (assuming FC-16 module, generic matrix)
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define MAX_DEVICES 4 // Adjust this based on your matrix size (e.g., 4 for 32x8 display)

// MAX7219 pins
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 10

// DHT22 pin
#define DHT_PIN 2
#define DHT_TYPE DHT22

// Timezone offset for Brasília (UTC-3)
#define TZ_OFFSET_SECONDS (-3 * 3600L)

// ─── AJUSTE AQUI: hora inicial de Brasília ───────
#define START_HOUR   18
#define START_MINUTE  0
#define START_SECOND  0
#define START_DAY     1
#define START_MONTH   3   // 3 = Março, 4 = Abril, etc
#define START_YEAR  2026
#define START_DOW     0   // 0=Dom,1=Seg,2=Ter,3=Qua,4=Qui,5=Sex,6=Sab
// ─────────────────────────────────────────────────

// DHT
DHT dht(DHT_PIN, DHT_TYPE);

// Parola object for scrolling text
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Arrays for day and month names in Portuguese
const char* daysOfWeek[] = {
    "Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"
};
const char* months[] = {
    "Jan", "Fev", "Mar", "Abr", "Mai", "Jun",
    "Jul", "Ago", "Set", "Out", "Nov", "Dez"
};

// Days of month (no leap year)
const uint8_t daysInMonth[] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

// Message buffer
char message[120];
unsigned long lastMillis = 0;

// Clock state
int8_t _sec, _min, _hour, _dow;
uint8_t _day, _month;
uint16_t _year;

void tickClock() {
    unsigned long now = millis();
    unsigned long diff = (now - lastMillis) / 1000UL;
    if (diff == 0) return;
    lastMillis += diff * 1000UL;

    _sec += diff;
    while (_sec >= 60) { _sec -= 60; _min++; }
    while (_min >= 60) { _min -= 60; _hour++; }
    while (_hour >= 24) {
        _hour -= 24;
        _day++;
        _dow = (_dow + 1) % 7;
        uint8_t dim = daysInMonth[_month - 1];
        if (_day > dim) {_day = 1; _month++; }
        if (_month > 12) {_month = 1; _year++; }
    }
}

int correctTimeZone(int UTChour) {
    int h = UTChour + TZ_OFFSET_SECONDS;
    if (h < 0) h += 24;
    if (h >= 24) h -= 24;
    return h;
}

void setup() {
    Serial.begin(9600); // For debugging

    // Inicializa estado do relógio
    _sec   = START_SECOND;
    _min   = START_MINUTE;
    _hour  = START_HOUR;
    _day   = START_DAY;
    _month = START_MONTH;
    _year  = START_YEAR;
    _dow   = START_DOW;
    lastMillis = millis();

    // Initialize DHT
    dht.begin();

    // Initialize Parola
    P.begin();
    P.setIntensity(8); // Brightness 0-15
    P.displayClear();
    strcpy(message, "Iniciando..."); // Initial message
    P.displayText(message, PA_LEFT, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT); // Optional, can be LEFT, RIGHT, CENTER
}
  
void loop() {
    tickClock(); 
    
    // Read DHT22
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Check if readings failed
    if (isnan(humidity) || isnan(temperature)){
        Serial.println("Falha em ler do sensor DHT!");
        delay(2000);
        return;
    }

    // Format strings
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d:%02d", _hour, _min, _sec);

    char dateStr[20];
    sprintf(dateStr, "%02d %s %04d", _day, months[_month - 1], _year);

    const char* dayStr = daysOfWeek[_dow];

    // Transfroming temperature and humidity in strings
    char tempBuff[8], humBuff[8];
    dtostrf(temperature, 4, 1, tempBuff);
    dtostrf(humidity, 4, 1, humBuff);
    
    char tempStr[16];
    sprintf(tempStr, "Temp: %sC", tempBuff);

    char humStr[16];
    sprintf(humStr, "Umid: %s%%", humBuff);

    sprintf(message, " %s  |  %s  |  %s  |  %s  |  %s ",
            timeStr, dateStr, dayStr, tempStr, humStr); 

    Serial.println(message); // For debugging
    
    // Display scrolling text
    if (P.displayAnimate()) {
        P.displayText(message, PA_LEFT, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT); // Speed 50, scroll left
    }

    delay(100); //Small delay to avoid overwhelming the loop
}


