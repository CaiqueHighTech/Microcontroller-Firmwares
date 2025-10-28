/**
 * @file main.cpp
 * @brief Principal file for this application.
 * @version 2.0.0
 * @date 2025
 * 
 * Complete system of control of semaphore to vehicles and pedestrians
 * using FreeRTOS to manager the concurrency tasks.
 * 
 * Architecture:
 *  - Clean Architecture with separations of layers.
 *  - HAL (Hardware Abstraction Layer)
 *  - State Machine Pattern
 *  - RAII to manage resources.
 *  - Thread-safe with mutex
 * 
 * @author Semaphore system v2.0
 */

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

// Systems includes
#include "Config.h"
#include "Types.h"
#include "Hardware_Abstraction_Layer.h"
#include "Semaphore_State_Machine.h"
#include "SemaphoreTasks.h"

// System namespace
using namespace SemaphoreSystem;

// ==================== GLOBALS VARIABLES ====================

// Principals instances of system (global necessary scope to FreeRTOS)

ArduinoHardwareController* g_hardwareController = nullptr;
SemaphoreStateMachine* g_stateMachine = nullptr;
SharedContext* g_sharedContext = nullptr;
TaskManager* g_taskManager = nullptr;

// ==================== AUXILIARIES FUNCTIONS ====================

/**
 * @brief Print banner of initialization
 */
void printStartupBanner() {
    Serial.println(F("\n"));
    Serial.println(F("╔════════════════════════════════════════════╗"));
    Serial.println(F("║   TRAFFIC LIGHT CONTROL SYSTEM v2.0       ║"));
    Serial.println(F("║   With FreeRTOS & Clean Architecture      ║"));
    Serial.println(F("╚════════════════════════════════════════════╝"));
    Serial.println(F(""));
    Serial.println(F("System Configuration:"));
    Serial.print(F("  - Green Car Duration: "));
    Serial.print(TimingConfig::GREEN_CAR_DURATION / 1000);
    Serial.println(F("s"));

    Serial.print(F("  - Yellow Car Duration: "));
    Serial.print(TimingConfig::YELLOW_CAR_DURATION / 1000);
    Serial.println(F("s"));

    Serial.print(F("  - Safety Gap Duration: "));
    Serial.print(TimingConfig::SAFETY_GAP_DURATION / 1000);
    Serial.println(F("s"));

    Serial.print(F("  - Green Pedestrian Duration: "));
    Serial.print(TimingConfig::GREEN_PEDESTRIAN_DURATION / 1000);
    Serial.println(F("s"));

    Serial.print(F("  - Total Cycle Time: "));
    Serial.print(TimingConfig::TOTAL_CYCLE_TIME / 1000);
    Serial.println(F("s"));

    Serial.println(F(""));
}

/**
 * @brief Print configurations of pins
 */
void printPinConfiguration() {
    Serial.println(F("Pin Configuration:"));
    Serial.println(F("  Vehicle LEDs:"));
    Serial.print(F("    - Red: Pin "));
    Serial.println(HardwareConfig::LED_RED_CAR);
    Serial.print(F("    - Yellow: Pin "));
    Serial.println(HardwareConfig::LED_YELLOW_CAR);
    Serial.print(F("    - Green: Pin "));
    Serial.println(HardwareConfig::LED_GREEN_CAR);
    
    Serial.println(F("  Pedestrian LEDs:"));
    Serial.print(F("    - Red: Pin "));
    Serial.println(HardwareConfig::LED_RED_PEDESTRIAN);
    Serial.print(F("    - Green: Pin "));
    Serial.println(HardwareConfig::LED_GREEN_PEDESTRIAN);
    Serial.println(F(""));
}

/**
 * @brief Initialize the hardware
 * @return true if the initialization was successful
 */
bool initializeHardware() {
    Serial.println(F("[INIT] Initializing hardware..."));

    // Get the instance singleton of hardware controller
    g_hardwareController = &ArduinoHardwareController::getInstance();

    if (g_hardwareController == nullptr) {
        Serial.println(F("[ERROR] Failed to get hardware controller instance!"));
        return false;
    }

    // Initialize the hardware
    g_hardwareController->initialize();
    Serial.println(F("[OK] Hardware initialized successfully"));

    return true;
}

/**
 * @brief Initialize the states machines
 * @return true if the initialization was successful
 */
bool initializeStateMachine() {
    Serial.println(F("[INIT] Initializing state machine..."));

    if (g_hardwareController == nullptr) {
        Serial.println(F("[ERROR] Hardware controller not initialized!"));
        return false;
    }

    // Create the machine of states
    g_stateMachine = new SemaphoreStateMachine(*g_hardwareController);

    if (g_stateMachine == nullptr) {
        Serial.println(F("[ERROR] Failed to create state machine instance!"));
        return false;
    }

    // Initialize and start the machine of states
    g_stateMachine->initialize();
    g_stateMachine->begin();

    Serial.println(F("[OK] State machine initialized successfully"));

    return true;
}

/**
 * @brief Initialize the shared context
 * @return true if the initialization was successful
 */
