#pragma once

#include <stdint-gcc.h>

#define RTC_INT_IRQ     8
#define RTC_TIMER_IRQ   204

// Index or "register number"
#define RTC_CMOS_ADDR   0x70

// Configuration space
#define RTC_CMOS_CONF   0x71

#define RTC_REG_A       0x0A
#define RTC_REG_B       0x0B
#define RTC_REG_C       0x0C

#define RTC_NMI_MASK    0x80
#define RTC_ENABLE_PIE  0x40

#define RTC_TIMER_HZ    1024

typedef struct rtc_time {
    uint8_t  sec;
    uint8_t  min;
    uint8_t  hour;
    uint8_t  day;
    uint8_t  mon;
    uint16_t year;
} rtc_time_t;

void rtc_eoi();

void rtc_timer_enable();
void rtc_timer_disable();

void rtc_normalize(rtc_time_t *time);
void rtc_read_time(rtc_time_t *time);

void rtc_set_rs(int rs);