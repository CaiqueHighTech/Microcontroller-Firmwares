/**
 * @file SemaphoreTasks.h
 * @brief Definition of FreeRTOS tasks
 * @version 2.0.0
 * 
 * Implement the tasks what will execute concurrently in FreeRTOS
 * following the bests concurrent programming practices
 */

#ifndef SEMAPHORE_TASKS_H
#define SEMAPHORE_TASKS_H

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include "Config.h"
#include "Types.h"
#include "Semaphore_State_Machine.h"

namespace SemaphoreSystem {

/**
 * @brief Class what encapsulate the shared context between tasks
 * 
 * Use mutex to protect shared resources (thread-safety)
 */
class SharedContext {
private:
    SemaphoreStateMachine& stateMachine_;
    SemaphoreHandle_t mutex_;
    bool systemActive_;
    uint32_t totalTransitions_;

public:
    /**
     * @brief Constructor
     */
    explicit SharedContext(SemaphoreStateMachine& sm)
        : stateMachine_(sm),
          mutex_(nullptr),
          systemActive_(true),
          totalTransitions_(0) {
        // Create binary mutex to protection
        mutex_ = xSemaphoreCreateMutex();
        
        if (mutex_ == nullptr) {
            Serial.println(F("[ERROR] Failed to create mutex!"))
        }
    }

    /**
     * @brief Get the states machine reference (with protection)
     */
    SemaphoreStateMachine& getStateMachine() {
        return stateMachine_;
    }

    /**
     * @brief Lock the mutex (blocker)
     */
    bool lock(TickType_t timeout = portMAX_DELAY) {
        if (mutex_ != nullptr) {
            return xSemaphoreTake(mutex_, timeout) == pdTRUE;
        }
        return false;
    }

    /**
     * @brief Release the mutex
     */
    void unlock() {
        if (mutex_ != nullptr) {
            xSemaphoreGive(mutex_);
        }
    }

    /**
     * @brief Verify if the system is active
     */
    bool isSystemActive() const {
        return systemActive_;
    }

    /**
     * @brief Define the system state
     */
    void setSystemActive(bool active) {
        systemActive_ = active;
    }

    /**
     * @brief Increment the transitions contator
     */
    void incrementTransitions() {
        totalTransitions_++;
    }

    /**
     * @brief Get the total transitions
     */
    uint32_t getTotalTransitions() const {
        return totalTransitions_;
    }

    /**
     * @brief Destructor
     */
    ~SharedContext() {
        if (mutex_ != nullptr) {
            vSemaphoreDelete(mutex_);
        }
    }

};

/**
 * @brief RAII wrapper to lock/unlock automatic of mutex
 * Guarantee what mutex be liberate even in case of execution
 */
class ScopedLock {
private:
    SharedContext& context_;
    bool locked_;
public:
    explicit ScopedLock(SharedContext& ctx, TickType_t timeout = portMAX_DELAY)
        : context_(ctx), locked_(false) {
        locked_ = context_.lock(timeout);   
    }

    ~ScopedLock() {
        if (locked_) {
            context_.unlock();
        }
    }

    bool isLocked() const {
        return locked_;
    }

