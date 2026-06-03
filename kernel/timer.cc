#include <timer.h>
#include <arch/x86/iroute.h>
#include <hal/apic.h>
#include <hal/cpu.h>
#include <hal/rtc.h>
#include <klib/fmt.h>
#include <mm/kalloc.h>

namespace timer {

static list_node_t timer_list_head;  // Doubly-linked list head

static uint32_t sys_freq_hz = 0;          // System frequency in Hz
static uint32_t ticks_per_interrupt = 0;  // Ticks during one interrupt

static volatile uint32_t rtc_interrupt_count = 0;

// Lock the timer list
void lock() {
    cpu::disable_interrupts();
}

// Unlock the timer list
void unlock() {
    cpu::enable_interrupts();
}

// Timer tick handler - decrements all events
extern "C"
void timer_tick_handler(int_frame_t* frame) {
    apic::eoi();
    list_node_t* node = timer_list_head.next;
    while (node != &timer_list_head) {
        timer_event_t* curr = reinterpret_cast<timer_event_t*>(node);
        if (curr->active && curr->expires > 0) {
            curr->expires--;

            if (curr->expires == 0) {
                switch (curr->mode) {
                    case TIMER_ONESHOT: // oneshot mode - deactivate
                        curr->active = false;
                        break;
                    case TIMER_PERIODIC: // Reload the count
                        curr->expires = curr->initial_count;
                        break;
                }
                
                // Fire the callback
                if (curr->callback) {
                    curr->callback(curr->arg);
                }

            }
        }
        node = node->next;
    }
}

// RTC periodic interrupt handler - used during calibration
extern "C"
void rtc_calibration_handler(int_frame_t* frame) {
    rtc_eoi();
    rtc_interrupt_count = rtc_interrupt_count + 1;
}

void apic_calibration_handler(int_frame_t* frame) {
    apic::eoi();
}

// Forward declarations
void calibrate(uint32_t sys_freq);

// Initialize and calibrate the timer system
void init(uint32_t sys_freq) {
    fmt::println("Timer: Initializing timer system...");

    // Calibrate APIC timer using RTC
    calibrate(sys_freq);

    cpu::disable_interrupts();

    sys_freq_hz = sys_freq;

    // Initialize the event list
    list_init(&timer_list_head);

    // Register the timer tick handler
    iroute_register(LAPIC_LVT_TIMER_VEC, timer_tick_handler);

    // Set APIC timer to periodic mode
    // Fixed jiffy rate: jiffy_hz Hz (e.g. 100 Hz)
    // Each interrupt fires after apic_ticks_per_jiffy APIC ticks
    apic::write_reg(LAPIC_LVT_TIMER, LAPIC_TIMER_PERIODIC | LAPIC_LVT_TIMER_VEC);
    apic::write_reg(LAPIC_DIVIDE_CONF, LAPIC_TIMER_DIVIDE_16);
    apic::write_reg(LAPIC_INIT_COUNT, ticks_per_interrupt);

    fmt::println("Timer: System frequency: {} Hz", sys_freq_hz);
    fmt::println("Timer: APIC ticks per interrupt: {}", ticks_per_interrupt);

    cpu::enable_interrupts();
}

// Calibrate APIC timer using RTC periodic interrupt
void calibrate(uint32_t sys_freq) {
    cpu::disable_interrupts();

    // Register RTC calibration handler before enabling RTC
    iroute_register(RTC_TIMER_IRQ, rtc_calibration_handler);
    iroute_register(LAPIC_LVT_TIMER_VEC, apic_calibration_handler);

    // Set APIC timer to periodic mode with maximum count
    // This ensures continuous counting throughout the calibration
    apic::write_reg(LAPIC_LVT_TIMER, LAPIC_TIMER_PERIODIC | LAPIC_LVT_TIMER_VEC);
    apic::write_reg(LAPIC_DIVIDE_CONF, LAPIC_TIMER_DIVIDE_16);
    apic::write_reg(LAPIC_INIT_COUNT, 0xFFFFFFFF);

    // Enable RTC periodic interrupt at 1024 Hz
    rtc_interrupt_count = 0;
    rtc_timer_enable();

    uint32_t start_count = apic::read_reg(LAPIC_CURR_COUNT);
    cpu::enable_interrupts();

    // Wait for 10 RTC interrupts (about 10/1024 seconds)
    while (rtc_interrupt_count < 10) {
        // busy wait
    }

    cpu::disable_interrupts();
    uint32_t curr_count = apic::read_reg(LAPIC_CURR_COUNT);

    // Disable RTC timer after calibration
    rtc_timer_disable();

    // Read elapsed APIC ticks (start_count - curr_count since timer counts down)
    uint32_t elapsed_ticks = start_count - curr_count;

    fmt::println("elapsed_ticks: {}, start count: {}, end count: {}", elapsed_ticks, start_count, curr_count);

    // Calculate APIC ticks per second
    // 10 RTC interrupts at 1024 Hz = 10/1024 seconds
    ticks_per_interrupt = elapsed_ticks / sys_freq * 1024 / 10;
}

// Get measured CPU frequency
uint32_t get_sys_freq() {
    return sys_freq_hz;
}

// Convert seconds to jiffies (scheduler ticks)
static uint32_t sec_to_ticks(uint32_t seconds) {
    return seconds * sys_freq_hz;
}

// Register a new timer event
timer_event_t* register_event(
    uint32_t count,
    timer_mode mode,
    timer_unit unit,
    void (*callback)(void*),
    void* arg
) {
    if (!callback) {
        return nullptr;
    }

    // Allocate the event structure
    timer_event_t* event = (timer_event_t*)mm::kvm::kalloc(sizeof(timer_event_t));
    if (!event) {
        return nullptr;
    }

    // Initialize the event
    event->callback = callback;
    event->arg = arg;
    event->mode = mode;
    event->unit = unit;
    event->active = true;

    if (unit == TIMER_SEC) {
        // Convert seconds to ticks
        event->initial_count = sec_to_ticks(count);
    } else {
        // count is already in ticks
        event->initial_count = count;
    }
    event->expires = event->initial_count;

    // Add to list
    lock();
    list_push_back(&timer_list_head, &event->node);
    unlock();

    return event;
}

// Unregister a timer event
void unregister_event(timer_event_t* event) {
    if (!event) {
        return;
    }

    lock();
    list_remove(&event->node);
    unlock();

    // Free the event structure
    mm::kvm::kfree(event);
}

// Reset an event's countdown
void reset_event(timer_event_t* event) {
    if (!event) {
        return;
    }

    lock();
    event->expires = event->initial_count;
    event->active = true;
    unlock();
}

// Align to next second boundary using RTC
// This is called internally when setting up second-based timers
void align_to_second() {
    rtc_time_t cur_time;
    rtc_read_time(&cur_time);
    rtc_normalize(&cur_time);

    // Calculate microseconds to next second
    // For simplicity, just set a oneshot timer for ~1ms and then switch to periodic
    // This approach uses the APIC timer directly without RTC alignment for now

    // TODO: Implement more precise alignment using RTC alarm
}

} // namespace timer