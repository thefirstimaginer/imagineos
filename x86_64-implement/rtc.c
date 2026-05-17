#include "x86_64/port.h"

#define PORT_RTC_COMMAND 0x70
#define PORT_RTC_DATA 0x71
#define RTC_REGISTER_SECONDS 0x00
#define RTC_REGISTER_STATUS_A 0x0A
#define RTC_REGISTER_STATUS_B 0x0B
#define RTC_UPDATE_IN_PROGRESS 0x80
#define RTC_DATA_MODE (1 << 2)

#define RTC_REGISTER_MINUTES 0x02
#define RTC_REGISTER_HOURS   0x04
#define RTC_REGISTER_DAY     0x07
#define RTC_REGISTER_MONTH   0x08
#define RTC_REGISTER_YEAR    0x09

uint8_t rtc_read_register(uint8_t reg) {
    port_outb(PORT_RTC_COMMAND, reg);
    return port_inb(PORT_RTC_DATA);
}

void rtc_wait() {
    while (rtc_read_register(RTC_REGISTER_STATUS_A) & RTC_UPDATE_IN_PROGRESS);
}

uint8_t rtc_is_bcd() {
    return !(rtc_read_register(RTC_REGISTER_STATUS_B) & RTC_DATA_MODE);
}

uint8_t rtc_seconds() {
    uint8_t seconds_a;
    uint8_t seconds_b;
    uint8_t is_bcd = rtc_is_bcd();
    
    do {
        rtc_wait();
        seconds_a = rtc_read_register(RTC_REGISTER_SECONDS);
        rtc_wait();
        seconds_b = rtc_read_register(RTC_REGISTER_SECONDS);
    } while (seconds_a != seconds_b);
    
    if (is_bcd) {
        return (seconds_b & 0x0F) + ((seconds_b & 0xF0) >> 4) * 10;
    }
    
    return seconds_b;
}

uint8_t rtc_get_value(uint8_t reg) {
    uint8_t val;
    rtc_wait();
    val = rtc_read_register(reg);
    
    // Se for BCD, converte para decimal antes de retornar
    if (rtc_is_bcd()) {
        return (val & 0x0F) + ((val & 0xF0) >> 4) * 10;
    }
    return val;
}