    // Delete copy/move
    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;
};

/**
 * @brief Principal task of semaphore
 * 
 * Responsable to update the states machine and manage
 * the transitions of semaphore
 * 
 * @param pvParameters Pointer to SharedContext
 */
void semaphoreControlTask(void* pvParameters) {
    SharedContext* context = static_cast<SharedContext*>(pvParameters);

    if (context == nullptr) {
        Serial.println(F("[ERROR] Semaphore task: null context!"));
        vTaskDelete(nullptr); // Remove the own task
        return;
    }

    Serial.println(F("[TASK] Semaphore Control Task started"));

    // Infinite loop of task
    for (;;) {
        // Protection with mutex using RAII
        {
            ScopedLock lock(*context);

            if (lock.isLocked() && context -> isSystemActive()) {
                // Update the states machine
                bool stateChanged = context -> getStateMachine().update();

                // If happened state change, increments the contator
                if (stateChanged) {
                    context -> incrementTransitions();
                }
            }
        } // Lock is released automatically here (RAII)

        // Yield to others tasks (cooperation)
        // Small delay to don't overload the cpu
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms
    }
}

/**
 * @brief Task of monitoring and statistics
 * 
 * Display periodically informations about the system
 * (current state, remaining time, cycles, etc.)
 * 
 * @param pvParameters Pointer to SharedContext
 */
void monitorTask(void* pvParameters) {
    SharedContext* context = static_cast<SharedContext*>(pvParameters);

    if (context == nullptr) {
        Serial.println(F("[ERROR] Monitor task: null context!"));
        vTaskDelete(nullptr);
        return;
    }

    Serial.println(F("[TASK] Monitor Task started"));

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(RTOSConfig::MONITOR_PERIOD_MS);
    
    // Infinite loop of task
    for (;;) {
        // Await the next period (precise periodic execution)
        vTaskDelete(&lastWakeTime, period);

        // Protection with mutex 
        {
            ScopedLock lock(*context);

            if (lock.isLocked() && context -> isSystemActive()) {
                SemaphoreStateMachine& sm = context -> getStateMachine();

                // Colect informations
                SemaphoreState currentState = sm.getCurrentState();
                uint32_t timeRemaining = sm.getTimeRemainingInState();
                uint32_t cycleCount = sm.getCycleCount();
                uint32_t totalTransitions = context ->  getTotalTransitions();

                // Display statistics
                Serial.println(F("\n========== SYSTEM STATUS =========="));
                Serial.print(F("Current State: "));
                Serial.println(toIndex(currentState));

                Serial.print(F("Time Remaining: "));
                Serial.print(timeRemaining / 1000);
                Serial.println(F("s"));

                Serial.print(F("Cycle Count: "));
                Serial.println(cycleCount);

                Serial.print(F("Total Transitions: "));
                Serial.println(totalTransitions);
                
                // Memory informations (debug)
                Serial.print(F("Free Heap: "));
                Serial.print(xPortGetFreeHeapSize());
                Serial.println(F(" byets"));
                
                Serial.println(F("===================================\n"));
            }
        }
    }
}

/**
 * @brief Tasks manager
 * 
 * Encapsulate the creation and manages the tasks from FreeRTOS
 */
class TaskManager {
private:
    SharedContext& context_;
    TaskHandle_t semaphoreTaskHandle_;
    TaskHandle_t monitorTaskHandle_;
    bool tasksCreated_;
public:
    /**
     * @brief Constructor
     */
    explicit TaskManager(SharedContext& ctx)
        : context_(ctx),
          semaphoreTaskHandle_(nullptr),
          monitorTaskHandle_(nullptr),
          tasksCreated_(false) {}
    
    /**
     * @brief Create and initiate all the tasks
     * @return true if all the tasks were created with success 
     */
    bool createTasks() {
        if (tasksCreated_) {
            Serial.println(F("[WARN] Tasks already created!"));
            return true;
        }

        BaseType_t result;

        // Create the task of semaphore (higher priority)
        result = xTaskCreate(
            semaphoreControlTask,
            "SemaphoreCtrl",
            RTOSConfig::SEMAPHORE_TASK_STACK_SIZE,
            &context_,
            RTOSConfig::SEMAPHORE_TASK_PRIORITY,
            &semaphoreTaskHandle_
        );

        if (result != pdPass) {
            Serial.println(F("[ERROR] Failed to create Semaphore task!"));
            return false;
        }

        // Create the monitoring task (lower priority)
        result = xTaskCreate(
            monitorTask,
            "Monitor",
            RTOSConfig::MONITOR_TASK_STACK_SIZE,
            &context_,
            RTOSConfig::MONITOR_TASK_PRIORITY,
            &monitorTaskHandle_
        );

        if (result != pdPass) {
            Serial.println(F("[ERROR] Failed to create Monitor task!"));
            // Remove the semaphore task if monitor fail
            if (semaphoreTaskHandle_ != nullptr) {
                vTaskDelete(semaphoreTaskHandle_);
            }
            return false;
        }

        taskCreated_ = true;
        Serial.println(F("[INFO] All tasks created successfully!"));
        return true;
    }

    /**
     * @brief Suspend all the tasks
     */
    void suspendAllTasks() {
        if (semaphoreTaskHandle_ != nullptr) {
            vTaskSuspend(semaphoreTaskHandle_);
        }
        if (monitorTaskHandle_ != nullptr) {
            vTaskSuspend(monitorTaskHandle_);
        }

        context_.setSystemActive(false);
        Serial.println(F("[INFO] All tasks suspended"));
    }

    /**
     * @brief Summary all the tasks
     */
    void resumeAllTasks() {
        if (semaphoreTaskHandle_ != nullptr) {
            vTaskResume(semaphoreTaskHandle_);
        }
        if (monitorTaskHandle_ != nullptr) {
            vTaskResume(monitorTaskHandle_);
        }
        
        context_.setSystemActive(true);
        Serial.println(F("[INFO] All tasks resumed")); 
    }

    /**
     * @brief Verify if the tasks were created
     */
    bool areTasksCreated() const {
        return tasksCreated_;
    }
};

} // namespace SemaphoreSystem

#endif // SEMAPHORE_TASKS_H

