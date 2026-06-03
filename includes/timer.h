#pragma once

#include <stdint-gcc.h>
#include <arch/x86/int.h>
#include <list.h>

namespace timer {

enum timer_mode {
    TIMER_ONESHOT = 0,    // Fire once, then auto-delete
    TIMER_PERIODIC = 1,   // Reload count and continue
};

enum timer_unit {
    TIMER_TICK = 0,       // Count in APIC timer ticks
    TIMER_SEC = 1,        // Count in real-world seconds
};

// Timer event structure
struct timer_event_t {
    list_node_t node; // Embedded list node (must be first)

    uint32_t expires;         // Remaining ticks to fire
    uint32_t initial_count;   // Initial count (for reload)
    timer_mode mode;          // ONESHOT or PERIODIC
    timer_unit unit;          // TICK or SEC
    void (*callback)(void*);  // Callback function
    void* arg;                // Argument passed to callback
    bool active;              // Whether this event is active
};

// Initialize the timer system
// This will calibrate the APIC timer to determine the CPU frequency
void init(uint32_t sys_freq=2048);

// Get the measured CPU frequency in Hz
uint32_t get_sys_freq();

// Register a timer event
// Returns the event pointer on success, nullptr on failure
timer_event_t* register_event(
    uint32_t count,
    timer_mode mode,
    timer_unit unit,
    void (*callback)(void*),
    void* arg
);

// Unregister a timer event
void unregister_event(timer_event_t* event);

// Reset an event's countdown to its initial count
void reset_event(timer_event_t* event);

// Lock/unlock the timer list (for critical sections)
void lock();
void unlock();

} // namespace timer