bool initializeSharedContext() {
    Serial.println(F("[INIT] Initializing shared context..."));

    if (g_stateMachine == nullptr) {
        Serial.println(F("[ERROR] State machine not initialized!"));
        return false;
    }

    // Create the shared context
    g_sharedContext = new SharedContext(*g_stateMachine);

    if (g_sharedContext == nullptr) {
        Serial.println(F("[ERROR] Failed to create shared context!"));
        return false;
    }

    Serial.println(F("[OK] Shared context initialized successfully"));

    return true;
}

/**
 * @brief Initialize the tasks of FreeRTOS
 * @return true if the initialization was successful
 */
bool initializeTasks() {
    Serial.println(F("[INIT] Creating FreeRTOS tasks..."));

    if (g_sharedContext == nullptr) {
        Serial.println(F("[ERROR] Shared context not initialized!"));
        return false;
    }

    // Create the task manager
    g_taskManager = new TaskManager(*g_sharedContext);

    if (g_taskManager == nullptr) {
        Serial.println(F("[ERROR] Failed to create task manager!"));
        return false;
    }

    // Create the tasks
    if (!g_taskManager->createTasks()) {
        Serial.println(F("[ERROR] Failed to create FreeRTOS tasks!"));
        return false;
    }

    Serial.println(F("[OK] FreeRTOS tasks created successfully"));

    return true;
}

/**
 * @brief Function about fatal error treatment
 * In case of critical error, blink all LEDs
 */
void fatalError() {
    Serial.println(F("\n[FATAL ERROR] System halted!"));
    Serial.println(F("All LEDs will blink to indicate error state."));

    // If the hardware was initialized, blink all LEDs
    if (g_hardwareController != nullptr) {
        for (;;) {
            g_hardwareController->turnAllLedsOff();
            delay(250);

            // Turn on all LEDs
            g_hardwareController->setLedState(
                HardwareConfig::LED_RED_CAR, LedStatus::ON);
            g_hardwareController->setLedState(
                HardwareConfig::LED_YELLOW_CAR, LedStatus::ON);
            g_hardwareController->setLedState(
                HardwareConfig::LED_GREEN_CAR, LedStatus::ON);
            g_hardwareController->setLedState(
                HardwareConfig::LED_RED_PEDESTRIAN, LedStatus::ON);
            g_hardwareController->setLedState(
                HardwareConfig::LED_GREEN_PEDESTRIAN, LedStatus::ON);
            
            delay(250);
        }
    } else {
        // if neither hardware was initialized, just stop
        for (;;) {
            delay(1000);
        }
    }
}

// ==================== ARDUINO SETUP ====================

/**
 * @brief Arduino setup function
 * Execute once at the start
 */
void setup() {
    // Intialize serial comunication
    Serial.begin(SerialConfig::BAUD_RATE);

    // Await the serial be ok (util for debugging)
    while (!Serial && millis() < 3000) {
        ; // Await until 3 seconds
    }

    // Print the banner
    printStartupBanner();
    printPinConfiguration();

    // Initialization sequence
    Serial.println(F("\n========== INITIALIZATION SEQUENCE ==========\n"));

    // 1. Initialize hardware
    if (!initializeHardware()) {
        fatalError();
    }

    // 2. Initialize state machine
    if (!initializeStateMachine()) {
        fatalError();
    }

    // 3. Initialize shared context
    if (!initializeSharedContext()) {
        fatalError();
    }

    // 4. Initialize FreeRTOS tasks
    if (!initializeTasks()) {
        fatalError();
    }

    Serial.println(F("\n========== INITIALIZATION COMPLETE ==========\n"));
    Serial.println(F("[INFO] Starting FreeRTOS scheduler..."));
    Serial.println(F("[INFO] System is now running!\n"));

    // Intialize the scheduler from FreeRTOS
    vTaskStartScheduler();

    // If all was wrong, enter in fatal error
    Serial.println(F("[CRITICAL] Scheduler failed to start!"));
    fatalError();
}

// ==================== ARDUINO LOOP ====================

/**
 * @brief Arduino loop function
 * Not used because FreeRTOS is managing the tasks
 */

void loop() {
    Serial.println(F("[ERROR] Loop reached - this should never happen!"));
    delay(1000);

    // Try restart the system
    fatalError();
}

// ==================== HOOKS DO FREERTOS ====================

/**
 * @brief Hook called when there is a stack overflow in a task
 * Util for debugging
 */
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    Serial.print(F("[CRITICAL] Stack overflow in task: "));
    Serial.println(pcTaskName);

    // Try recover turn off everything
    if (g_hardwareController != nullptr) {
        g_hardwareController->turnAllLedsOff();
    }

    // Entry on fatal error
    fatalError();
}

/**
 * @brief Hook called when malloc fails (security)
 */
extern "C" void vApplicationMallocFailedHook(void) {
    Serial.println(F("[CRITICAL] Memory allocation failed!"));
    Serial.print(F("Free heap: "));
    Serial.println(xPortGetFreeHeapSize());

    fatalError();
}