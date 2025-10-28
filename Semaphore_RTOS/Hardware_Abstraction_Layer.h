/**
 * @file Hardware_Abstraction_Layer.h
 * @brief Hardware Abstraction Layer (HAL) for semaphore system
 * @version 2.0.0
 * 
 * Implement the HAL pattern to isolate the business code
 * of the semaphore system from the hardware specifics.
 */

#ifndef HARDWARE_ABSTRACTION_LAYER_H
#define HARDWARE_ABSTRACTION_LAYER_H

#include <Arduino.h>
#include "Types.h"
#include "Config.h"

namespace SemaphoreSystem {

/**
 * @brief Interface to control the hardware components
 * Allow create mocks for unit testing
 */
class IHardwareController {
public:
    virtual ~IHardwareController() = default;

    virtual void initialize() = 0;
    virtual void setLedState(uint8_t pin, LedStatus status) = 0;
    virtual void applyConfiguration(const LedConfiguration& config) = 0;
    virtual void turnAllLedsOff() = 0;
};

/**
 * @brief Concrete implementation of hardware control to Arduino
 * 
 * This class encapsulates all the physical pins interations,
 * following the Single Responsibility Principle.
 */
class ArduinoHardwareController : public IHardwareController {
private:
    // Singleton pattern - only one instance
    static ArduinoHardwareController* instance_;

    // Private constructor (Singleton)
    ArduinoHardwareController() = default;
    
    // Delete copy constructor and assignment operator
    ArduinoHardwareController(const ArduinoHardwareController&) = delete;
    ArduinoHardwareController& operator=(const ArduinoHardwareController&) = delete;

    /**
     * @brief Configurate the pin as output of safety form
     */
    void configurePinAsOutput(uint8_t pin) const {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW); // Safety initial state
    }
public:
    /**
     * @brief Get the singleton instance (Singleton)
     */
    static ArduinoHardwareController& getInstance() {
        if (instance_ == nullptr) {
            instance_ = new ArduinoHardwareController();
        }
        return *instance_;
    }

    /**
     * @brief Initialize the hardware (pins configuration)
     * Set all pins as OUTPUT and initial state OFF
     */
    void initialize() override {
        using namespace HardwareConfig;

        // Configure car LEDs
        configurePinAsOutput(LED_RED_CAR);
        configurePinAsOutput(LED_YELLOW_CAR);
        configurePinAsOutput(LED_GREEN_CAR);

        // Configure pedestrian LEDs
        configurePinAsOutput(LED_RED_PEDESTRIAN);
        configurePinAsOutput(LED_GREEN_PEDESTRIAN);

        // Guarantee all LEDs are OFF at start
        turnAllLedsOff();
    }

    /**
     * @brief Set the state of a specific LED
     * @param pin The pin number of the LED
     * @param status The desired state (ON/OFF)
     */
    void setLedState(uint8_t pin, LedStatus status) override {
        digitalWrite(pin, toDigitalValue(status));
    }

    /**
     * @brief Apply a full LED configuration
     * @param config The LedConfiguration to apply
     * 
     * Use atomic-like operation to minimize intermediates states
     */
    void applyConfiguration(const LedConfiguration& config) override {
        using namespace HardwareConfig;

        // Firstly turn off all LEDs to avoid glitches
        turnAllLedsOff();

        // Apply the new configuration
        setLedState(LED_RED_CAR, config.redCar);
        setLedState(LED_YELLOW_CAR, config.yellowCar);
        setLedState(LED_GREEN_CAR, config.greenCar);
        setLedState(LED_RED_PEDESTRIAN, config.redPedestrian);
        setLedState(LED_GREEN_PEDESTRIAN, config.greenPedestrian);
    }

    /**
     * @brief Turn off all LEDs
     * Ensure a known safe state
     */
    void turnAllLedsOff() override {
        using namespace HardwareConfig;

        digitalWrite(LED_RED_CAR, LOW);
        digitalWrite(LED_YELLOW_CAR, LOW);
        digitalWrite(LED_GREEN_CAR, LOW);
        digitalWrite(LED_RED_PEDESTRIAN, LOW);
        digitalWrite(LED_GREEN_PEDESTRIAN, LOW);
    }

    /**
     * @brief Destructor
     */
    ~ArduinoHardwareController() override = default;
};

// Initialize static pointer
ArduinoHardwareController* ArduinoHardwareController::instance_ = nullptr;

} // namespace SemaphoreSystem

#endif // HARDWARE_ABSTRACTION_LAYER_H