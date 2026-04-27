#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>
#include <avr/sleep.h>
#include <avr/power.h>

#pragma GCC optimize ("Os") // Otimização para código compacto e eficiente

// ─────────────────────────────────────────────────────────────
//  CONFIGURAÇÃO DE HARDWARE (Wokwi/Real)
// ─────────────────────────────────────────────────────────────
constexpr uint8_t TFT_CS = 10;
constexpr uint8_t TFT_DC = 9;
constexpr uint16_t SCREEN_W = 320;
constexpr uint16_t SCREEN_H = 240;
constexpr uint16_t BOXSIZE = 40; // Tamanho de cada dor na paleta
constexpr uint16_t PENRADIUS = 3; // Espessura do pincel

// Paleta de cores (8 blocos iguais)
constexpr uint16_t PALETTE[] = {
    ILI9341_RED, ILI9341_YELLOW, ILI9341_GREEN,
    ILI9341_CYAN, ILI9341_BLUE, ILI9341_MAGENTA
};
constexpr uint8_t NUM_COLORS = sizeof(PALETTE) / sizeof(PALETTE[0]);

// ─────────────────────────────────────────────────────────────
//  ESTADO DA APLICAÇÃO (Zero-alloc, cache-friendly)
// ─────────────────────────────────────────────────────────────
struct AppState {
    uint16_t currentColor = ILI9341_RED;
    uint32_t lastTouchMs = 0;
    uint8_t selectedIdx = 0;
} appState;

// ─────────────────────────────────────────────────────────────
//  EVENT DISPATCHER (Pub-Sub circular, lock-free)
// ─────────────────────────────────────────────────────────────
enum class EventType : uint8_t { SYSTEM_INIT, TOUCH_EVENT, IDLE_TIMEOUT };
using EventCallback = void (*)(const void*);

class EventDispatcher {
public:
    static constexpr uint8_t MAX_SUBS = 5;
    static constexpr uint8_t QUEUE_SIZE = 10;

    void subscribe(EventType type, EventCallback cb) noexcept {
        for (auto& s : subs_) {
            if (!s.cb) { s.type = type; s.cb = cb; return; }
        }
    }

    void publish(EventType type, const void* payload = nullptr) noexcept {
        uint8_t next = (tail_ + 1) % QUEUE_SIZE;
        if (next == head_) return; // Fila cheia: descarta evento antigo
        queue_[tail_] = {type, payload};
        tail_ = next;
    }

    void process() noexcept {
        while (head_ != tail_) {
            const auto& evt = queue_[head_];
            head_ = (head_ + 1) % QUEUE_SIZE;
            for (const auto& s: subs_) {
                if (s.type == evt.type && s.cb) s.cb(evt.payload);
            }
        }
    }

private:
    struct Subscriber { EventType type{EventType::SYSTEM_INIT}; EventCallback cb{nullptr}; };
    struct Event { EventType type; const void* payload; };
    Subscriber subs_[MAX_SUBS]{};
    Event queue_[QUEUE_SIZE];
    uint8_t head_{0}, tail_{0};
} dispatcher;

// ─────────────────────────────────────────────────────────────
//  GERENCIAMENTO DE CLOCK & POWER (RAII-style)
// ─────────────────────────────────────────────────────────────
class PowerManager {
public:
    static void init() noexcept {
        // Clock full (16MHz). Prescaler padrão do bootloader já é div_1.
        clock_prescale_set(clock_div_1);
        set_sleep_mode(SLEEP_MODE_IDLE); // CPU descansa, mas SPI/I2C/INTs ativos
    }
    
    static void sleepIfIdle(uint32_t lastActivity, uint32_t now) noexcept {
       if (now - lastActivity > 2000) {
          sleep_enable(); sei(); sleep_cpu(); sleep_disable();  
       }
    }
};

// ─────────────────────────────────────────────────────────────
//  PERIFÉRICOS (Alocação estática para evitar fragmentação de RAM)
// ─────────────────────────────────────────────────────────────
Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
Adafruit_FT6206 touch; // I2C, não usa CS

