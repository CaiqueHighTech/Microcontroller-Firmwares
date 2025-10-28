/**
 * @file Semaphgore_State_Machine.h
 * @brief State machine for semaphore control
 * @version 2.0.0
 * 
 * Implement the state machine logic to manage the transitions
 * of states of semaphore of safe form and previsible
 */

#ifndef SEMAPHORE_STATE_MACHINE_H
#define SEMAPHORE_STATE_MACHINE_H

#include "Types.h"
#include <Arduino.h>
#include "Config.h"
#include "Hardware_Abstraction_Layer.h"

namespace SemaphoreSystem {

/**
 * @brief Semaphore state table (lookup table)
 * Constante configuration in program memory (PROGMEM)
 * to economize RAM
 */
class StateTable{
private:
    static constexpr StateInfo stateTable_[5] = {
        // 0 State: GREEN_CAR
        StateInfo(
            SemaphoreState::GREEN_CAR,
            TimingConfig::GREEN_CAR_DURATION,
            LedConfiguration(
                LedStatus::OFF,    // redCar
                LedStatus::OFF,    // yellowCar
                LedStatus::ON,     // greenCar
                LedStatus::ON,     // redPedestrian
                LedStatus::OFF     // greenPedestrian
            ),
            "GREEN_CAR: Green to cars, red to pedestrians"
        ),

        // 1 State: YELLOW_CAR
        StateInfo(
            SemaphoreState::YELLOW_CAR,
            TimingConfig::YELLOW_CAR_DURATION,
            LedConfiguration(
                LedStatus::OFF,    // redCar
                LedStatus::ON,     // yellowCar
                LedStatus::OFF,    // greenCar
                LedStatus::ON,     // redPedestrian
                LedStatus::OFF     // greenPedestrian
            ),
            "YELLOW_CAR: Yellow to cars, red to pedestrians"
        ),

        // 2 State: SAFETY_GAP_BEFORE
        StateInfo(
            SemaphoreState::SAFETY_GAP_BEFORE,
            TimingConfig::SAFETY_GAP_DURATION,
            LedConfiguration(
                LedStatus::ON,     // redCar
                LedStatus::OFF,    // yellowCar
                LedStatus::OFF,    // greenCar
                LedStatus::ON,     // redPedestrian
                LedStatus::OFF     // greenPedestrian
            ),
            "SAFETY_GAP_BEFORE: All red (safety gap before changing to green for pedestrians)"
        ),

        // 3 State: GREEN_PEDESTRIAN
        StateInfo(
            SemaphoreState::GREEN_PEDESTRIAN,
            TimingConfig::GREEN_PEDESTRIAN_DURATION,
            LedConfiguration(
                LedStatus::ON,     // redCar
                LedStatus::OFF,    // yellowCar
                LedStatus::OFF,    // greenCar
                LedStatus::OFF,    // redPedestrian
                LedStatus::ON      // greenPedestrian
            ),
            "GREEN_PEDESTRIAN: Green to pedestrians, red to cars"
        ),

        // 4 State: SAFETY_GAP_AFTER
        StateInfo(
            SemaphoreState::SAFETY_GAP_AFTER,
            TimingConfig::SAFETY_GAP_DURATION,
            LedConfiguration(
                LedStatus::ON,     // redCar
                LedStatus::OFF,    // yellowCar
                LedStatus::OFF,    // greenCar
                LedStatus::ON,     // redPedestrian
                LedStatus::OFF     // greenPedestrian
            ),
            "SAFETY_GAP_AFTER: All red (safety gap after changing to green for pedestrians)"
        )
    };
public:
    /**
     * @brief Get informations of a specific state
     */
    static const StateInfo& getStateInfo(SemaphoreState state) {
        return stateTable_[toIndex(state)];
    }

    /**
     * @brief Get the state duration
     */
    static uint32_t getStateDuration(SemaphoreState state) {
        return stateTable_[toIndex(state)].duration;
    }

    /**
     * @brief Get the LED configuration for a specific state
     */
    static const LedConfiguration& getLedConfiguration(SemaphoreState state) {
        return stateTable_[toIndex(state)].ledConfig;
    }    
};

// Static table definition
constexpr StateInfo StateTable::stateTable_[5];

/**
 * @brief Semaphore states machine 
 * 
 * Manages the transitions between states and control the hardware through HAL.
 * Thread-safe to use with FreeRTOS
 */
class SemaphoreStateMachine {
private:
    SemaphoreState currentState_;
    IHardwareController& hardwareController_;
    uint32_t stateStartTime_;
    uint32_t cycleCount_;
    bool isInitialized_;

