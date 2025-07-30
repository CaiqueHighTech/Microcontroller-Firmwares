/**
 * Sistema de Controle de Semáforo - Arduino
 * 
 * Este código implementa um sistema de semáforo para veículos e pedestres
 * com sincronização adequada e diferença de 5 segundos entre as transições
 * conforme especificado.
 * 
 * Sequência de funcionamento:
 * 1. Verde Carro (20s) + Vermelho Pedestre
 * 2. Amarelo Carro (3s) + Vermelho Pedestre  
 * 3. Vermelho Carro (5s) + Vermelho Pedestre (diferença de 5s)
 * 4. Vermelho Carro + Verde Pedestre (20s)
 * 5. Vermelho Carro (5s) + Vermelho Pedestre (diferença de 5s)
 * 
 * Ciclo total: 53 segundos
 */

#include <U8x8lib.h>

// Definição dos pinos dos LEDs 

const int ledRedCar = 13;
const int ledYellowCar = 12;
const int ledGreenCar = 11;
const int ledGreenPedestrian = 10;
const int ledRedPedestrian = 9;

// Variáveis de controle de tempo
unsigned long previousMillis = 0; // Armazena o último tempo registrado
unsigned long currentMillis = 0; // Tempo atual
int currentState = 0; // Estado atual do semáforo (0-4)

// Definição dos tempos para cada estado (em milissegundos)
const unsigned long STATE_DURATIONS[] = {
  20000, // Estado 0: Verde carro (20s)
  3000,  // Estado 1: Amarelo carro (3s)
  5000,  // Estado 2: Vermelho carro + Vermelho pedestre (5s - diferença)
  20000, // Estado 3: Verde pedestre (20s)
  5000   // Estado 4: Vermelho carro + Vermelho pedestre (5s - diferença)
};

void setup() {
  // Configuração dos pinos como saída
  pinMode(ledRedCar, OUTPUT);
  pinMode(ledYellowCar, OUTPUT);
  pinMode(ledGreenCar, OUTPUT);
  pinMode(ledGreenPedestrian, OUTPUT);
  pinMode(ledRedPedestrian, OUTPUT);

  // Inicialização do sistema
  previousMillis = millis();
  currentState = 0;

  // Inicialização serial para debug (opcional)
  Serial.begin(9600);
  Serial.println("Sistema de Semáforo Iniciado");
}

void loop(){
 currentMillis = millis();

 // Verifica se é hora de mudar de estado
 if (currentMillis - previousMillis >= STATE_DURATIONS[currentState]){
  // Avança para o próximo estado
  currentState = (currentState + 1) % 5; // Cicla entre os estados de 0 a 4
  previousMillis = currentMillis; // Atualiza o tempo do último estado

  // Debug - mostra mudança de estado
  Serial.print("Mudando para o estado: ");
  Serial.println(currentState);
 }
 // Controla os LEDs baseado no estado atual
 controlSemaphore(currentState);
}

/**
 * Função que controla os LEDs baseado no estado atual
 * @param state - Estado atual do semáforo (0-4)
 */

void controlSemaphore(int state){

  // Desliga todos os LEDs antes de definir o novo estado
  turnAllLEDSOff();

  // Liga os LEDs de acordo com o estado atual
  switch (state) {
      case 0: // Verde para carros, vermelho para pedestres
        digitalWrite(ledGreenCar, HIGH);
        digitalWrite(ledRedPedestrian, HIGH);
        break;

      case 1: // Amarelo para carros, vermelho para pedestres
        digitalWrite(ledYellowCar, HIGH);;
        digitalWrite(ledRedPedestrian, HIGH);
        break;
        
      case 2: // Vermelho para carros, vermelho para pedestres (diferença de 5s)
        digitalWrite(ledRedCar, HIGH);
        digitalWrite(ledRedPedestrian, HIGH);
        break;
        
      case 3: // Vermelho para carros, verde para pedestres
        digitalWrite(ledRedCar, HIGH);
        digitalWrite(ledGreenPedestrian, HIGH);
        break;
        
      case 4: // Vermelho para carros, vermelho para pedestres (diferença de 5s)
        digitalWrite(ledRedCar, HIGH);
        digitalWrite(ledRedPedestrian, HIGH);
        break;
      default:
        // Estado inválido - mantém todos os LEDs desligados
        break;
    }
}

void turnAllLEDSOff() {
  digitalWrite(ledRedCar, LOW);
  digitalWrite(ledYellowCar, LOW);
  digitalWrite(ledGreenCar, LOW);
  digitalWrite(ledGreenPedestrian, LOW);
  digitalWrite(ledRedPedestrian, LOW);
}

/**
 * ESTADOS DO SEMÁFORO:
 * 
 * Estado 0 (20s): Verde Carro + Vermelho Pedestre
 *   - Veículos podem passar
 *   - Pedestres devem aguardar
 * 
 * Estado 1 (3s): Amarelo Carro + Vermelho Pedestre  
 *   - Veículos devem se preparar para parar
 *   - Pedestres ainda aguardam
 * 
 * Estado 2 (5s): Vermelho Carro + Vermelho Pedestre
 *   - Diferença de segurança de 5s
 *   - Ninguém pode passar
 * 
 * Estado 3 (20s): Vermelho Carro + Verde Pedestre
 *   - Veículos devem parar
 *   - Pedestres podem atravessar
 * 
 * Estado 4 (5s): Vermelho Carro + Vermelho Pedestre
 *   - Diferença de segurança de 5s antes de reiniciar
 *   - Ninguém pode passar
 * 
 * CICLO TOTAL: 53 segundos (20+3+5+20+5)
 */
