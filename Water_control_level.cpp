/*
 * CONTROLE DE NÍVEL DE ÁGUA EM RESERVATÓRIO
 * Arduino Uno Rev3
 * 
 * Componentes:
 * - LCD 16x2 (RS:2, E:3, D4:4, D5:5, D6:6, D7:7)
 * - Sensor Ultrassônico HC-SR04 (TRIG:8, ECHO:9)
 * - Pushbutton (porta 10)
 * - Slide Switch (porta 11)
 * - LED (porta 12)
 */

#include <LiquidCrystal.h>

// Configuração dos pinos do LCD
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

// Definição dos pinos
const int trigPin = 8;
const int echoPin = 9;
const int buttonPin = 10;
const int switchPin = 11;
const int ledPin = 12;

// Configurações do reservatório (em cm)
const float alturaReservatorio = 100.0; // Altura total do reservatório
const float nivelMinimo = 20.0; // Nível mínimo (20%)
const float nivelCritico = 10.0; // Nível crítico (10%)

// Variáveis de controle
float distancia = 0;
float nivelAgua = 0;
float percentualAgua = 0;
bool sistemaLigado = false;
bool buttonState = false;
bool lastButtonState = false;
bool modoDetalhado = false;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 500; // Atualiza a cada 500ms

void setup() {
    // Inicializa o LCD
    lcd.begin(16, 2);

    // Configuração dos pinos
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(switchPin, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);

    // Mensagem inicial
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sistema de");
    lcd.setCursor(0, 1);
    lcd.print("Nivel de Agua");
    delay(2000);

    Serial.begin(9600);
}

void loop() {
    // Verifica o estado do slide switch
    sistemaLigado = !digitalRead(switchPin); // Invertido por usar INPUT_PULLUP

    // Verifica o botão (para alternar modo de visualização)
    buttonState = !digitalRead(buttonPin);
    if (buttonState && !lastButtonState){
        modoDetalhado = !modoDetalhado;
        delay(200); // Debounce
    }
    lastButtonState = buttonState;

    // Se o sistema estiver desligado
    if (!sistemaLigado){
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Sistema");
        lcd.setCursor(0, 1);
        lcd.print("DESLIGADO");
        digitalWrite(ledPin, LOW);
        delay(500);
        return;
    }

    // Atualiza leituras a cada intervalo definido
    if (millis() - lastUpdate >= updateInterval){
        lastUpdate = millis();

        // Mede a distância com o sensor ultrassônico
        distancia = medirDistancia();

        // Calcula o nível da água
        nivelAgua = alturaReservatorio - distancia;
        if (nivelAgua < 0) nivelAgua = 0;
        if (nivelAgua > alturaReservatorio) nivelAgua = alturaReservatorio;

        // Calcula o percentual
        percentualAgua = (nivelAgua / alturaReservatorio) * 100.0;

        // Controla o LED de alerta
        controleLED();

        // Atualiza o display
        atualizarDisplay();

        // Debug via Serial
        Serial.print("Distancia: ");
        Serial.print(distancia);
        Serial.print(" cm | Nivel: ");
        Serial.print(nivelAgua);
        Serial.print(" cm | Percentual: ");
        Serial.print(percentualAgua);
        Serial.println(" %");
    }
}

float medirDistancia() {
    // Limpa o trigger
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    // Envia o pulso de 10us
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Lê o tempo de retorno do pulso
    long duracao = pulseIn(echoPin, HIGH, 30000); // Timeout de 30ms

    // Calcula a distância
    float dist = duracao * 0.0343 / 2.0;

    // Limita valores inválidos
    if (dist > 400 || dist < 2){
        return distancia; // Retorna a última leitura válida
    }

    return dist;
}

void controleLED() {
    // LED piscando rápido se nível crítico
    if (percentualAgua <= nivelCritico){
        digitalWrite(ledPin, (millis() / 500) % 2); // Pisca rápido (200ms)
    }

    // LED piscando lento se nível baixo
    else if (percentualAgua <= nivelMinimo){
        digitalWrite(ledPin, (millis() / 200) % 2); // Piscando lento (500ms)
    }

    // LED se apagando se nível OK
    else{
        digitalWrite(ledPin, LOW);
    }
}

void atualizarDisplay() {
    lcd.clear();

    if (!modoDetalhado){
        // Modo simples - mostra percentual e status
        lcd.setCursor(0, 0);
        lcd.print("Nível: ");
        lcd.print(percentualAgua, 1);
        lcd.print("%");

        lcd.setCursor(0, 1);
        if (percentualAgua <= nivelCritico){
            lcd.print("CRITICO!");
        }
        else if (percentualAgua <= nivelMinimo){
            lcd.print("BAIXO!");
        }
        else if (percentualAgua >= 90){
            lcd.print("CHEIO!");
        }
        else{
            lcd.print("NIVEL NORMAL");
        }

        // Barra de nível visual
        int barras = map(percentualAgua, 0, 100, 0, 6);
        lcd.setCursor(10, 1);
        for (int i = 0; i < barras; i++){
            lcd.write(255); // Caractere sólido
        } 
    }
    else{
        // Modo detalhado - mostra medidas
        lcd.setCursor(0, 0);
        lcd.print("Dist:");
        lcd.print(distancia, 1);
        lcd.print("cm");
        lcd.setCursor(0, 1);
        lcd.print("Agua:");
        lcd.print(nivelAgua, 1);
        lcd.print("cm");
    }
}