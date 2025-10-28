/**
 * @file Config.h
 * @brief Configuration settings for the application.
 * @version 2.0.0
 * @date 2025
 * 
 * This file contains configuration settings and constants used throughout the application.
 * It is designed to be included in various parts of the project to maintain consistency.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

namespace SemaphoreSystem {

/**
 * @brief Hardware configurations - Pinned mappings
 */
namespace HardwareConfig {
    // Car LEDs
    constexpr uint8_t LED_RED_CAR = 13;
    constexpr uint8_t LED_YELLOW_CAR = 12;
    constexpr uint8_t LED_GREEN_CAR = 11;

    // Pedestrian LEDs
    constexpr uint8_t LED_GREEN_PEDESTRIAN = 10;
    constexpr uint8_t LED_RED_PEDESTRIAN = 9;
}
/**
 * @brief Semaphore timing configurations
 * All timings are in milliseconds
 */
namespace TimingConfig {
    constexpr uint32_t GREEN_CAR_DURATION = 20000;   // Duration for green car light
    constexpr uint32_t YELLOW_CAR_DURATION = 3000;  // Duration for yellow light
    constexpr uint32_t SAFETY_GAP_DURATION = 5000;     // Safety gap duration between LEDs
    constexpr uint32_t GREEN_PEDESTRIAN_DURATION = 20000; // Duration for pedestrian crossing

    // Complete cicle duration = 53s
    constexpr uint32_t TOTAL_CYCLE_DURATION = GREEN_CAR_DURATION +
                                              YELLOW_CAR_DURATION +
                                              SAFETY_GAP_DURATION +
                                              GREEN_PEDESTRIAN_DURATION +
                                              SAFETY_GAP_DURATION;
}

/**
 * @brief FreeRTOS configurations
 */
namespace FreeRTOSConfig {
    // Stack size for tasks
    constexpr uint16_t SEMAPHORE_TASK_STACK_SIZE = 256;
    constexpr uint16_t MONITOR_TASK_STACK_SIZE = 128;

    // Task priorities
    constexpr UBaseType_t SEMAPHORE_TASK_PRIORITY = 2;
    constexpr UBaseType_t MONITOR_TASK_PRIORITY = 1;

    // Period of verification for task of monitoring
    constexpr TickType_t MONITOR_PERIOD_MS = 1000;
}

/**
 * @brief Serial communication configurations
 */
namespace SerialConfig {
    constexpr uint32_t BAUD_RATE = 115200;
    constexpr bool ENABLE_DEBUG = true;
}

} // namespace SemaphoreSystem

#endif // CONFIG_H