// ─────────────────────────────────────────────────────────────
//  HANDLERS (Stateless, noexcept, C++ moderno)
// ─────────────────────────────────────────────────────────────
void onInit([[maybe_unused]] const void*) noexcept {
    tft.begin();
    tft.setRotation(1); // 320x240 landscape
    tft.fillScreen(ILI9341_WHITE);
    
    // Desenha paleta
    for (uint8_t i = 0; i < NUM_COLORS; ++i){
        tft.fillRect(i * BOXSIZE, 0, BOXSIZE, BOXSIZE, PALETTE[i]);
    }
    // Destaque inicial (vermelho)
    tft.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);

    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(10, BOXSIZE + 10);
    tft.println("Toque para desenhar");
}

void onTouchEvent(const void* data) noexcept {
   const auto& pt = static_cast<const TS_Point*>(data);

   // ── MAPEAMENTO CORRETO (igual ao código funcional) ──
    // FT6206 retorna coordenadas com eixos invertidos em relação à tela
   int16_t x = map(pt->x, 0, 240, 240, 0); // Inverte X
   int16_t y = map(pt->y, 0, 320, 320, 0); // Inverte Y
   x = constrain(x, 0, SCREEN_W - 1);
   y = constrain(y, 0, SCREEN_H - 1);

   if (y < BOXSIZE) {
        // Seleção de cor
        uint8_t idx = x / BOXSIZE;
        if (idx < NUM_COLORS && idx != appState.selectedIdx) {
            // Remove destaque da cor anterior
            tft.fillRect(appState.selectedIdx * BOXSIZE, 0, BOXSIZE, BOXSIZE, PALETTE[appState.selectedIdx]);
            // Atualiza estado
            appState.selectedIdx = idx;
            appState.currentColor = PALETTE[idx];
            // Aplica novo destaque
            tft.drawRect(idx * BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE); // Nova seleção
        }
    } 
    else if ((y - PENRADIUS) > BOXSIZE && (y + PENRADIUS) < SCREEN_H) {
       // ── ÁREA DE DESENHO ──
       // Desenha círculo simples 
       tft.fillCircle(x, y, PENRADIUS, appState.currentColor);
   }
}

void onIdle([[maybe_unused]] const void*) noexcept {
    PowerManager::sleepIfIdle(appState.lastTouchMs, millis());
}

// ─────────────────────────────────────────────────────────────
//  SETUP & LOOP
// ─────────────────────────────────────────────────────────────
void setup() {
    PowerManager::init();

    // I2C para touch (SDA=A4, SCL=A5)
    Wire.begin();
    Wire.setClock(400000UL);  // Fast-mode para resposta mais rápida
    
    // SPI para display (MOSI=11, MISO=12, SCK=13)
    SPI.begin();
    SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));

    // Inicialização dos drivers
    tft.begin();
    if (!touch.begin(10)) while (true) {  /* Falha crítica */ }

    // Registro de subscribers
    dispatcher.subscribe(EventType::SYSTEM_INIT, onInit);
    dispatcher.subscribe(EventType::TOUCH_EVENT, onTouchEvent);
    dispatcher.subscribe(EventType::IDLE_TIMEOUT, onIdle);

    // Dispara evento de boot
    dispatcher.publish(EventType::SYSTEM_INIT);
}

void loop() {
    // 1. Processa fila de eventos (zero delay, zero bloqueio)
    dispatcher.process();

    // 2. Polling não-bloqueante de touch (limitado a 30Hz para evitar flood)
    static uint32_t lastTouch = 0;
    if (millis() - lastTouch >= 16) { // ~60Hz para desenho fluido 
        lastTouch = millis();
        if (touch.touched()) {
            TS_Point p = touch.getPoint();
            dispatcher.publish(EventType::TOUCH_EVENT, &p);
            appState.lastTouchMs = millis();
        }
    }

    // 3. Power Management: entra em idle quando não há eventos pendentes
    dispatcher.publish(EventType::IDLE_TIMEOUT);
}