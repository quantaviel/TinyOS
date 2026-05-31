#include <hal/rtc.h>
#include <hal/cpu.h>
#include <klib/string.h>

static int bcd_to_bin(int);
static void rtc_read_raw(rtc_time_t *t);
static uint8_t rtc_read_reg(uint8_t reg);
static void rtc_write_reg(uint8_t reg, uint8_t val);

static inline void rtc_wait_update(void) {
    while (rtc_read_reg(RTC_REG_A) & 0x80);
}

void rtc_eoi() {
    rtc_read_reg(RTC_REG_C);
}

uint8_t rtc_read_reg(uint8_t reg) {
    cpu::outb(RTC_CMOS_ADDR, reg);
    return cpu::inb(RTC_CMOS_CONF);
}

void rtc_write_reg(uint8_t reg, uint8_t val) {
    cpu::outb(RTC_CMOS_ADDR, reg);
    cpu::outb(RTC_CMOS_CONF, val);
}

void rtc_timer_enable() {
    // select register B and disable NMI
    // read the previous value of B
    uint8_t regb = rtc_read_reg(RTC_REG_B | RTC_NMI_MASK);
    // select again and enable rtc ISR on IRQ 8
    rtc_write_reg(RTC_REG_B | RTC_NMI_MASK, regb | RTC_ENABLE_PIE);
}

void rtc_timer_disable() {
    // select register B and disable NMI
    // read the previous value of B
    uint8_t regb = rtc_read_reg(RTC_REG_B | RTC_NMI_MASK);
    // select again and disable rtc ISR on IRQ 8
    rtc_write_reg(RTC_REG_B | RTC_NMI_MASK, regb & (~RTC_ENABLE_PIE));
}

void rtc_set_rs(int rs) {
    uint8_t rega = rtc_read_reg(RTC_REG_A);
    rtc_write_reg(RTC_REG_A, (rega & 0xF0) | (rs & 0x0F));
}

void rtc_read_time(rtc_time_t *time) {
    rtc_time_t t1, t2;

    do {
        rtc_wait_update();
        rtc_read_raw(&t1);
        rtc_wait_update();
        rtc_read_raw(&t2);
    } while (memcmp(&t1, &t2, sizeof(t1)) != 0);

    *time = t2;
}

void rtc_normalize(rtc_time_t *time) {
    uint8_t regB = rtc_read_reg(RTC_REG_B);

    if (!(regB & 0x04)) {
        time->sec  = bcd_to_bin(time->sec);
        time->min  = bcd_to_bin(time->min);
        time->hour = bcd_to_bin(time->hour & 0x7F);
        time->day  = bcd_to_bin(time->day);
        time->mon  = bcd_to_bin(time->mon);
        time->year = bcd_to_bin(time->year);
    }

    if (!(regB & 0x02)) {
        int pm = time->hour & 0x80;
        time->hour &= 0x7F;
        if (pm && time->hour < 12)
            time->hour += 12;
    }

    // 世纪（如果有）
    uint8_t century = rtc_read_reg(0x32);
    if (century)
        time->year += bcd_to_bin(century) * 100;
    else
        time->year += 2000;  // 常见假设
}

int bcd_to_bin(int num) {
    return (num & 0xF) + (num >> 4) * 10;
}

void
rtc_read_raw(rtc_time_t *t) {
    t->sec  = rtc_read_reg(0x00);
    t->min  = rtc_read_reg(0x02);
    t->hour = rtc_read_reg(0x04);
    t->day  = rtc_read_reg(0x07);
    t->mon  = rtc_read_reg(0x08);
    t->year = rtc_read_reg(0x09);
}