    /**
     * @brief Update the hardware with the current state's LED configuration
     */
    void updateHardware() {
        const LedConfiguration& config = StateTable::getLedConfiguration(currentState_);
        hardwareController_.applyConfiguration(config);
    }

    /**
     * @brief Register a state transition (to debug/logging)
     */
    void logStateChange() const {
        if (SerialConfig::ENABLE_DEBUG) {
            const StateInfo& info = StateTable::getStateInfo(currentState_);
            Serial.print(F("[STATE] Cycle: "));
            Serial.print(cycleCount_);
            Serial.print(F(" | State: "));
            Serial.print(toIndex(currentState_));
            Serial.print(F(" | "));
            Serial.println(info.description);
        }

    }
public:
    /**
     * @brief Constructor
     * @param hardwareController Reference to the hardware controller (HAL)
     */
    explicit SemaphoreStateMachine(IHardwareController& hardwareController)
        : currentState_(SemaphoreState::GREEN_CAR),
          hardwareController_(hardwareController),
          stateStartTime_(0),
          cycleCount_(0),
          isInitialized_(false) {}
    
    /**
     * @brief Initialize the state machine
     * Must be called once before using the state machine
     */
    void initialize() {
        if (!isInitialized_) {
            hardwareController_.initialize();
            isInitialized_ = true;
        }
    }

    /**
     * @brief Start the state machine operation
     * Define the initial state and the reference time
     */
    void begin() {
        if (!isInitialized_) {
            initialize();
        }

        currentState_ = SemaphoreState::GREEN_CAR;
        stateStartTime_ = millis();
        cycleCount_ = 1;

        updateHardware();
        logStateChange();
    }

    /**
     * @brief Update the state machine
     * Verify if is the time to transition to the next state
     * and execute the transition if necessary
     * 
     * @return true if a state transition occurred, false otherwise
     */
    bool update() {
        if (!isInitialized_) {
            return false;
        }

        const uint32_t currentTime = millis();
        const uint32_t elapsedTime = currentTime - stateStartTime_;
        const uint32_t stateDuration = StateTable::getStateDuration(currentState_);

        // Verify if it's time to transition to the next state
        if (elapsedTime >= stateDuration) {
            transitionToNextState();
            return true;
        }

        return false;
    }

    /**
     * @brief Transition to the next state in the sequence
     */
    void transitionToNextState() {
        // Save the previous state for logging
        SemaphoreState previousState = currentState_;

        // Move to the next state
        ++currentState_;

        // If returned to the first state, increment the cycle count
        if (currentState_ == SemaphoreState::GREEN_CAR &&
            previousState == SemaphoreState::SAFETY_GAP_AFTER) {
            cycleCount_++;   
        }

        // Update the initial time for the new state
        stateStartTime_ = millis();

        // Apply the new state's LED configuration to the hardware
        updateHardware();

        // Log the state change
        logStateChange();
    }

    /**
     * @brief Get the current state
     */
    SemaphoreState getCurrentState() const {
        return currentState_;
    }

    /**
     * @brief Get the current number cycle
     */
    uint32_t getCycleCount() const {
        return cycleCount_;
    }

    /**
     * @brief Get the remaining time in the current state (in milliseconds)
     */
    uint32_t getTimeRemainingInState() const {
        const uint32_t elapsedTime = millis() - stateStartTime_;
        const uint32_t stateDuration = StateTable::getStateDuration(currentState_);

        if (elapsedTime >= stateDuration) {
            return 0;
        } 

        return stateDuration - elapsedTime;
    }

    /**
     * @brief Emergency mode - turn off all
     */
    void emergencyStop() {
        hardwareController_.turnAllLedsOff();

        if (SerialConfig::ENABLE_DEBUG) {
            Serial.println(F("[EMERGENCY] All LEDs turned OFF"));
        }
    }

    /**
     * @brief Verify if is initialized
     */
    bool isInitialized() const {
        return isInitialized_;
    }
};

} // namespace SemaphoreSystem

#endif // SEMAPHORE_STATE_MACHINE_H