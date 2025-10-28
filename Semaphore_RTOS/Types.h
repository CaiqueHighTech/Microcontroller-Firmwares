/**
 * @file Types.h
 * @brief Types definitions and system enumerations
 * @version 2.0.0
 * 
 * Define safe types and strongly typed enumerations(enum class)
 * to represent the semaphore states and configurations
 */

#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

namespace SemaphoreSystem {

/**
 * @brief Possible states of a semaphore
 * Use enum class to type safety (avoid implicit conversions)
 */
enum class SemaphoreState : uint8_t {
    GREEN_CAR = 0, // Green to cars, red to pedestrians        
    YELLOW_CAR = 1, // Yellow to cars, red to pedestrians     
    SAFETY_GAP_BEFORE = 2, // All red (safety gap before changing to green for pedestrians)       
    GREEN_PEDESTRIAN = 3, // Green to pedestrians, red to cars
    SAFETY_GAP_AFTER = 4, // All red (safety gap after changing to green for pedestrians)

    // Assistants values
    FIRST_STATE = GREEN_CAR,
    LAST_STATE = SAFETY_GAP_AFTER,
    TOTAL_STATES = 5
}; 

/**
 * @brief LEDs individuals states
 */
enum class LedStatus : bool {
    OFF = false,
    ON = true
};

/**
 * @brief Semaphore type
 */
enum class SemaphoreType : uint8_t {
    VEHICLE,
    PEDESTRIAN
};

/**
 * @brief Structure to represent the LEDs configuration to a state
 * Use safety and explicit types
 */
struct LedConfiguration {
    LedStatus redCar;
    LedStatus yellowCar;
    LedStatus greenCar;
    LedStatus redPedestrian;
    LedStatus greenPedestrian;

    // Pattern constructor - all LEDs OFF
    constexpr LedConfiguration()
        : redCar(LedStatus::OFF),
          yellowCar(LedStatus::OFF),
          greenCar(LedStatus::OFF),
          redPedestrian(LedStatus::OFF),
          greenPedestrian(LedStatus::OFF) {}

    // Parameterized constructor
    constexpr LedConfiguration(
        LedStatus rc, LedStatus yc, LedStatus gc,
        LedStatus rp, LedStatus gp
    ) : redCar(rc), yellowCar(yc), greenCar(gc),
        redPedestrian(rp), greenPedestrian(gp) {}
};

/**
 * @brief Structure to encapsulate the state semaphore informations
 */
struct StateInfo {
    SemaphoreState state;
    uint32_t duration; // Duration in milliseconds
    LedConfiguration ledConfig;
    const char* description; // Description to debug/logging

    constexpr StateInfo (
        SemaphoreState s,
        uint32_t d,
        LedConfiguration lc,
        const char* desc
    ) : state(s), duration(d), ledConfig(lc), description(desc) {}
};

/**
 * @brief Operator to advance to the next state (pattern-like)
 * Implement the states cycle transition of safe form
 */
inline SemaphoreState& operator++(SemaphoreState& state) {
    if (state == SemaphoreState::LAST_STATE) {
        state = SemaphoreState::FIRST_STATE;
    } else {
        state = static_cast<SemaphoreState>(static_cast<uint8_t>(state) + 1);
    }
    return state;
}

/**
 * @brief Convert enum to numeric index (helper function)
 */
constexpr uint8_t toIndex(SemaphoreState state) {
    return static_cast<uint8_t>(state);
}

/**
 * @brief Convert LedStatus to boolean value to use with digitalWrite
 */
constexpr uint8_t toDigitalValue(LedStatus status) {
    return status == LedStatus::ON ? HIGH : LOW;
}

} // namespace SemaphoreSystem

#endif // TYPES_H