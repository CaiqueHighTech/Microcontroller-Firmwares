#include <Wire.h> // Biblioteca para comunicar-se com circuitos inter-integrados
#include <LiquidCrystal_I2C.h> // Biblioteca para mexer no LCD
#define LED 13 // Pino do LED
#define BUZZER 8 // Pino do buzzer

// Configuração do display LCD I2C
// Endereço I2C: 0x27, 16 colunas, 2 linhas 
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pino analógico onde o LM35 está conectado
const int pinoLM35 = A0;

// Variável para controlar se o buzzer deve estar ativo
bool buzzerAtivo = false;

void setup(){

    // Define o pino do LED e do BUZZER
    pinMode(LED, OUTPUT);
    pinMode(BUZZER, OUTPUT);

    // Inicializa comunicação serial para debug
    Serial.begin(9600);

    // Inicializa o display LCD
    lcd.init();
    lcd.backlight(); // Liga a luz de fundo do LCD

    // Exibe mensagem inicial no LCD
    lcd.setCursor(0, 0);
    lcd.print("Termometro LM35");
    lcd.setCursor(0, 1);
    lcd.print("Iniciando...");
    
    delay(2000); // Aguarda 2 segundos para mostrar a mensagem inicial
    lcd.clear(); // Limpa o display
}

void loop(){
    // Lê o valor do LM35
    int valorLM35 = analogRead(pinoLM35);
    
    // Converte o valor lido para temperatura em Celsius
    float temperaturaC = (((valorLM35 * 5.0) / 1024.0) - 0.5) * 100.0; // Converte para Celsius (TMP35 fornece 10 mV/°C)

    // Exibe a temperatura no monitor serial
    Serial.print("Valor analógico no LM35: ");
    Serial.print(valorLM35);
    Serial.print("Temperatura: ");
    Serial.print(temperaturaC, 1);
    Serial.println(" °C");

    // Atualiza o display LCD com a temperatura
    lcd.setCursor(0, 1);
    lcd.print(temperaturaC, 1);
    lcd.print(" C   "); // Espaços para limpar caracteres antigos

    if (temperaturaC >= 40.0) {
        // Se a temperatura for maior que 30°C, exibe alerta
        lcd.setCursor(0, 0);
        lcd.print("Alerta: Alta T! ");
        Serial.println("Alerta: Alta T!");

        // Pisca o LED
        digitalWrite(LED, HIGH);

        // Liga o buzzer continuamente
        if (!buzzerAtivo){
            tone(BUZZER, 1000); // Frequência de 1000Hz (1kHz)
            buzzerAtivo = true;
        }
    } else {
        // Limpa o alerta se a temperatura estiver normal
        lcd.setCursor(0, 0);
        lcd.print("Temperatura OK   ");
        Serial.println("Temperatura OK");

        // Desativa o LED
        digitalWrite(LED, LOW);
        
        // Desliga o buzzer
        if (buzzerAtivo){
            noTone(BUZZER);
            buzzerAtivo = false;
        }
    }

    delay(1000); // Aguarda 1 segundo antes da próxima leitura
}
